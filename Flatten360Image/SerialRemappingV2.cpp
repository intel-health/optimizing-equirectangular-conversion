// The code here takes inspiration from the python code at 
// https://github.com/fuenwang/Equirec2Perspec/blob/master/Equirec2Perspec.py but in addition to
// converting to C++, many additional features, memory reductions, and optimizations have been implemented too by
// Doug Bogia

#include "SerialRemappingV2.hpp"
#include <chrono>
#include "TimingStats.hpp"
#include <opencv2/calib3d.hpp>

#ifdef VTUNE_API
#include "ittnotify.h"
#pragma comment(lib, "libittnotify.lib")
extern __itt_domain *pittTests_domain;
// Create string handle for denoting when the kernel is running
wchar_t const *pSerialRemappingV2Extract = _T("SerialRemapping Extract Kernel");
__itt_string_handle *handle_SerialRemappingV2_extract_kernel = __itt_string_handle_create(pSerialRemappingV2Extract);
wchar_t const *pSerialRemappingV2Calc = _T("SerialRemapping Calc Kernel");
__itt_string_handle *handle_SerialRemappingV2_calc_kernel = __itt_string_handle_create(pSerialRemappingV2Calc);
#endif

SerialRemappingV2::SerialRemappingV2(SParameters& parameters) : BaseAlgorithm(parameters)
{
	m_storageType = SRV2_INIT;
}

SerialRemappingV2::~SerialRemappingV2()
{
	delete[] m_pXYPoints;
	m_pXYPoints = NULL;
	delete[] m_pXPoints;
	m_pXPoints = NULL;
	delete[] m_pYPoints;
	m_pYPoints = NULL;
}

std::string SerialRemappingV2::GetDescription()
{
	switch (m_storageType)
	{
	case SRV2_AOS:
		return "V2 Single loop point by point conversion from equirectangular to flat.  Array of structure row/column layout.";
		break;
	case SRV2_SOA:
		return "V2 Single loop point by point conversion from equirectangular to flat.  Two arrays for X and Y points.";
		break;
	}

	return "Unknown";
}

// Pass in theta, phi, and psi in radians, not degrees
void SerialRemappingV2::ComputeRotationMatrix(float radTheta, float radPhi, float radPsi)
{
	// Python code snippet that this is attempting to match
	//# Compute a matrix representing the three rotations THETA, PHI, and PSI
	//x_axis = np.array([1.0, 0.0, 0.0], np.float32)
	//y_axis = np.array([0.0, 1.0, 0.0], np.float32)
	//z_axis = np.array([0.0, 0.0, 1.0], np.float32)
	//Ry, _ = cv2.Rodrigues(y_axis * np.radians(THETA))
	//Rx, _ = cv2.Rodrigues(np.dot(Ry, x_axis) * np.radians(PHI))
	//Rz, _ = cv2.Rodrigues(np.dot(Rx, np.dot(Ry, z_axis)) * np.radians(PSI))
	//R = Rz @ Rx @ Ry
	cv::Mat x_axis = (cv::Mat_<float>(3, 1) << 1, 0, 0);
	cv::Mat y_axis = (cv::Mat_<float>(3, 1) << 0, 1, 0);
	cv::Mat z_axis = (cv::Mat_<float>(3, 1) << 0, 0, 1);
	cv::Mat Rx;
	cv::Mat Ry;
	cv::Mat Rz;
	cv::Mat R;

	cv::Rodrigues(y_axis * radTheta, Ry);
	cv::Rodrigues(Ry * x_axis * radPhi, Rx);
	cv::Rodrigues(Rx * Ry * z_axis * radPsi, Rz);

	m_rotationMatrix = Rz * Rx * Ry;

//#define CONFIRMATION_PRINTS
#ifdef CONFIRMATION_PRINTS
	printf("ComputeRotationMatrix:\n");
	for (int row = 0; row < 3; row++) 
	{
		for (int col = 0; col < 3; col++)
		{
			printf("%10.4f    ", m_rotationMatrix.at<float>(row, col));
		}
		printf("\n");
	}
#endif
}

