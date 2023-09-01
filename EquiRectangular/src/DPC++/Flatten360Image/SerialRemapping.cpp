// Copyright (C) 2023 Intel Corporation
//
// This software and the related documents are Intel copyrighted materials, 
// and your use of them is governed by the express license under which
// they were provided to you ("License"). Unless the License provides
// otherwise, you may not use, modify, copy, publish, distribute, disclose or
// transmit this software or the related documents without Intel's prior
// written permission.
//
// This software and the related documents are provided as is, with no
// express or implied warranties, other than those that are expressly stated
// in the License.

// The code here takes inspiration from the python code at 
// https://github.com/fuenwang/Equirec2Perspec/blob/master/Equirec2Perspec.py but in addition to
// converting to C++, many additional features and optimizations have been implemented too by
// Doug Bogia

#include "SerialRemapping.hpp"
#include <chrono>
#include "TimingStats.hpp"
#include <opencv2/calib3d.hpp>

#ifdef VTUNE_API
#include "ittnotify.h"
#pragma comment(lib, "libittnotify.lib")
extern __itt_domain *pittTests_domain;
// Create string handle for denoting when the kernel is running
wchar_t const *pSerialRemappingExtract = _T("SerialRemapping Extract Kernel");
__itt_string_handle *handle_SerialRemapping_extract_kernel = __itt_string_handle_create(pSerialRemappingExtract);
wchar_t const *pSerialRemappingCalc = _T("SerialRemapping Calc Kernel");
__itt_string_handle *handle_SerialRemapping_calc_kernel = __itt_string_handle_create(pSerialRemappingCalc);
#endif

SerialRemapping::SerialRemapping(SParameters& parameters) : BaseAlgorithm(parameters)
{
	m_storageOrder = STORE_INIT;
}

SerialRemapping::~SerialRemapping()
{
	delete m_pXYZPoints;
	m_pXYZPoints = NULL;
	delete m_pXYPoints;
	m_pXYPoints = NULL;
	delete m_pLonLatPoints;
	m_pLonLatPoints = NULL;
}

std::string SerialRemapping::GetDescription()
{
	switch (m_storageOrder)
	{
	case STORE_ROW_COL:
		return "V1 Multiple loop serial point by point conversion from equirectangular to flat.  Memory array of structure row/column layout.";
		break;
	case STORE_COL_ROW:
		return "V1 Multiple loop serial point by point conversion from equirectangular to flat.  Memory array of structure column/row layout.";
		break;
	//case STORE_SOA:
	//	return "Serial point by point conversion from equirectangular to flat.  Memory structure of arrays layout.";
	//	break;
	}

	return "Unknown";
}

