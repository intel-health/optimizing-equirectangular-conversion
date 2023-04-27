#include "DpcppRemapping.hpp"
#include "ConfigurableDeviceSelector.hpp"

// output message for runtime exceptions
#define EXCEPTION_MSG \
  "    If you are targeting an FPGA hardware, please ensure that an FPGA board is plugged to the system, \n\
        set up correctly and compile with -DFPGA  \n\
    If you are targeting the FPGA emulator, compile with -DFPGA_EMULATOR.\n"

DpcppRemapping::DpcppRemapping(SParameters& parameters) : DpcppBaseAlgorithm(parameters)
{
}

std::string DpcppRemapping::GetDescription()
{
    std::string strDesc = GetDeviceDescription();

    return "Computes a Remapping algorithm using oneAPI's DPC++ " + strDesc;
}

void DpcppRemapping::FrameCalculations()
{
}

cv::Mat DpcppRemapping::ExtractFrameImage()
{
    cv::Mat retVal;

    m_parameters->m_image[m_parameters->m_imageIndex].copyTo(retVal);

    return retVal;
}

cv::Mat DpcppRemapping::GetDebugImage()
{
	cv::Mat retVal;

	m_parameters->m_image[m_parameters->m_imageIndex].copyTo(retVal);

	return retVal;
}
