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

#include "ParseArgs.hpp"
#include <string>
#include <string.h>

const int STORE_INIT = -1;
// Array of structures row then column
const int STORE_ROW_COL = 0;
// Array of structures column then row
const int STORE_COL_ROW = 1;

const int STORE_MAX = 2;

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