void SerialRemapping::ComputeXYZCoords()
{
	float f;
	float cx;
	float cy;
	float invf;
	float translatecx;
	float translatecy;

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

	switch (m_storageOrder)
	{
	case STORE_ROW_COL:
	{
		Point3D* pElement = &m_pXYZPoints[0];

		for (int y = 0; y < m_parameters->m_heightOutput; y++)
		{
			for (int x = 0; x < m_parameters->m_widthOutput; x++)
			{
				pElement->m_x = x * invf + translatecx;
				pElement->m_y = y * invf + translatecy;
				pElement->m_z = 1.0f;
				pElement++;
			}
		}

//#define CONFIRMATION_PRINTS
#ifdef CONFIRMATION_PRINTS
		Point3D* pRow = &m_pXYZPoints[0];
		printf("Row, Col Point[0, 0] = [%f, %f, %f]\n", pRow[0].m_x, pRow[0].m_y, pRow[0].m_z);
		printf("Point[1, 0] = [%f, %f, %f]\n", pRow[1].m_x, pRow[1].m_y, pRow[1].m_z);
		pRow = &m_pXYZPoints[m_parameters->m_widthOutput];
		printf("Point[0, 1] = [%f, %f, %f]\n", pRow[0].m_x, pRow[0].m_y, pRow[0].m_z);
#endif
		break;
	}
	case STORE_COL_ROW:
	{
#if 1
		Point3D* pElement = &m_pXYZPoints[0];

		for (int x = 0; x < m_parameters->m_widthOutput; x++)
		{
			for (int y = 0; y < m_parameters->m_heightOutput; y++)
			{
				pElement->m_x = x * invf + translatecx;
				pElement->m_y = y * invf + translatecy;
				pElement->m_z = 1.0f;
				pElement++;
			}
		}
#else
		for (int x = 0; x < m_parameters->m_widthOutput; x++)
		{
			Point3D* pCol = &m_pXYZPoints[x * m_parameters->m_heightOutput];

			for (int y = 0; y < m_parameters->m_heightOutput; y++)
			{
				pCol[y].m_x = x * invf + translatecx;
				pCol[y].m_y = y * invf + translatecy;
				pCol[y].m_z = 1.0f;
			}
		}
#endif

#ifdef CONFIRMATION_PRINTS
		Point3D* pCol = &m_pXYZPoints[0];
		printf("Col Row Point[0, 0] = [%f, %f, %f]\n", pCol[0].m_x, pCol[0].m_y, pCol[0].m_z);
		printf("Point[0, 1] = [%f, %f, %f]\n", pCol[1].m_x, pCol[1].m_y, pCol[1].m_z);
		pCol = &m_pXYZPoints[m_parameters->m_heightOutput];
		printf("Point[1, 0] = [%f, %f, %f]\n", pCol[0].m_x, pCol[0].m_y, pCol[0].m_z);
#endif
		break;
	}
//	case STORE_SOA:
//	{
//		float* pX = &m_SoAXYZPoints.m_pX[0];
//		float* pY = &m_SoAXYZPoints.m_pY[0];
//		float* pZ = &m_SoAXYZPoints.m_pZ[0];
//
//		for (int y = 0; y < m_parameters->m_heightOutput; y++)
//		{
//			for (int x = 0; x < m_parameters->m_widthOutput; x++)
//			{
//				*pX = x * invf + translatecx;
//				*pY = y * invf + translatecy;
//				*pZ = 1.0f;
//				pX++;
//				pY++;
//				pZ++;
//			}
//		}
//
//#ifdef CONFIRMATION_PRINTS
//		int index = 0;
//
//		printf("Row, Col Point[0, 0] = [%f, %f, %f]\n", m_SoAXYZPoints.m_pX[index], m_SoAXYZPoints.m_pY[index], m_SoAXYZPoints.m_pZ[index]);
//		index = 1;
//		printf("Point[1, 0] = [%f, %f, %f]\n", m_SoAXYZPoints.m_pX[index], m_SoAXYZPoints.m_pY[index], m_SoAXYZPoints.m_pZ[index]);
//		index = m_parameters->m_widthOutput;
//		printf("Point[0, 1] = [%f, %f, %f]\n", m_SoAXYZPoints.m_pX[index], m_SoAXYZPoints.m_pY[index], m_SoAXYZPoints.m_pZ[index]);
//#endif
//		break;
//	}
	}
}

// Pass in theta, phi, and psi in radians, not degrees
void SerialRemapping::ComputeRotationMatrix(float radTheta, float radPhi, float radPsi)
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

