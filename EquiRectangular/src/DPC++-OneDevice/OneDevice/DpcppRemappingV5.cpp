// Copyright (C) 2023 Intel Corporation
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http ://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// Author: Douglas P. Bogia

// This code starts from V2 and attempts to see if there are speed ups availabe in the ExtractFrame area

#include "DpcppRemappingV5.hpp"
#include "ConfigurableDeviceSelector.hpp"
#include "TimingStats.hpp"
#include <opencv2/calib3d.hpp>

#ifdef VTUNE_API
#include "ittnotify.h"
#pragma comment(lib, "libittnotify.lib")
extern __itt_domain* pittTests_domain;
// Create string handle for denoting when the kernel is running
wchar_t const *pDpcppRemappingV5Extract = _T("DpcppRemappingV5 Extract Kernel");
__itt_string_handle* handle_DpcppRemappingV5_extract_kernel = __itt_string_handle_create(pDpcppRemappingV5Extract);
wchar_t const *pDpcppRemappingV5Calc = _T("DpcppRemappingV5 Calc Kernel");
__itt_string_handle *handle_DpcppRemappingV5_calc_kernel = __itt_string_handle_create(pDpcppRemappingV5Calc);
#endif

const int pixelBytes = 3;

DpcppRemappingV5::DpcppRemappingV5(SParameters& parameters) : DpcppBaseAlgorithm(parameters)
{
	m_storageType = STORAGE_TYPE_INIT;
}

std::string DpcppRemappingV5::GetDescription()
{
    std::string strDesc = GetDeviceDescription();

	switch (m_storageType)
	{
	case STORAGE_TYPE_USM:
		return "DpcppRemappingV5: DpcppRemappingV2 and optimized ExtractFrame using DPC++ and USM " + strDesc;
		break;
	case STORAGE_TYPE_DEVICE:
		return "DpcppRemappingV5: DpcppRemappingV2 and optimized ExtractFrame using DPC++ and Device Memory on " + strDesc;
		break;
	}

	return "Unknown";
}

void DpcppRemappingV5::FrameCalculations(bool bParametersChanged)
{
	if (bParametersChanged || m_bFrameCalcRequired)
	{
		BaseAlgorithm::FrameCalculations(bParametersChanged);

		ComputeRotationMatrix((float)m_parameters->m_yaw * DEGREE_CONVERSION_FACTOR, (float)m_parameters->m_pitch * DEGREE_CONVERSION_FACTOR, (float)m_parameters->m_roll * DEGREE_CONVERSION_FACTOR);

		float f;
		float cx;
		float cy;
		float invf;
		float translatecx;
		float translatecy;
		int height = m_parameters->m_heightOutput;
		int width = m_parameters->m_widthOutput;
		Point2D* pPoints = m_pXYPoints;
		Point2D* pDevPoints = m_pDevXYPoints;
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
		case STORAGE_TYPE_USM:
		{
#ifdef VTUNE_API
			__itt_task_begin(pittTests_domain, __itt_null, __itt_null, handle_DpcppRemappingV5_calc_kernel);
#endif
			m_pQ->submit([&](sycl::handler& cgh) {
				cgh.parallel_for(sycl::range<2>(height, width),
				[=](sycl::id<2> item) {
					Point2D* pElement = &pPoints[item[0] * width + item[1]];
					float x = item[1] * invf + translatecx;
					float y = item[0] * invf + translatecy;
					float z  = 1.0f;
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

					pElement->m_x = (x / xDiv + 0.5) * imageWidth;
					pElement->m_y = (y / M_PI + 0.5) * imageHeight;
				});
			});
			m_pQ->wait();
#ifdef VTUNE_API
			__itt_task_end(pittTests_domain);
#endif

			break;
		}
		case STORAGE_TYPE_DEVICE:
		{
			m_pQ->submit([&](sycl::handler& cgh) {
				cgh.parallel_for(sycl::range<2>(height, width),
				[=](sycl::id<2> item) {
					Point2D* pElement = &pDevPoints[item[0] * width + item[1]];
					float x = item[1] * invf + translatecx;
					float y = item[0] * invf + translatecy;
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

					pElement->m_x = (x / xDiv + 0.5) * imageWidth;
					pElement->m_y = (y / M_PI + 0.5) * imageHeight;
				});
			});
			m_pQ->wait();

			break;
		}
		}
		m_bFrameCalcRequired = false;
	}

}

