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
#pragma once

#include "BaseAlgorithm.hpp"
#include "Point2D.hpp"
#include "Point3D.hpp"
#include "SoAPoints3D.hpp"
#include <opencv2/core/mat.hpp>

const int SRV2_INIT = -1;
// Array of structures row then column
const int SRV2_AOS = 0;
// Separate arrays for X and Y values
const int SRV2_SOA = 1;
const int SRV2_MAX = 2;

class SerialRemappingV2 : public BaseAlgorithm {

private:
	Point2D *m_pXYPoints = NULL;
	float *m_pXPoints = NULL;
	float *m_pYPoints = NULL;
	int m_storageType;
	cv::Mat m_rotationMatrix;

	// Pass in theta, phi, and psi in radians, not degrees
	void ComputeRotationMatrix(float radTheta, float radPhi, float radPsi);

public:
	SerialRemappingV2(SParameters &parameters);
	~SerialRemappingV2();

	virtual void FrameCalculations(bool bParametersChanged);
	virtual cv::Mat ExtractFrameImage();
	virtual cv::Mat GetDebugImage();

	virtual std::string GetDescription();

	virtual bool StartVariant();
	virtual void StopVariant();
};