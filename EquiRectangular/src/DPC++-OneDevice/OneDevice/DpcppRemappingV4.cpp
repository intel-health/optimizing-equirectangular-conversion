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

// This code investigates the impact of changing V2 to use the sub-groups to access the data so they are not
// scattered when writing.  This is not significantly better than the V2 and might be slightly worse.  It does
// seem to have the lowest stall amount of the V2, V3, or V4 variations but still > 30%.  On the other hand,
// the overall elapsed time and the GPU time are the highest for this version.

#include "DpcppRemappingV4.hpp"
#include "ConfigurableDeviceSelector.hpp"
#include "TimingStats.hpp"
#include <opencv2/calib3d.hpp>

#ifdef VTUNE_API
#include "ittnotify.h"
#pragma comment(lib, "libittnotify.lib")
extern __itt_domain* pittTests_domain;
// Create string handle for denoting when the kernel is running
wchar_t const *pDpcppRemappingV4Extract = _T("DpcppRemappingV4 Extract Kernel");
__itt_string_handle *handle_DpcppRemappingV4_extract_kernel = __itt_string_handle_create(pDpcppRemappingV4Extract);
wchar_t const *pDpcppRemappingV4Calc = _T("DpcppRemappingV4 Calc Kernel");
__itt_string_handle *handle_DpcppRemappingV4_calc_kernel = __itt_string_handle_create(pDpcppRemappingV4Calc);
#endif

DpcppRemappingV4::DpcppRemappingV4(SParameters& parameters) : DpcppBaseAlgorithm(parameters)
{
	m_storageType = STORAGE_TYPE_INIT;
}

std::string DpcppRemappingV4::GetDescription()
{
    std::string strDesc = GetDeviceDescription();

	switch (m_storageType)
	{
	case STORAGE_TYPE_USM:
		return "DpcppRemappingV4: Computes a Remapping algorithm using oneAPI's DPC++ sub-groups to reduce scatter with Universal Shared Memory on " + strDesc;
		break;
	case STORAGE_TYPE_DEVICE:
		return "DpcppRemappingV4: Computes a Remapping algorithm using oneAPI's DPC++ sub-groups to reduce scatter with  Device Memory on " + strDesc;
		break;
	}

	return "Unknown";
}

void DpcppRemappingV4::FrameCalculations(bool bParametersChanged)
{
	if (bParametersChanged || m_bFrameCalcRequired)
	{
#ifdef VTUNE_API
		__itt_task_begin(pittTests_domain, __itt_null, __itt_null, handle_DpcppRemappingV4_calc_kernel);
#endif

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
		// Width must be a multiple of numPerLoop or things don't work well.
		const int numPerLoop = 16;

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
			Point2D *pPoints = m_pXYPoints;

			m_pQ->submit([&](sycl::handler &cgh) {
				cgh.parallel_for(sycl::nd_range<1>(sycl::range<1>{height * width / numPerLoop}, sycl::range<1>{ 1 }),
				[=](sycl::nd_item<1> item) {
					int i = item.get_global_linear_id();
					auto sg = item.get_sub_group();
					int sgSize = sg.get_local_range()[0];
					i = (i / sgSize) * sgSize * numPerLoop + (i % sgSize);
					int iY = i / width;
					int baseX = i - iY * width;
					for (int j = 0; j < sgSize * numPerLoop; j += sgSize) {
						int iX = baseX + j;
						Point2D *pElement = &pPoints[i + j];
						float x = iX * invf + translatecx;
						float y = iY * invf + translatecy;
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
					}
				});
			});
			m_pQ->wait();

			break;
		}
		case STORAGE_TYPE_DEVICE:
		{
			Point2D *pDevPoints = m_pDevXYPoints;

			m_pQ->submit([&](sycl::handler &cgh) {
				cgh.parallel_for(sycl::nd_range<1>(sycl::range<1>{height * width / numPerLoop}, sycl::range<1>{ 1 }),
				[=](sycl::nd_item<1> item) {
					int i = item.get_global_linear_id();
					auto sg = item.get_sub_group();
					int sgSize = sg.get_local_range()[0];
					i = (i / sgSize) * sgSize * numPerLoop + (i % sgSize);
					int iY = i / width;
					int baseX = i - iY * width;
					for (int j = 0; j < sgSize * numPerLoop; j += sgSize) {
						int iX = baseX + j;
						Point2D *pElement = &pDevPoints[i + j];

						float x = iX * invf + translatecx;
						float y = iY * invf + translatecy;
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
					}
				});
			});
			m_pQ->wait();

			// Now copy the data back to the host
			m_pQ->submit([&](sycl::handler &cgh) {
				// copy deviceArray back to hostArray
				cgh.memcpy(&m_pXYPoints[0], pDevPoints, height * width * sizeof(Point2D));
				});
			m_pQ->wait();

			break;
		}
		}
		m_bFrameCalcRequired = false;
