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

// The code here is focused on optimizing the FrameCalculation area.  This is done with the parallel_for_work_group
// call.  This seems to be worse that V2 perhaps due to the barriers that are imposed by the parallel_for_work_group call.
// According to VTune the thread occupancy was better and the FPU utilization was better, but the overall time taken
// was worse for the FrameCalculations.

#include "DpcppRemappingV3.hpp"
#include "ConfigurableDeviceSelector.hpp"
#include "TimingStats.hpp"
#include <opencv2/calib3d.hpp>

#ifdef VTUNE_API
#include "ittnotify.h"
#pragma comment(lib, "libittnotify.lib")
extern __itt_domain* pittTests_domain;
// Create string handle for denoting when the kernel is running
wchar_t const *pDpcppRemappingV3Extract = _T("DpcppRemappingV2 Extract Kernel");
__itt_string_handle *handle_DpcppRemappingV3_extract_kernel = __itt_string_handle_create(pDpcppRemappingV3Extract);
wchar_t const *pDpcppRemappingV3Calc = _T("DpcppRemappingV3 Calc Kernel");
__itt_string_handle *handle_DpcppRemappingV3_calc_kernel = __itt_string_handle_create(pDpcppRemappingV3Calc);
#endif

DpcppRemappingV3::DpcppRemappingV3(SParameters& parameters) : DpcppBaseAlgorithm(parameters)
{
	m_storageType = STORAGE_TYPE_INIT;
}

std::string DpcppRemappingV3::GetDescription()
{
    std::string strDesc = GetDeviceDescription();

	switch (m_storageType)
	{
	case STORAGE_TYPE_USM:
		return "DpcppRemappingV3: Computes a Remapping algorithm using oneAPI's DPC++ parallel_for_work_group & Universal Shared Memory on " + strDesc;
		break;
	case STORAGE_TYPE_DEVICE:
		return "DpcppRemappingV3: Computes a Remapping algorithm using oneAPI's DPC++ parallel_for_work_group & Device Memory on " + strDesc;
		break;
	}

	return "Unknown";
}

