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

#include "ParseArgs.hpp"
#include <string>
#include <string.h>

class BaseAlgorithm {
protected:
	SParameters* m_parameters;
	bool m_bFrameCalcRequired;
	bool m_bVariantRun;

public:
	BaseAlgorithm(SParameters& parameters);
	virtual ~BaseAlgorithm();

	virtual void FrameCalculations(bool bParametersChanged);
	virtual cv::Mat ExtractFrameImage() = 0;
	virtual cv::Mat GetDebugImage() = 0;

	virtual std::string GetDescription() = 0;

	virtual bool StartVariant();
	virtual void StopVariant() = 0;
};