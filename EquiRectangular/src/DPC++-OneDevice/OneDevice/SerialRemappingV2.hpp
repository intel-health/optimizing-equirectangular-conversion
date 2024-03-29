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