cv::Mat DpcppRemappingV5::ExtractFrameImage()
{
	std::chrono::high_resolution_clock::time_point startTime	= std::chrono::high_resolution_clock::now();
	cv::Mat retVal;

	switch (m_storageType)
	{
	case STORAGE_TYPE_USM:
	{
#ifdef VTUNE_API
		__itt_task_begin(pittTests_domain, __itt_null, __itt_null, handle_DpcppRemappingV5_extract_kernel);
#endif
		int height = m_parameters->m_heightOutput;
		int width = m_parameters->m_widthOutput;
		int imageHeight = m_parameters->m_image[m_parameters->m_imageIndex].rows;
		int imageWidth = m_parameters->m_image[m_parameters->m_imageIndex].cols;
		Point2D *pPoints = m_pXYPoints;
		unsigned char *pFlatImage = m_pFlatImage;

		if (m_currentIndex != m_parameters->m_imageIndex)
		{
			// TODO: This assumes that both images are the exact same size.  Perhaps should put an
			// ASSERT here to check that assumption
			memcpy(m_pFullImage, m_parameters->m_image[m_parameters->m_imageIndex].data, imageHeight * imageWidth * sizeof(unsigned char) * pixelBytes);
			m_currentIndex = m_parameters->m_imageIndex;
		}

		unsigned char *pFullImage = m_pFullImage;

		m_pQ->submit([&](sycl::handler &cgh) {
			cgh.parallel_for(sycl::range<2>(height, width),
			[=](sycl::id<2> item) {
				int offset = item[0] * width + item[1];
				Point2D *pElement = &pPoints[offset];
				unsigned char *pFlatPixel = &pFlatImage[offset * pixelBytes];
				// determine the nearest top left pixel for bilinear interpolation
				int top_left_x = static_cast<int>(pElement->m_x); // convert the subpixel value to an integer pixel value (top left pixel due to int() operator)
				int top_left_y = static_cast<int>(pElement->m_y);
				// initialize weights for bilinear interpolation
				double dx = pElement->m_x - top_left_x;
				double dy = pElement->m_y - top_left_y;
				double wtl = (1.0 - dx) * (1.0 - dy);
				double wtr = dx * (1.0 - dy);
				double wbl = (1.0 - dx) * dy;
				double wbr = dx * dy;
				// Starting bytes for the four points to use for the calculation of the color for the flat image pixel.
				unsigned char *tl = pFullImage + (top_left_y * imageWidth + top_left_x) * pixelBytes;
				unsigned char *tr = tl + pixelBytes;
				unsigned char *br = tr + (imageWidth * pixelBytes);
				unsigned char *bl = br - pixelBytes;

				// There is an assumption here that the image has 3 consecutive bytes for
				// blue, green, and red (i.e., no alpha channel)
				for (int i = 0; i < pixelBytes; i++)
				{
					pFlatPixel[i] = wtl * tl[i] + wtr * tr[i] + wbl * bl[i] + wbr * br[i];
				}
			});
		}).wait();
		retVal = cv::Mat(m_parameters->m_heightOutput, m_parameters->m_widthOutput, CV_8UC3, m_pFlatImage);
		TimingStats::GetTimingStats()->AddIterationResults(ETimingType::TIMING_REMAP, startTime, std::chrono::high_resolution_clock::now());
#ifdef VTUNE_API
		__itt_task_end(pittTests_domain);
#endif

		break;
	}
	case STORAGE_TYPE_DEVICE:
	{
		int height = m_parameters->m_heightOutput;
		int width = m_parameters->m_widthOutput;
		int imageHeight = m_parameters->m_image[m_parameters->m_imageIndex].rows;
		int imageWidth = m_parameters->m_image[m_parameters->m_imageIndex].cols;
		unsigned char *pFlatImage = m_pDevFlatImage;
		unsigned char *pDevFullImage = m_pDevFullImage;
		Point2D *pDevXYPoints = m_pDevXYPoints;

		if (m_currentIndex != m_parameters->m_imageIndex)
		{
			// TODO: This assumes that both images are the exact same size.  Perhaps should put an
			// ASSERT here to check that assumption
			m_pQ->memcpy(m_pDevFullImage, m_parameters->m_image[m_parameters->m_imageIndex].data, imageHeight * imageWidth * sizeof(unsigned char) * pixelBytes);
			m_pQ->wait();
			m_currentIndex = m_parameters->m_imageIndex;
		}

		m_pQ->submit([&](sycl::handler &cgh) {
			cgh.parallel_for(sycl::range<2>(height, width),
			[=](sycl::id<2> item) {
				int offset = item[0] * width + item[1];
				Point2D *pElement = &pDevXYPoints[offset];
				unsigned char *pFlatPixel = &pFlatImage[offset * pixelBytes];
				// determine the nearest top left pixel for bilinear interpolation
				int top_left_x = static_cast<int>(pElement->m_x); // convert the subpixel value to an integer pixel value (top left pixel due to int() operator)
				int top_left_y = static_cast<int>(pElement->m_y);
				// initialize weights for bilinear interpolation
				double dx = pElement->m_x - top_left_x;
				double dy = pElement->m_y - top_left_y;
				double wtl = (1.0 - dx) * (1.0 - dy);
				double wtr = dx * (1.0 - dy);
				double wbl = (1.0 - dx) * dy;
				double wbr = dx * dy;
				// Starting bytes for the four points to use for the calculation of the color for the flat image pixel.
				unsigned char *tl = pDevFullImage + (top_left_y * imageWidth + top_left_x) * pixelBytes;
				unsigned char *tr = tl + pixelBytes;
				unsigned char *br = tr + (imageWidth * pixelBytes);
				unsigned char *bl = br - pixelBytes;

				// There is an assumption here that the image has 3 consecutive bytes for
				// blue, green, and red (i.e., no alpha channel)
				for (int i = 0; i < pixelBytes; i++)
				{
					pFlatPixel[i] = wtl * tl[i] + wtr * tr[i] + wbl * bl[i] + wbr * br[i];
				}
			});
		}).wait();
		retVal = cv::Mat(m_parameters->m_heightOutput, m_parameters->m_widthOutput, CV_8UC3);
		m_pQ->memcpy(retVal.data, m_pDevFlatImage, m_parameters->m_heightOutput * m_parameters->m_widthOutput * sizeof(unsigned char) * pixelBytes);
		m_pQ->wait();
		TimingStats::GetTimingStats()->AddIterationResults(ETimingType::TIMING_REMAP, startTime, std::chrono::high_resolution_clock::now());
		break;
	}
	}

	return retVal;
}