void SerialRemappingV2::FrameCalculations(bool bParametersChanged)
{
	if (bParametersChanged || m_bFrameCalcRequired)
	{
#ifdef VTUNE_API
		__itt_task_begin(pittTests_domain, __itt_null, __itt_null, handle_SerialRemappingV2_calc_kernel);
#endif
		BaseAlgorithm::FrameCalculations(bParametersChanged);

		ComputeRotationMatrix((float)m_parameters->m_yaw * DEGREE_CONVERSION_FACTOR, (float)m_parameters->m_pitch * DEGREE_CONVERSION_FACTOR, (float)m_parameters->m_roll * DEGREE_CONVERSION_FACTOR);

		float f;
		float cx;
		float cy;
		float invf;
		float translatecx;
		float translatecy;
		float m00 = m_rotationMatrix.at<float>(0, 0);
		float m01 = m_rotationMatrix.at<float>(0, 1);
		float m02 = m_rotationMatrix.at<float>(0, 2);
		float m10 = m_rotationMatrix.at<float>(1, 0);
		float m11 = m_rotationMatrix.at<float>(1, 1);
		float m12 = m_rotationMatrix.at<float>(1, 2);
		float m20 = m_rotationMatrix.at<float>(2, 0);
		float m21 = m_rotationMatrix.at<float>(2, 1);
		float m22 = m_rotationMatrix.at<float>(2, 2);
		float imageWidth = m_parameters->m_image[m_parameters->m_imageIndex].cols - 1;
		float imageHeight = m_parameters->m_image[m_parameters->m_imageIndex].rows - 1;;
		float xDiv = 2 * M_PI;

		std::chrono::system_clock::time_point startTime;

		startTime = std::chrono::system_clock::now();

		f = 0.5 * m_parameters->m_widthOutput * 1 / tan(0.5 * m_parameters->m_fov / 180.0 * M_PI);
		cx = ((float)m_parameters->m_widthOutput - 1.0f) / 2.0f;
		cy = ((float)m_parameters->m_heightOutput - 1.0f) / 2.0f;

		// In concept, the intent of this section of the code is to create an inverse of the intrinsic matrix K
		// (see https://ksimek.github.io/2013/08/13/intrinsic for explanation).  However, in this case we will
		// not represent as an actual matrix, but just calculate the individual values and then use them
		// to initialze the 2D array of XYZ points.
		// Initialize the inverse of the intrinsic matrix K
		//K_inv = (Mat_<float>(3, 3) <<
		//	1 / f, 0, -cx / f,
		//	0, 1 / f, -cy / f,
		//	0, 0, 1);
		invf = 1.0f / f;
		translatecx = -cx * invf;
		translatecy = -cy * invf;

		switch (m_storageType)
		{
		case SRV2_AOS:
		{
			Point2D *pPoints = m_pXYPoints;

			for (int row = 0; row < m_parameters->m_heightOutput; row++)
			{
				for (int col = 0; col < m_parameters->m_widthOutput; col++)
				{
					float x = col * invf + translatecx;
					float y = row * invf + translatecy;
					float z = 1.0f;
					float norm;

					// Calculate xyz * R, save the initial x, y, and z values for the computation
					float eX = x;
					float eY = y;
					float eZ = z;

					x = eX * m00 + eY * m01 + eZ * m02;
					y = eX * m10 + eY * m11 + eZ * m12;
					z = eX * m20 + eY * m21 + eZ * m22;

					norm = sqrt(x * x + y * y + z * z);

					x = atan2(x / norm, z / norm);
					y = asin(y / norm);

					pPoints->m_x = (x / xDiv + 0.5) * imageWidth;
					pPoints->m_y = (y / M_PI + 0.5) * imageHeight;

					pPoints++;
				}
			}

			break;
		}
		case SRV2_SOA:
		{
			float *pXPoints = m_pXPoints;
			float *pYPoints = m_pYPoints;

			for (int row = 0; row < m_parameters->m_heightOutput; row++)
			{
				for (int col = 0; col < m_parameters->m_widthOutput; col++)
				{
					float x = col * invf + translatecx;
					float y = row * invf + translatecy;
					float z = 1.0f;
					float norm;

					// Calculate xyz * R, save the initial x, y, and z values for the computation
					float eX = x;
					float eY = y;
					float eZ = z;

					x = eX * m00 + eY * m01 + eZ * m02;
					y = eX * m10 + eY * m11 + eZ * m12;
					z = eX * m20 + eY * m21 + eZ * m22;

					norm = sqrt(x * x + y * y + z * z);

					x = atan2(x / norm, z / norm);
					y = asin(y / norm);

					*pXPoints = (x / xDiv + 0.5) * imageWidth;
					*pYPoints = (y / M_PI + 0.5) * imageHeight;

					pXPoints++;
					pYPoints++;
				}
			}

			break;
		}
		}
		m_bFrameCalcRequired = false;
#ifdef VTUNE_API
		__itt_task_end(pittTests_domain);
#endif

	}
}

cv::Mat SerialRemappingV2::ExtractFrameImage()
{
#ifdef VTUNE_API
	__itt_task_begin(pittTests_domain, __itt_null, __itt_null, handle_SerialRemappingV2_extract_kernel);
#endif

	cv::Mat retVal;
	std::chrono::system_clock::time_point startTime;

	startTime = std::chrono::system_clock::now();

	switch (m_storageType)
	{
	case SRV2_AOS:
	{
		cv::Mat map = cv::Mat(m_parameters->m_heightOutput, m_parameters->m_widthOutput, CV_32FC2, m_pXYPoints);

		cv::remap(m_parameters->m_image[m_parameters->m_imageIndex], retVal, map, cv::Mat{}, cv::INTER_CUBIC, cv::BORDER_WRAP);

		TimingStats::GetTimingStats()->AddIterationResults(ETimingType::TIMING_REMAP, startTime, std::chrono::system_clock::now());

		break;
	}
	case SRV2_SOA:
	{
		cv::Mat mapX = cv::Mat(m_parameters->m_heightOutput, m_parameters->m_widthOutput, CV_32FC1, m_pXPoints);
		cv::Mat mapY = cv::Mat(m_parameters->m_heightOutput, m_parameters->m_widthOutput, CV_32FC1, m_pYPoints);

		cv::remap(m_parameters->m_image[m_parameters->m_imageIndex], retVal, mapX, mapY, cv::INTER_CUBIC, cv::BORDER_WRAP);

		TimingStats::GetTimingStats()->AddIterationResults(ETimingType::TIMING_REMAP, startTime, std::chrono::system_clock::now());
		break;
	}
	}

#ifdef VTUNE_API
	__itt_task_end(pittTests_domain);
#endif

	return retVal;
}