void DpcppRemappingV3::FrameCalculations(bool bParametersChanged)
{
	if (bParametersChanged || m_bFrameCalcRequired)
	{
#ifdef VTUNE_API
		__itt_task_begin(pittTests_domain, __itt_null, __itt_null, handle_DpcppRemappingV3_calc_kernel);
#endif

		BaseAlgorithm::FrameCalculations(bParametersChanged);

		ComputeRotationMatrix((float)m_parameters->m_yaw * DEGREE_CONVERSION_FACTOR, (float)m_parameters->m_pitch * DEGREE_CONVERSION_FACTOR, (float)m_parameters->m_roll * DEGREE_CONVERSION_FACTOR);

		const int height = m_parameters->m_heightOutput;
		const int width = m_parameters->m_widthOutput;
		Point2D* pPoints = m_pXYPoints;
		Point2D* pDevPoints = m_pDevXYPoints;
		const float m00 = m_rotationMatrix.at<float>(0, 0);
		const float m01 = m_rotationMatrix.at<float>(0, 1);
		const float m02 = m_rotationMatrix.at<float>(0, 2);
		const float m10 = m_rotationMatrix.at<float>(1, 0);
		const float m11 = m_rotationMatrix.at<float>(1, 1);
		const float m12 = m_rotationMatrix.at<float>(1, 2);
		const float m20 = m_rotationMatrix.at<float>(2, 0);
		const float m21 = m_rotationMatrix.at<float>(2, 1);
		const float m22 = m_rotationMatrix.at<float>(2, 2);
		const float imageWidth = m_parameters->m_image[m_parameters->m_imageIndex].cols - 1;
		const float imageHeight = m_parameters->m_image[m_parameters->m_imageIndex].rows - 1;;
		const float xDiv = 2 * M_PI;
		const int numWorkItems = 16;
		const float f = 0.5 * m_parameters->m_widthOutput * 1 / tan(0.5 * m_parameters->m_fov / 180.0 * M_PI);
		const float cx = ((float)m_parameters->m_widthOutput - 1.0f) / 2.0f;
		const float cy = ((float)m_parameters->m_heightOutput - 1.0f) / 2.0f;

		// In concept, the intent of this section of the code is to create an inverse of the intrinsic matrix K
		// (see https://ksimek.github.io/2013/08/13/intrinsic for explanation).  However, in this case we will
		// not represent as an actual matrix, but just calculate the individual values and then use them
		// to initialze the 2D array of XYZ points.
		// Initialize the inverse of the intrinsic matrix K
		//K_inv = (Mat_<float>(3, 3) <<
		//	1 / f, 0, -cx / f,
		//	0, 1 / f, -cy / f,
		//	0, 0, 1);
		const float invf = 1.0f / f;
		const float translatecx = -cx * invf;
		const float translatecy = -cy * invf;

		switch (m_storageType)
		{
		case STORAGE_TYPE_USM:
		{
			m_pQ->submit([&](sycl::handler &cgh) {
				cgh.parallel_for_work_group(sycl::range<2>(height, width / numWorkItems), sycl::range<2>(1, numWorkItems), [=](sycl::group<2> g) {
					float wgm00 = m00;
					float wgm01 = m01;
					float wgm02 = m02;
					float wgm10 = m10;
					float wgm11 = m11;
					float wgm12 = m12;
					float wgm20 = m20;
					float wgm21 = m21;
					float wgm22 = m22;

					g.parallel_for_work_item([&](sycl::h_item<2> item) {
						int iX = item.get_global_id(1);
						int iY = item.get_global_id(0);
						Point2D *pElement = &pPoints[iY * width + iX];
						float x = iX * invf + translatecx;
						float y = iY * invf + translatecy;
						float z = 1.0f;
						float norm;

						// Calculate xyz * R, save the initial x, y, and z values for the computation
						float eX = x;
						float eY = y;
						float eZ = z;

						x = eX * wgm00 + eY * wgm01 + eZ * wgm02;
						y = eX * wgm10 + eY * wgm11 + eZ * wgm12;
						z = eX * wgm20 + eY * wgm21 + eZ * wgm22;

						norm = sqrt(x * x + y * y + z * z);

						x = atan2(x / norm, z / norm);
						y = asin(y / norm);

						pElement->m_x = (x / xDiv + 0.5) * imageWidth;
						pElement->m_y = (y / M_PI + 0.5) * imageHeight;

					});
				});
			}).wait();
			break;
		}
		case STORAGE_TYPE_DEVICE:
		{
			m_pQ->submit([&](sycl::handler &cgh) {
				cgh.parallel_for_work_group(sycl::range<2>(height, width / numWorkItems), sycl::range<2>(1, numWorkItems), [=](sycl::group<2> g) {
					g.parallel_for_work_item([&](sycl::h_item<2> item) {
						int iX = item.get_global_id(1);
						int iY = item.get_global_id(0);
						Point2D* pElement = &pDevPoints[iY * width + iX];
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
					});
				});
			}).wait();

			// Now copy the data back to the host
			m_pQ->submit([&](sycl::handler& cgh) {
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

cv::Mat DpcppRemappingV3::ExtractFrameImage()
{
#ifdef VTUNE_API
	__itt_task_begin(pittTests_domain, __itt_null, __itt_null, handle_DpcppRemappingV3_extract_kernel);
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
void DpcppRemappingV3::ComputeRotationMatrix(float radTheta, float radPhi, float radPsi)
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

bool DpcppRemappingV3::StartVariant()
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

			printf("DpcppRemappingV3::StartVariant STORAGE_TYPE_USM\n");

			break;
		}
		case STORAGE_TYPE_DEVICE:
			m_pXYPoints = (Point2D*)malloc(size * sizeof(Point2D));
			m_pDevXYPoints = (Point2D*)malloc_device(size * sizeof(Point2D), m_pQ->get_device(), m_pQ->get_context());

			printf("DpcppRemappingV3::StartVariant STORAGE_TYPE_DEVICE\n");

			break;
		}
		m_bFrameCalcRequired = true;
		bRetVal = true;
	}

	return bRetVal;
}

void DpcppRemappingV3::StopVariant()
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