void SerialRemapping::ConvertXYZToLonLat()
{
	// Python code snippet that this is attempting to match along with applying the rotation matrix to the points
	//xyz = xyz @ R.T
	//atan2 = np.arctan2
    //asin = np.arcsin

    //norm = np.linalg.norm(xyz, axis=-1, keepdims=True)
    //xyz_norm = xyz / norm
    //x = xyz_norm[..., 0:1]
    //y = xyz_norm[..., 1:2]
    //z = xyz_norm[..., 2:]

    //lon = atan2(x, z)
    //lat = asin(y)
    //lst = [lon, lat]
	//out = np.concatenate(lst, axis=-1)

	Point3D* pXYZElement = &m_pXYZPoints[0];
	Point2D* pLonLatElement = &m_pLonLatPoints[0];
	float norm;
	// Optimization, pull the values out of the rotational matrix if CACHE_ROTATION_ELEMENTS is defined.  This can
	// bring a ~3x speed improvement for this function.
#define	CACHE_ROTATION_ELEMENTS
#ifdef CACHE_ROTATION_ELEMENTS
	float m00 = m_rotationMatrix.at<float>(0, 0);
	float m01 = m_rotationMatrix.at<float>(0, 1);
	float m02 = m_rotationMatrix.at<float>(0, 2);
	float m10 = m_rotationMatrix.at<float>(1, 0);
	float m11 = m_rotationMatrix.at<float>(1, 1);
	float m12 = m_rotationMatrix.at<float>(1, 2);
	float m20 = m_rotationMatrix.at<float>(2, 0);
	float m21 = m_rotationMatrix.at<float>(2, 1);
	float m22 = m_rotationMatrix.at<float>(2, 2);
#endif

	switch (m_storageOrder)
	{
	case STORE_ROW_COL:
	{

		for (int y = 0; y < m_parameters->m_heightOutput; y++)
		{
			for (int x = 0; x < m_parameters->m_widthOutput; x++)
			{
#ifdef CACHE_ROTATION_ELEMENTS
				// Calculate xyz * R
				float eX = pXYZElement->m_x;
				float eY = pXYZElement->m_y;
				float eZ = pXYZElement->m_z;

				pXYZElement->m_x = eX * m00 + eY * m01 + eZ * m02;
				pXYZElement->m_y = eX * m10 + eY * m11 + eZ * m12;
				pXYZElement->m_z = eX * m20 + eY * m21 + eZ * m22;

				norm = sqrt(pXYZElement->m_x * pXYZElement->m_x + pXYZElement->m_y * pXYZElement->m_y + pXYZElement->m_z * pXYZElement->m_z);

				pLonLatElement->m_x = atan2(pXYZElement->m_x / norm, pXYZElement->m_z / norm);
				pLonLatElement->m_y = asin(pXYZElement->m_y / norm);

				pXYZElement++;
				pLonLatElement++;
#else
				// Calculate xyz * R
				float eX = pXYZElement->m_x;
				float eY = pXYZElement->m_y;
				float eZ = pXYZElement->m_z;

				pXYZElement->m_x = eX * m_rotationMatrix.at<float>(0, 0) + eY * m_rotationMatrix.at<float>(0, 1) + eZ * m_rotationMatrix.at<float>(0, 2);
				pXYZElement->m_y = eX * m_rotationMatrix.at<float>(1, 0) + eY * m_rotationMatrix.at<float>(1, 1) + eZ * m_rotationMatrix.at<float>(1, 2);
				pXYZElement->m_z = eX * m_rotationMatrix.at<float>(2, 0) + eY * m_rotationMatrix.at<float>(2, 1) + eZ * m_rotationMatrix.at<float>(2, 2);

				norm = sqrt(pXYZElement->m_x * pXYZElement->m_x + pXYZElement->m_y * pXYZElement->m_y + pXYZElement->m_z * pXYZElement->m_z);

				pLonLatElement->m_x = atan2(pXYZElement->m_x / norm, pXYZElement->m_z / norm);
				pLonLatElement->m_y = asin(pXYZElement->m_y / norm);

				pXYZElement++;
				pLonLatElement++;
#endif

			}
		}
//#define	CONFIRMATION_PRINTS
#ifdef CONFIRMATION_PRINTS
		printf("ConvertXYZToLonLat:\n");
		pXYZElement = &m_pXYZPoints[0];
		Point3D* pXYZRow = &pXYZElement[0];
		printf("Row, Col Point[0, 0] = [%f, %f, %f]\n", pXYZRow[0].m_x, pXYZRow[0].m_y, pXYZRow[0].m_z);
		printf("Point[1, 0] = [%f, %f, %f]\n", pXYZRow[1].m_x, pXYZRow[1].m_y, pXYZRow[1].m_z);
		pXYZRow = &pXYZElement[m_parameters->m_widthOutput];
		printf("Point[0, 1] = [%f, %f, %f]\n", pXYZRow[0].m_x, pXYZRow[0].m_y, pXYZRow[0].m_z);

		pLonLatElement = &m_pLonLatPoints[0];
		Point2D* pRow = &m_pLonLatPoints[0];
		printf("Row, Col Point[0, 0] = [%f, %f]\n", pRow[0].m_x, pRow[0].m_y);
		printf("Point[1, 0] = [%f, %f]\n", pRow[1].m_x, pRow[1].m_y);
		pRow = &m_pLonLatPoints[m_parameters->m_widthOutput];
		printf("Point[0, 1] = [%f, %f]\n", pRow[0].m_x, pRow[0].m_y);
#endif
		break;
	}
	case STORE_COL_ROW:
	{
		for (int x = 0; x < m_parameters->m_widthOutput; x++)
		{
			for (int y = 0; y < m_parameters->m_heightOutput; y++)
			{
#ifdef CACHE_ROTATION_ELEMENTS
				// Calculate xyz * R
				float eX = pXYZElement->m_x;
				float eY = pXYZElement->m_y;
				float eZ = pXYZElement->m_z;

				pXYZElement->m_x = eX * m00 + eY * m01 + eZ * m02;
				pXYZElement->m_y = eX * m10 + eY * m11 + eZ * m12;
				pXYZElement->m_z = eX * m20 + eY * m21 + eZ * m22;

				norm = sqrt(pXYZElement->m_x * pXYZElement->m_x + pXYZElement->m_y * pXYZElement->m_y + pXYZElement->m_z * pXYZElement->m_z);

				pLonLatElement->m_x = atan2(pXYZElement->m_x / norm, pXYZElement->m_z / norm);
				pLonLatElement->m_y = asin(pXYZElement->m_y / norm);

				pXYZElement++;
				pLonLatElement++;
#else
				// Calculate xyz * R
				float eX = pXYZElement->m_x;
				float eY = pXYZElement->m_y;
				float eZ = pXYZElement->m_z;

				pXYZElement->m_x = eX * m_rotationMatrix.at<float>(0, 0) + eY * m_rotationMatrix.at<float>(1, 0) + eZ * m_rotationMatrix.at<float>(2, 0);
				pXYZElement->m_y = eX * m_rotationMatrix.at<float>(0, 1) + eY * m_rotationMatrix.at<float>(1, 1) + eZ * m_rotationMatrix.at<float>(2, 1);
				pXYZElement->m_z = eX * m_rotationMatrix.at<float>(0, 2) + eY * m_rotationMatrix.at<float>(1, 2) + eZ * m_rotationMatrix.at<float>(2, 2);

				norm = sqrt(pXYZElement->m_x * pXYZElement->m_x + pXYZElement->m_y * pXYZElement->m_y + pXYZElement->m_z * pXYZElement->m_z);

				pLonLatElement->m_x = atan2(pXYZElement->m_x / norm, pXYZElement->m_z / norm);
				pLonLatElement->m_y = asin(pXYZElement->m_y / norm);

				pXYZElement++;
				pLonLatElement++;
#endif
			}
		}

#ifdef CONFIRMATION_PRINTS
		printf("ConvertXYZToLonLat:\n");
		pXYZElement = &m_pXYZPoints[0];
		Point3D* pXYZCol = &pXYZElement[0];
		printf("Row, Col Point[0, 0] = [%f, %f, %f]\n", pXYZCol[0].m_x, pXYZCol[0].m_y, pXYZCol[0].m_z);
		printf("Point[0, 1] = [%f, %f, %f]\n", pXYZCol[1].m_x, pXYZCol[1].m_y, pXYZCol[1].m_z);
		pXYZCol = &pXYZElement[m_parameters->m_heightOutput];
		printf("Point[0, 1] = [%f, %f, %f]\n", pXYZCol[0].m_x, pXYZCol[0].m_y, pXYZCol[0].m_z);

		pLonLatElement = &m_pLonLatPoints[0];
		Point2D* pCol = &m_pLonLatPoints[0];
		printf("Row, Col Point[0, 0] = [%f, %f]\n", pCol[0].m_x, pCol[0].m_y);
		printf("Point[0, 1] = [%f, %f]\n", pCol[1].m_x, pCol[1].m_y);
		pCol = &m_pLonLatPoints[m_parameters->m_heightOutput];
		printf("Point[1, 0] = [%f, %f]\n", pCol[0].m_x, pCol[0].m_y);
#endif
		break;
	}
	}

}