#ifdef VTUNE_API
		__itt_task_end(pittTests_domain);
#endif

	}

}

cv::Mat DpcppRemappingV4::ExtractFrameImage()
{
#ifdef VTUNE_API
	__itt_task_begin(pittTests_domain, __itt_null, __itt_null, handle_DpcppRemappingV4_extract_kernel);
#endif

	std::chrono::high_resolution_clock::time_point startTime	= std::chrono::high_resolution_clock::now();
	cv::Mat retVal;

	switch (m_storageType)
	{
	case STORAGE_TYPE_USM:
	{
		cv::Mat map = cv::Mat(m_parameters->m_heightOutput, m_parameters->m_widthOutput, CV_32FC2, m_pXYPoints);

		cv::remap(m_parameters->m_image[m_parameters->m_imageIndex], retVal, map, cv::Mat{}, cv::INTER_CUBIC, cv::BORDER_WRAP);

		TimingStats::GetTimingStats()->AddIterationResults(ETimingType::TIMING_REMAP, startTime, std::chrono::high_resolution_clock::now());
		break;
	}
	case STORAGE_TYPE_DEVICE:
	{
		cv::Mat map = cv::Mat(m_parameters->m_heightOutput, m_parameters->m_widthOutput, CV_32FC2, m_pXYPoints);

		cv::remap(m_parameters->m_image[m_parameters->m_imageIndex], retVal, map, cv::Mat{}, cv::INTER_CUBIC, cv::BORDER_WRAP);

		TimingStats::GetTimingStats()->AddIterationResults(ETimingType::TIMING_REMAP, startTime, std::chrono::high_resolution_clock::now());
		break;
	}
	}
#ifdef VTUNE_API
	__itt_task_end(pittTests_domain);
#endif

	return retVal;
}

// Pass in theta, phi, and psi in radians, not degrees
void DpcppRemappingV4::ComputeRotationMatrix(float radTheta, float radPhi, float radPsi)
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

bool DpcppRemappingV4::StartVariant()
{
	bool bRetVal = false;

	if (m_storageType == STORAGE_TYPE_INIT)
	{
		DpcppBaseAlgorithm::StartVariant();
	}

	m_storageType++;
	if (m_pQ && m_storageType < STORAGE_TYPE_MAX)
	{
		int size = m_parameters->m_widthOutput * m_parameters->m_heightOutput;

		switch (m_storageType)
		{
		case STORAGE_TYPE_USM:
		{
			auto dev = m_pQ->get_device();
			auto ctxt = m_pQ->get_context();

			m_pXYPoints = (Point2D*)malloc_shared(size * sizeof(Point2D), dev, ctxt);

			printf("DpcppRemappingV4::StartVariant STORAGE_TYPE_USM\n");
			break;
		}
		case STORAGE_TYPE_DEVICE:
			m_pXYPoints = (Point2D*)malloc(size * sizeof(Point2D));
			m_pDevXYPoints = (Point2D*)malloc_device(size * sizeof(Point2D), m_pQ->get_device(), m_pQ->get_context());

			printf("DpcppRemappingV4::StartVariant STORAGE_TYPE_DEVICE\n");
			break;
		}
		m_bFrameCalcRequired = true;
		bRetVal = true;
	}

	return bRetVal;
}

void DpcppRemappingV4::StopVariant()
{
	switch (m_storageType)
	{
	case STORAGE_TYPE_USM:
	{
		auto ctxt = m_pQ->get_context();

		free(m_pXYPoints, ctxt);
		m_pXYPoints = NULL;

		break;
	}
	case STORAGE_TYPE_DEVICE:
		delete m_pXYPoints;
		m_pXYPoints = NULL;
		free(m_pDevXYPoints, m_pQ->get_context());
		m_pDevXYPoints = NULL;
		break;
	}

	if (m_storageType == STORAGE_TYPE_MAX - 1)
	{
		DpcppBaseAlgorithm::StopVariant();
		m_storageType = STORAGE_TYPE_INIT;
	}
	DpcppBaseAlgorithm::StopVariant();
}
