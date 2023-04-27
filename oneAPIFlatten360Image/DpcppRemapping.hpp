#pragma once

#include <sycl/sycl.hpp>
#include "DpcppBaseAlgorithm.hpp"

class DpcppRemapping : public DpcppBaseAlgorithm {
private:

public:

	DpcppRemapping(SParameters& parameters);

	virtual void FrameCalculations();
	virtual cv::Mat ExtractFrameImage();

	virtual std::string GetDescription();
	virtual cv::Mat GetDebugImage();

};
