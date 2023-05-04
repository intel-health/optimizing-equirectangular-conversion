#include "ParallelRemapping.hpp"

ParallelRemapping::ParallelRemapping(SParameters& parameters) : BaseAlgorithm(parameters)
{

}

std::string ParallelRemapping::GetDescription()
{
	return "Computes using a Remapping algorithm using SIMD.";
}

void ParallelRemapping::FrameCalculations(bool bParametersChanged)
{
}

cv::Mat ParallelRemapping::ExtractFrameImage()
{
	return cv::Mat{};
}

cv::Mat ParallelRemapping::GetDebugImage()
{
	cv::Mat retVal;

	m_parameters->m_image[m_parameters->m_imageIndex].copyTo(retVal);

	return retVal;
}