cv::Mat SerialRemappingV2::GetDebugImage()
{
	cv::Mat retVal;

	m_parameters->m_image[m_parameters->m_imageIndex].copyTo(retVal);

	switch (m_storageType)
	{
	case SRV2_AOS:
	{
		Point2D *pXYElement = &m_pXYPoints[0];
		// Draw the points across the top of the viewing region using blue
		for (int x = 0; x < m_parameters->m_widthOutput; x++)
		{
			cv::Point pt = cv::Point(pXYElement->m_x, pXYElement->m_y);
			cv::line(retVal, pt, pt, cv::Scalar(255, 0, 0), 10);

			pXYElement++;
		}
		// Draw the left (red) and right (green) sides of the viewing region
		for (int y = 1; y < m_parameters->m_heightOutput - 2; y++)
		{
			cv::Point ptLeft = cv::Point(pXYElement->m_x, pXYElement->m_y);
			cv::line(retVal, ptLeft, ptLeft, cv::Scalar(0, 0, 255), 10);

			pXYElement += m_parameters->m_widthOutput - 1;
			cv::Point ptRight = cv::Point(pXYElement->m_x, pXYElement->m_y);
			cv::line(retVal, ptRight, ptRight, cv::Scalar(0, 255, 0), 10);

			pXYElement++;
		}

		// Draw the points across the bottom of the viewing region in tan
		for (int x = 0; x < m_parameters->m_widthOutput; x++)
		{
			cv::Point pt = cv::Point(pXYElement->m_x, pXYElement->m_y);
			cv::line(retVal, pt, pt, cv::Scalar(74, 136, 175), 10);

			pXYElement++;
		}
		break;
	}
	case SRV2_SOA:
	{
		float *pXElement = &m_pXPoints[0];
		float *pYElement = &m_pYPoints[0];

		// Draw the points on the left of the viewing area in red
		for (int y = 0; y < m_parameters->m_heightOutput; y++)
		{
			cv::Point ptLeft = cv::Point(*pXElement, *pYElement);
			cv::line(retVal, ptLeft, ptLeft, cv::Scalar(0, 0, 255), 10);

			pXElement++;
			pYElement++;
		}

		// Draw the top (blue) and bottom (tan) sides of the viewing region
		for (int x = 1; x < m_parameters->m_widthOutput - 2; x++)
		{
			cv::Point ptTop = cv::Point(*pXElement, *pYElement);
			cv::line(retVal, ptTop, ptTop, cv::Scalar(255, 0, 0), 10);

			pXElement += m_parameters->m_heightOutput - 1;
			pYElement += m_parameters->m_heightOutput - 1;

			cv::Point ptBottom = cv::Point(*pXElement, *pYElement);
			cv::line(retVal, ptBottom, ptBottom, cv::Scalar(74, 136, 175), 10);

			pXElement++;
			pYElement++;
		}

		// Draw the points on the rigth of the viewing area in green
		for (int y = 0; y < m_parameters->m_heightOutput; y++)
		{
			cv::Point pt = cv::Point(*pXElement, *pYElement);
			cv::line(retVal, pt, pt, cv::Scalar(0, 255, 0), 10);

			pXElement++;
			pYElement++;
		}
		break;
	}
	}

	return retVal;
}

bool SerialRemappingV2::StartVariant()
{
	BaseAlgorithm::StartVariant();

	bool bRetVal = false;

	m_storageType++;

	if (m_storageType < SRV2_MAX)
	{
		int size = m_parameters->m_widthOutput * m_parameters->m_heightOutput;

		switch (m_storageType)
		{
		case SRV2_AOS:
		{
			m_pXYPoints = new Point2D[size];
			break;
		}
		case SRV2_SOA:
		{
			m_pXPoints = new float[size];
			m_pYPoints = new float[size];
			break;
		}
		}
		bRetVal = true;
		m_bFrameCalcRequired = true;
	}

	return bRetVal;
}

void SerialRemappingV2::StopVariant()
{
	delete[] m_pXYPoints;
	m_pXYPoints = NULL;
	delete[] m_pXPoints;
	m_pXPoints = NULL;
	delete[] m_pYPoints;
	m_pYPoints = NULL;
}

