#pragma once

#include "BaseAlgorithm.hpp"

class ParallelRemapping : public BaseAlgorithm {
public:

	ParallelRemapping(SParameters &parameters);

	virtual void FrameCalculations(bool bParametersChanged);
	virtual cv::Mat ExtractFrameImage();

	virtual std::string GetDescription();
	virtual cv::Mat GetDebugImage();

};
