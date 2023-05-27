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