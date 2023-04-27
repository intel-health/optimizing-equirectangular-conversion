#pragma once

#include "ParseArgs.hpp"
#include <string>
#include <string.h>

class BaseAlgorithm {
protected:
	SParameters* m_parameters;

public:
	BaseAlgorithm(SParameters& parameters);

	virtual void FrameCalculations() = 0;
	virtual cv::Mat ExtractFrameImage() = 0;
	virtual cv::Mat GetDebugImage() = 0;

	virtual std::string GetDescription() = 0;

	virtual bool StartSubAlgorithm(uint algNum);
};