void SerialRemapping::ComputeLonLatCoords()
{
	ComputeRotationMatrix((float)m_parameters->m_yaw * DEGREE_CONVERSION_FACTOR, (float)m_parameters->m_pitch * DEGREE_CONVERSION_FACTOR, (float)m_parameters->m_roll * DEGREE_CONVERSION_FACTOR);
	ConvertXYZToLonLat();
}

void SerialRemapping::ComputeXYCoords()
{
	// Python code snippet that this is attempting to match (shape is the width and height of the equirectangular image)
	//X = (lonlat[..., 0:1] / (2 * np.pi) + 0.5) * (shape[1] - 1)
	//Y = (lonlat[..., 1:] / (np.pi) + 0.5) * (shape[0] - 1)
	//lst = [X, Y]
	//out = np.concatenate(lst, axis = -1)
	Point2D* pLonLatElement = &m_pLonLatPoints[0];
	Point2D* pXYElement = &m_pXYPoints[0];
	float xDiv = 2 * M_PI;
	float width = m_parameters->m_image[m_parameters->m_imageIndex].cols - 1;
	float height = m_parameters->m_image[m_parameters->m_imageIndex].rows - 1;;

	switch (m_storageOrder)
	{
	case STORE_ROW_COL:
	{
		for (int y = 0; y < m_parameters->m_heightOutput; y++)
		{
			for (int x = 0; x < m_parameters->m_widthOutput; x++)
			{

				pXYElement->m_x = (pLonLatElement->m_x / xDiv + 0.5) * width;
				pXYElement->m_y = (pLonLatElement->m_y / M_PI + 0.5) * height;

				pLonLatElement++;
				pXYElement++;
			}
		}
//#define	CONFIRMATION_PRINTS
#ifdef CONFIRMATION_PRINTS
		printf("ComputeXYCoords:\n");
		pXYElement = &m_pXYPoints[0];
		Point2D* pRow = &pXYElement[0];
		printf("Row, Col Point[0, 0] = [%f, %f]\n", pRow[0].m_x, pRow[0].m_y);
		printf("Point[1, 0] = [%f, %f]\n", pRow[1].m_x, pRow[1].m_y);
		pRow = &pXYElement[m_parameters->m_widthOutput];
		printf("Point[0, 1] = [%f, %f]\n", pRow[0].m_x, pRow[0].m_y);
#endif
		break;
	}
	case STORE_COL_ROW:
	{
		for (int x = 0; x < m_parameters->m_widthOutput; x++)
		{
			for (int y = 0; y < m_parameters->m_heightOutput; y++)
			{
				pXYElement->m_x = (pLonLatElement->m_x / xDiv + 0.5) * width;
				pXYElement->m_y = (pLonLatElement->m_y / M_PI + 0.5) * height;

				pLonLatElement++;
				pXYElement++;
			}
		}
#ifdef CONFIRMATION_PRINTS
		printf("ComputeXYCoords:\n");
		pXYElement = &m_pXYPoints[0];
		Point2D* pCol = &pXYElement[0];
		printf("Col Row Point[0, 0] = [%f, %f]\n", pCol[0].m_x, pCol[0].m_y);
		printf("Point[0, 1] = [%f, %f]\n", pCol[1].m_x, pCol[1].m_y);
		pCol = &pXYElement[m_parameters->m_heightOutput];
		printf("Point[1, 0] = [%f, %f]\n", pCol[0].m_x, pCol[0].m_y);
#endif

		break;
	}
	}

}

