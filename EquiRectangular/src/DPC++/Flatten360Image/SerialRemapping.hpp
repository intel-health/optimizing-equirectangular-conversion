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

const int STORE_INIT = -1;
// Array of structures row then column
const int STORE_ROW_COL = 0;
// Array of structures column then row
const int STORE_COL_ROW = 1;
//// Structure of arrays
//const int E_STORE_SOA = 2;
const int STORE_MAX = 2;

class SerialRemapping : public BaseAlgorithm {

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
	SerialRemapping(SParameters &parameters);
	~SerialRemapping();

	virtual void FrameCalculations(bool bParametersChanged);
	virtual cv::Mat ExtractFrameImage();
	virtual cv::Mat GetDebugImage();

	virtual std::string GetDescription();

	virtual bool StartVariant();
	virtual void StopVariant();
};