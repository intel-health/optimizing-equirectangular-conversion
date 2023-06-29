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