void SerialRemapping::FrameCalculations(bool bParametersChanged)
{
	if (bParametersChanged || m_bFrameCalcRequired)
	{
#ifdef VTUNE_API
		__itt_task_begin(pittTests_domain, __itt_null, __itt_null, handle_SerialRemapping_calc_kernel);
#endif

		BaseAlgorithm::FrameCalculations(bParametersChanged);

		std::chrono::system_clock::time_point startTime;

		startTime = std::chrono::system_clock::now();
		ComputeXYZCoords();
		TimingStats::GetTimingStats()->AddIterationResults(ETimingType::TIMING_CREATE_XYZ_COORDS, startTime, std::chrono::system_clock::now());

		startTime = std::chrono::system_clock::now();
		ComputeLonLatCoords();
		TimingStats::GetTimingStats()->AddIterationResults(ETimingType::TIMING_CREATE_LON_LAT_COORDS, startTime, std::chrono::system_clock::now());

		startTime = std::chrono::system_clock::now();
		ComputeXYCoords();
		TimingStats::GetTimingStats()->AddIterationResults(ETimingType::TIMING_CREATE_XY_COORDS, startTime, std::chrono::system_clock::now());
		m_bFrameCalcRequired = false;
#ifdef VTUNE_API
		__itt_task_end(pittTests_domain);
#endif

	}
}