// Pass in theta, phi, and psi in radians, not degrees
void DpcppRemappingV5::ComputeRotationMatrix(float radTheta, float radPhi, float radPsi)
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
}

bool DpcppRemappingV5::StartVariant()
{
	bool bRetVal = false;

	m_currentIndex = -1;
	if (m_storageType == STORAGE_TYPE_INIT)
	{
		DpcppBaseAlgorithm::StartVariant();
	}
	m_storageType++;
	if (m_pQ && m_storageType < STORAGE_TYPE_MAX)
	{
		int size = m_parameters->m_widthOutput * m_parameters->m_heightOutput;
		auto dev = m_pQ->get_device();
		auto ctxt = m_pQ->get_context();

		switch (m_storageType)
		{
		case STORAGE_TYPE_USM:
		{
			m_pXYPoints = (Point2D*)malloc_shared(size * sizeof(Point2D), dev, ctxt);
			m_pFlatImage = (unsigned char *)malloc_shared(size * pixelBytes * sizeof(unsigned char), dev, ctxt);
			m_pFullImage = (unsigned char *)malloc_shared(m_parameters->m_image[m_parameters->m_imageIndex].cols * m_parameters->m_image[m_parameters->m_imageIndex].rows * pixelBytes * sizeof(unsigned char), dev, ctxt);

			printf("DpcppRemappingV5::StartVariant STORAGE_TYPE_USM\n");

			break;
		}
		case STORAGE_TYPE_DEVICE:
			m_pDevXYPoints = (Point2D*)malloc_device(size * sizeof(Point2D), dev, ctxt);
			m_pDevFlatImage = (unsigned char *)malloc_device(size * pixelBytes * sizeof(unsigned char), dev, ctxt);
			m_pDevFullImage = (unsigned char *)malloc_device(m_parameters->m_image[m_parameters->m_imageIndex].cols * m_parameters->m_image[m_parameters->m_imageIndex].rows * pixelBytes * sizeof(unsigned char), dev, ctxt);

			printf("DpcppRemappingV5::StartVariant STORAGE_TYPE_DEVICE\n");

			break;
		}
		m_bFrameCalcRequired = true;
		bRetVal = true;
	}

	return bRetVal;
}

void DpcppRemappingV5::StopVariant()
{
	switch (m_storageType)
	{
	case STORAGE_TYPE_USM:
	{
		auto ctxt = m_pQ->get_context();

		free(m_pXYPoints, ctxt);
		m_pXYPoints = NULL;
		free(m_pFlatImage, ctxt);
		m_pFlatImage = NULL;
		free(m_pFullImage, ctxt);
		m_pFullImage = NULL;

		break;
	}
	case STORAGE_TYPE_DEVICE:
		free(m_pDevXYPoints, m_pQ->get_context());
		m_pDevXYPoints = NULL;
		free(m_pDevFlatImage, m_pQ->get_context());
		m_pDevFlatImage = NULL;
		free(m_pDevFullImage, m_pQ->get_context());
		m_pDevFullImage = NULL;
		break;
	}

	if (m_storageType == STORAGE_TYPE_MAX - 1)
	{
		DpcppBaseAlgorithm::StopVariant();
		m_storageType = STORAGE_TYPE_INIT;
	}
}