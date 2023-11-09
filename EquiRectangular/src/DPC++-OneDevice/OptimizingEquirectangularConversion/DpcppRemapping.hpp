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

#include <sycl/sycl.hpp>
#include "DpcppBaseAlgorithm.hpp"
#include "Point3D.hpp"

class DpcppRemapping : public DpcppBaseAlgorithm {
private:

	Point3D* m_pXYZPoints = NULL;
	Point2D* m_pLonLatPoints = NULL;
	Point2D* m_pXYPoints = NULL;
	int m_storageType;
	cv::Mat m_rotationMatrix;

private:
	// Pass in theta, phi, and psi in radians, not degrees
	void ComputeRotationMatrix(float radTheta, float radPhi, float radPsi);
	void ComputeXYZCoords();
	void ComputeLonLatCoords();
	void ConvertXYZToLonLat();
	void ComputeXYCoords();

public:

	DpcppRemapping(SParameters& parameters);

	virtual void FrameCalculations(bool bParametersChanged);
	virtual cv::Mat ExtractFrameImage();

	virtual std::string GetDescription();

	virtual bool StartVariant();
	virtual void StopVariant();

};