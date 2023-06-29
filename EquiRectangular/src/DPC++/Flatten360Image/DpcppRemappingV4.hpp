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

// The primary difference between this implementation and DpcppRemapping2 is that this one spawns
// less parallel_for items and does more work in each of the iterations

#include <sycl/sycl.hpp>
#include "DpcppBaseAlgorithm.hpp"
#include "Point2D.hpp"
#include "Point3D.hpp"

class DpcppRemappingV4 : public DpcppBaseAlgorithm {
private:

	Point2D* m_pDevXYPoints = NULL;
	int m_storageType;
	cv::Mat m_rotationMatrix;

private:
	// Pass in theta, phi, and psi in radians, not degrees
	void ComputeRotationMatrix(float radTheta, float radPhi, float radPsi);

public:

	DpcppRemappingV4(SParameters& parameters);

	virtual void FrameCalculations(bool bParametersChanged);
	virtual cv::Mat ExtractFrameImage();

	virtual std::string GetDescription();

	virtual bool StartVariant();
	virtual void StopVariant();

};
