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

// The code here takes inspiration from the python code at 
// https://github.com/fuenwang/Equirec2Perspec/blob/master/Equirec2Perspec.py and changes to DPC++ (SYCL)
// to test out parallel computations.

#include "DpcppRemapping.hpp"
#include "ConfigurableDeviceSelector.hpp"
#include "TimingStats.hpp"
#include <opencv2/calib3d.hpp>

#ifdef VTUNE_API
#include "ittnotify.h"
#pragma comment(lib, "libittnotify.lib")
extern __itt_domain *pittTests_domain;
// Create string handle for denoting when the kernel is running
wchar_t const *pDpcppRemappingExtract = _T("DpcppRemappingV1 Extract Kernel");
__itt_string_handle *handle_DpcppRemapping_extract_kernel = __itt_string_handle_create(pDpcppRemappingExtract);
wchar_t const *pDpcppRemappingCalc = _T("DpcppRemappingV1 Calc Kernel");
__itt_string_handle *handle_DpcppRemapping_calc_kernel = __itt_string_handle_create(pDpcppRemappingCalc);
#endif

DpcppRemapping::DpcppRemapping(SParameters& parameters) : DpcppBaseAlgorithm(parameters)
{
	m_storageType = STORAGE_TYPE_USM;
}

std::string DpcppRemapping::GetDescription()
{
    std::string strDesc = GetDeviceDescription();

	switch (m_storageType)
	{
	case STORAGE_TYPE_USM:
		return "DpcppRemapping: Computes a Remapping algorithm using oneAPI's DPC++ Universal Shared Memory on " + strDesc;
		break;
	}

	return "Unknown";
}

void DpcppRemapping::FrameCalculations(bool bParametersChanged)
{
	if (bParametersChanged || m_bFrameCalcRequired)
	{
#ifdef VTUNE_API
		__itt_task_begin(pittTests_domain, __itt_null, __itt_null, handle_DpcppRemapping_calc_kernel);
#endif
		BaseAlgorithm::FrameCalculations(bParametersChanged);

		std::chrono::high_resolution_clock::time_point startTime;

		startTime = std::chrono::high_resolution_clock::now();
		ComputeXYZCoords();
		TimingStats::GetTimingStats()->AddIterationResults(ETimingType::TIMING_CREATE_XYZ_COORDS, startTime, std::chrono::high_resolution_clock::now());

		startTime = std::chrono::high_resolution_clock::now();
		ComputeLonLatCoords();
		TimingStats::GetTimingStats()->AddIterationResults(ETimingType::TIMING_CREATE_LON_LAT_COORDS, startTime, std::chrono::high_resolution_clock::now());

		startTime = std::chrono::high_resolution_clock::now();
		ComputeXYCoords();
		TimingStats::GetTimingStats()->AddIterationResults(ETimingType::TIMING_CREATE_XY_COORDS, startTime, std::chrono::high_resolution_clock::now());
		m_bFrameCalcRequired = false;
#ifdef VTUNE_API
		__itt_task_end(pittTests_domain);
#endif
	}
}

cv::Mat DpcppRemapping::ExtractFrameImage()
{
#ifdef VTUNE_API
	__itt_task_begin(pittTests_domain, __itt_null, __itt_null, handle_DpcppRemapping_extract_kernel);
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
	}

#ifdef VTUNE_API
	__itt_task_end(pittTests_domain);
#endif

	return retVal;
}

void DpcppRemapping::ComputeXYZCoords()
{
	float f;
	float cx;
	float cy;
	float invf;
	float translatecx;
	float translatecy;
	int height = m_parameters->m_heightOutput;
	int width = m_parameters->m_widthOutput;
	Point3D *pPoints = m_pXYZPoints;

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

		m_pQ->submit([&](sycl::handler& cgh) {
			//cgh.parallel_for(sycl::range<2>(height, width),
			//[=](sycl::id<2> item) {
			//	Point3D *pElement = &pPoints[item[0] * width + item[1]];

			//	pElement->m_x = item[1] * invf + translatecx;
			//	pElement->m_y = item[0] * invf + translatecy;
			//	pElement->m_z = 1.0f;
			//});
			Point3D *pElement = &pPoints[0];
			for (int y = 0; y < height; y++)
			{
				for (int x = 0; x < width; x++)
				{
					pElement->m_x = x * invf + translatecx;
					pElement->m_y = y * invf + translatecy;
					pElement->m_z = 1.0f;
					pElement++;
				}
			}
		});
		m_pQ->wait();

		//for (int y = 0; y < m_parameters->m_heightOutput; y++)
		//{
		//	for (int x = 0; x < m_parameters->m_widthOutput; x++)
		//	{
		//		pElement->m_x = x * invf + translatecx;
		//		pElement->m_y = y * invf + translatecy;
		//		pElement->m_z = 1.0f;
		//		pElement++;
		//	}
		//}
		break;
	}
	}
}

// Pass in theta, phi, and psi in radians, not degrees
void DpcppRemapping::ComputeRotationMatrix(float radTheta, float radPhi, float radPsi)
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