cv::Mat SerialRemapping::ExtractFrameImage()
{
#ifdef VTUNE_API
	__itt_task_begin(pittTests_domain, __itt_null, __itt_null, handle_SerialRemapping_extract_kernel);
#endif

	cv::Mat retVal;
	std::chrono::system_clock::time_point startTime;

	startTime = std::chrono::system_clock::now();

	switch (m_storageOrder)
	{
	case STORE_ROW_COL:
	{
		cv::Mat map = cv::Mat(m_parameters->m_heightOutput, m_parameters->m_widthOutput, CV_32FC2, m_pXYPoints);

		cv::remap(m_parameters->m_image[m_parameters->m_imageIndex], retVal, map, cv::Mat{}, cv::INTER_CUBIC, cv::BORDER_WRAP);

		TimingStats::GetTimingStats()->AddIterationResults(ETimingType::TIMING_REMAP, startTime, std::chrono::system_clock::now());

		break;
	}
	case STORE_COL_ROW:
	{
		// The image is in row/col format, but we have stored data in col/row format, so need to be careful here and
		// transpose the data
		Point2D* pXYElement = &m_pXYPoints[0];
		float* m_pX = new float[m_parameters->m_widthOutput * m_parameters->m_heightOutput];
		float* m_pY = new float[m_parameters->m_widthOutput * m_parameters->m_heightOutput];
		float* pXElement = &m_pX[0];
		float* pYElement = &m_pY[0];

		for (int y = 0; y < m_parameters->m_heightOutput; y++)
		{
			pXYElement = &m_pXYPoints[y];
			for (int x = 0; x < m_parameters->m_widthOutput; x++)
			{
				*pXElement = pXYElement->m_x;
				*pYElement = pXYElement->m_y;

				pXYElement += m_parameters->m_heightOutput;
				pYElement++;
				pXElement++;
			}
		}
		cv::Mat mapX = cv::Mat(m_parameters->m_heightOutput, m_parameters->m_widthOutput, CV_32FC1, m_pX);
		cv::Mat mapY = cv::Mat(m_parameters->m_heightOutput, m_parameters->m_widthOutput, CV_32FC1, m_pY);

		TimingStats::GetTimingStats()->AddIterationResults(ETimingType::TIMING_CREATE_MAP, startTime, std::chrono::system_clock::now());

		startTime = std::chrono::system_clock::now();
		cv::remap(m_parameters->m_image[m_parameters->m_imageIndex], retVal, mapX, mapY, cv::INTER_CUBIC, cv::BORDER_WRAP);
		delete[] m_pX;
		delete[] m_pY;
		TimingStats::GetTimingStats()->AddIterationResults(ETimingType::TIMING_REMAP, startTime, std::chrono::system_clock::now());
		break;
	}
	}

#ifdef VTUNE_API
	__itt_task_end(pittTests_domain);
#endif

	return retVal;
}

