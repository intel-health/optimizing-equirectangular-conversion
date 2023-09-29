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

class SerialRemappingV1a : public BaseAlgorithm {

private:
	Point3D *m_pXYZPoints = NULL;
	Point2D *m_pLonLatPoints = NULL;
	Point2D *m_pXYPoints = NULL;
	int m_storageOrder;
	cv::Mat m_rotationMatrix;

	// Pass in theta, phi, and psi in radians, not degrees
	void ComputeRotationMatrix(float radTheta, float radPhi, float radPsi);
	void ComputeXYZCoords();
	void ComputeLonLatCoords();
	void ConvertXYZToLonLat();
	void ComputeXYCoords();


public:
	SerialRemappingV1a(SParameters &parameters);
	~SerialRemappingV1a();

	virtual void FrameCalculations(bool bParametersChanged);
	virtual cv::Mat ExtractFrameImage();
	virtual cv::Mat GetDebugImage();

	virtual std::string GetDescription();

	virtual bool StartVariant();
	virtual void StopVariant();
};