void DpcppRemapping::ConvertXYZToLonLat()
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
	int height = m_parameters->m_heightOutput;
	int width = m_parameters->m_widthOutput;
	Point3D* pPoints = &m_pXYZPoints[0];
	Point2D* pLonLatPoints = &m_pLonLatPoints[0];
	// Optimization, pull the values out of the rotational matrix if CACHE_ROTATION_ELEMENTS is defined.  This can
	// bring a 10x speed improvement for this function.
	float m00 = m_rotationMatrix.at<float>(0, 0);
	float m01 = m_rotationMatrix.at<float>(0, 1);
	float m02 = m_rotationMatrix.at<float>(0, 2);
	float m10 = m_rotationMatrix.at<float>(1, 0);
	float m11 = m_rotationMatrix.at<float>(1, 1);
	float m12 = m_rotationMatrix.at<float>(1, 2);
	float m20 = m_rotationMatrix.at<float>(2, 0);
	float m21 = m_rotationMatrix.at<float>(2, 1);
	float m22 = m_rotationMatrix.at<float>(2, 2);

	switch (m_storageType)
	{
	case STORAGE_TYPE_USM:
	{
		m_pQ->submit([&](sycl::handler& cgh) {
			cgh.parallel_for(sycl::range<2>(height, width),
			[=](sycl::id<2> item) {
				Point3D* pElement = &pPoints[item[0] * width + item[1]];
				Point2D* pLonLatElement = &pLonLatPoints[item[0] * width + item[1]];
				float norm;
	
				// Calculate xyz * R
				float eX = pElement->m_x;
				float eY = pElement->m_y;
				float eZ = pElement->m_z;

				pElement->m_x = eX * m00 + eY * m01 + eZ * m02;
				pElement->m_y = eX * m10 + eY * m11 + eZ * m12;
				pElement->m_z = eX * m20 + eY * m21 + eZ * m22;

				norm = sqrt(pElement->m_x * pElement->m_x + pElement->m_y * pElement->m_y + pElement->m_z * pElement->m_z);

				pLonLatElement->m_x = atan2(pElement->m_x / norm, pElement->m_z / norm);
				pLonLatElement->m_y = asin(pElement->m_y / norm);
			});
		});
		m_pQ->wait();

		break;
	}
	}
}

void DpcppRemapping::ComputeLonLatCoords()
{
	ComputeRotationMatrix((float)m_parameters->m_yaw * DEGREE_CONVERSION_FACTOR, (float)m_parameters->m_pitch * DEGREE_CONVERSION_FACTOR, (float)m_parameters->m_roll * DEGREE_CONVERSION_FACTOR);
	ConvertXYZToLonLat();
}

void DpcppRemapping::ComputeXYCoords()
{
	// Python code snippet that this is attempting to match (shape is the size or the equirectangular image)
	//X = (lonlat[..., 0:1] / (2 * np.pi) + 0.5) * (shape[1] - 1)
	//Y = (lonlat[..., 1:] / (np.pi) + 0.5) * (shape[0] - 1)
	//lst = [X, Y]
	//out = np.concatenate(lst, axis = -1)

	Point2D* pLonLatPoints = &m_pLonLatPoints[0];
	float xDiv = 2 * M_PI;
	float imageWidth = m_parameters->m_image[m_parameters->m_imageIndex].cols - 1;
	float imageHeight = m_parameters->m_image[m_parameters->m_imageIndex].rows - 1;;
	int height = m_parameters->m_heightOutput;
	int width = m_parameters->m_widthOutput;
	Point2D* pPoints = &m_pXYPoints[0];;

	switch (m_storageType)
	{
	case STORAGE_TYPE_USM:
	{
		m_pQ->submit([&](sycl::handler& cgh) {
			cgh.parallel_for(sycl::range<2>(height, width),
			[=](sycl::id<2> item) {
				Point2D* pElement = &pPoints[item[0] * width + item[1]];
				Point2D* pLonLatElement = &pLonLatPoints[item[0] * width + item[1]];

				pElement->m_x = (pLonLatElement->m_x / xDiv + 0.5) * imageWidth;
				pElement->m_y = (pLonLatElement->m_y / M_PI + 0.5) * imageHeight;
			});
		});
		m_pQ->wait();

		break;
	}
	}
}

bool DpcppRemapping::StartVariant()
{
	DpcppBaseAlgorithm::StartVariant();

	if (m_pQ)
	{
		switch (m_storageType)
		{
		case STORAGE_TYPE_USM:
			auto dev = m_pQ->get_device();
			auto ctxt = m_pQ->get_context();
			int size = m_parameters->m_widthOutput * m_parameters->m_heightOutput;

			m_pXYZPoints = (Point3D *)malloc_shared(size * sizeof(Point3D), dev, ctxt);
			m_pXYPoints = (Point2D *)malloc_shared(size * sizeof(Point2D), dev, ctxt);
			m_pLonLatPoints = (Point2D *)malloc_shared(size * sizeof(Point2D), dev, ctxt);

			printf("DpcppRemapping::StartVariant STORAGE_TYPE_USM\n");
			break;
		}
	}

	return m_pQ != NULL;
}

void DpcppRemapping::StopVariant()
{
	auto ctxt = m_pQ->get_context();

	free(m_pXYZPoints, ctxt);
	m_pXYZPoints = NULL;
	free(m_pXYPoints, ctxt);
	m_pXYPoints = NULL;
	free(m_pLonLatPoints, ctxt);
	m_pLonLatPoints = NULL;
}