cv::Mat SerialRemapping::GetDebugImage()
{
	cv::Mat retVal;
	Point2D* pXYElement = &m_pXYPoints[0];

	m_parameters->m_image[m_parameters->m_imageIndex].copyTo(retVal);

	switch (m_storageOrder)
	{
	case STORE_ROW_COL:
	{
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
	case STORE_COL_ROW:
	{
		// Draw the points on the left of the viewing area in red
		for (int y = 0; y < m_parameters->m_heightOutput; y++)
		{
			cv::Point ptLeft = cv::Point(pXYElement->m_x, pXYElement->m_y);
			cv::line(retVal, ptLeft, ptLeft, cv::Scalar(0, 0, 255), 10);

			pXYElement++;
		}

		// Draw the top (blue) and bottom (tan) sides of the viewing region
		for (int x = 1; x < m_parameters->m_widthOutput - 2; x++)
		{
			cv::Point ptTop = cv::Point(pXYElement->m_x, pXYElement->m_y);
			cv::line(retVal, ptTop, ptTop, cv::Scalar(255, 0, 0), 10);

			pXYElement += m_parameters->m_heightOutput - 1;

			cv::Point ptBottom = cv::Point(pXYElement->m_x, pXYElement->m_y);
			cv::line(retVal, ptBottom, ptBottom, cv::Scalar(74, 136, 175), 10);

			pXYElement++;
		}

		// Draw the points on the rigth of the viewing area in green
		for (int y = 0; y < m_parameters->m_heightOutput; y++)
		{
			cv::Point pt = cv::Point(pXYElement->m_x, pXYElement->m_y);
			cv::line(retVal, pt, pt, cv::Scalar(0, 255, 0), 10);

			pXYElement++;
		}
		break;
	}
	}



	//if bDebug:
	//# When debugging, we want to show the original image with an outline of the Region of Interest
	//	# The color is BGR
	//	debugImg = img[imgIndex].copy()
	//	(height, width) = XY.shape[:2]
	//	print("XY shape", XY.shape[:2])
	//	for pt in XY[0]:
	//cv2.line(debugImg, (int(pt[0]), int(pt[1])), (int(pt[0]), int(pt[1])), (255, 0, 0), 10)
	//	for y in range(1, height - 2) :
	//		cv2.line(debugImg, (int(XY[y][0][0]), int(XY[y][0][1])), (int(XY[y][0][0]), int(XY[y][0][1])), (0, 0, 255), 10)
	//		cv2.line(debugImg, (int(XY[y][width - 1][0]), int(XY[y][width - 1][1])), (int(XY[y][width - 1][0]), int(XY[y][width - 1][1])), (0, 255, 0), 10)
	//		for pt in XY[height - 1] :
	//			cv2.line(debugImg, (int(pt[0]), int(pt[1])), (int(pt[0]), int(pt[1])), (74, 136, 175), 10)


	//			#debugImg = cv2.resize(debugImg, (1080, 540))
	//			cv2.namedWindow("Debug View", cv2.WINDOW_NORMAL)
	//			cv2.imshow("Debug View", debugImg)


	return retVal;
}

bool SerialRemapping::StartVariant()
{
	BaseAlgorithm::StartVariant();

	bool bRetVal = false;

	m_storageOrder++;

	if (m_storageOrder < STORE_MAX)
	{
		int size = m_parameters->m_widthOutput * m_parameters->m_heightOutput;

		// Reserve the space for the 3D points
		if (m_storageOrder == STORE_ROW_COL || m_storageOrder == STORE_COL_ROW)
		{
			m_pXYZPoints = new Point3D[size];
			m_pXYPoints = new Point2D[size];
			m_pLonLatPoints = new Point2D[size];
		}
		//else if (m_storageOrder == STORE_SOA)
		//{
		//	m_pXYZPoints = NULL;
		//	m_SoAXYZPoints.m_pX = new float[size];
		//	m_SoAXYZPoints.m_pY = new float[size];
		//	m_SoAXYZPoints.m_pZ = new float[size];
		//}
		bRetVal = true;
		m_bFrameCalcRequired = true;
	}

	return bRetVal;
}

void SerialRemapping::StopVariant()
{
	delete m_pXYPoints;
	m_pXYPoints = NULL;
}

