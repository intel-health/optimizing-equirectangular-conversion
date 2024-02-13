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

#include "DpcppBaseAlgorithm.hpp"
#include "ConfigurableDeviceSelector.hpp"

DpcppBaseAlgorithm::DpcppBaseAlgorithm(SParameters& parameters) : BaseAlgorithm(parameters)
{
    m_pQ = NULL;
    m_bInitRequired = true;
}

DpcppBaseAlgorithm::~DpcppBaseAlgorithm()
{
    StopVariant();
}

sycl::queue* DpcppBaseAlgorithm::GetDeviceQ()
{
    // create an async exception handler so the program fails more gracefully.
    auto ehandler = [](cl::sycl::exception_list exceptionList) {
        for (std::exception_ptr const& e : exceptionList) {
            try {
                std::cout << "Exception caught in ehandler, rethrowing " << std::endl;
                std::rethrow_exception(e);
            }
            catch (cl::sycl::exception const& e) {
                std::cout << "Caught an asynchronous DPC++ exception, terminating the "
                    "program."
                    << std::endl;
                std::terminate();
            }
        }
    };

    try {
        if (m_pQ != NULL)
        {
            delete m_pQ;
            m_pQ = NULL;
        }
        ConfigurableDeviceSelector::set_search(m_typePreference, m_platformName, m_deviceName, m_driverVersion);
        m_pQ = new sycl::queue(ConfigurableDeviceSelector::device_selector, ehandler);
#define SHOW_RESULTS
#ifdef SHOW_RESULTS
        if (m_pQ)
        {
            sycl::device device = m_pQ->get_device();
            ConfigurableDeviceSelector::print_platform_info(device);
            ConfigurableDeviceSelector::print_device_info(device);
        }
#endif
    }
    catch (cl::sycl::exception const& e) {
        std::cout << "Exception caught " << e.what() << std::endl;
        m_pQ = NULL;
    }

    return m_pQ;
}

sycl::queue* DpcppBaseAlgorithm::GetNextDeviceQ()
{
    bool bDone = false;

    do
    {
        if (m_curDevice != m_devices.end())
        {
            m_curDevice++;
        }
        if (m_curDevice == m_devices.end())
        {
            if (m_curPlatform != m_platforms.end())
            {
                m_curPlatform++;
            }
            if (m_curPlatform != m_platforms.end())
            {
                m_devices = m_curPlatform->get_devices();
                m_curDevice = m_devices.begin();
            }
        }
        if (m_curPlatform != m_platforms.end() && m_curDevice != m_devices.end())
        {
            m_platformName = m_curPlatform->get_info<sycl::info::platform::name>();
            m_deviceName = m_curDevice->get_info<sycl::info::device::name>();
            m_driverVersion = m_curDevice->get_info<sycl::info::device::driver_version>();
            GetDeviceQ();
        }
        else
        {
            bDone = true;
            delete m_pQ;
            m_pQ = NULL;
        }
    } while (m_pQ == NULL && !bDone);

    return m_pQ;
}

std::string DpcppBaseAlgorithm::GetDeviceDescription()
{
    std::string retVal = "";

    if (m_pQ != NULL)
    {
        retVal = ConfigurableDeviceSelector::get_device_description(m_pQ->get_device());
    }

    return retVal;
}

bool DpcppBaseAlgorithm::StartVariant()
{
    printf("DpcppBaseAlgorithm::StartVariant start\n");
    BaseAlgorithm::StartVariant();

    if (m_bInitRequired)
    {
        m_bInitRequired = false;
        // First time, so we need to figure out if this is a one-shot run (i.e.,
        // neither the m_platformName nor m_deviceName == all or if we will be looping
        // due to either being all.
        if (m_parameters->m_platformName == "all" || m_parameters->m_deviceName == "all")
        {
            m_platforms = sycl::platform::get_platforms();
            m_curPlatform = m_platforms.begin();
            if (m_curPlatform != m_platforms.end())
            {
                m_devices = m_curPlatform->get_devices();
                m_curDevice = m_devices.begin();
                m_platformName = m_curPlatform->get_info<sycl::info::platform::name>();
                if (m_curDevice != m_devices.end())
                {
                    m_deviceName = m_curDevice->get_info<sycl::info::device::name>();
                    m_driverVersion = m_curDevice->get_info<sycl::info::device::driver_version>();
                }
            }
            m_typePreference = m_parameters->m_typePreference;
            m_bAll = true;
        }
        else
        {
            m_typePreference = m_parameters->m_typePreference;
            m_platformName = m_parameters->m_platformName;
            m_deviceName = m_parameters->m_deviceName;
            m_driverVersion = m_parameters->m_driverVersion;

            m_bAll = false;
        }
        if (GetDeviceQ() == NULL)
        {
            GetNextDeviceQ();
        }
    }
    else
    {
        if (m_bAll)
        {
            GetNextDeviceQ();
        }
        else
        {
            delete m_pQ;
            m_pQ = NULL;
        }
    }
    m_bFrameCalcRequired = true;
    printf("DpcppBaseAlgorithm::StartVariant end\n");

    return m_pQ != NULL;
}

void DpcppBaseAlgorithm::StopVariant()
{
    delete m_pQ;
    m_pQ = NULL;
}

cv::Mat DpcppBaseAlgorithm::GetDebugImage()
{
    cv::Mat retVal;
    Point2D* pXYElement = &m_pXYPoints[0];

    m_parameters->m_image[m_parameters->m_imageIndex].copyTo(retVal);

    // Draw the points across the top of the viewing region using blue
    for (int x = 0; x < m_parameters->m_widthOutput; x++)
    {
        cv::Point pt = cv::Point(pXYElement->m_x, pXYElement->m_y);
        cv::line(retVal, pt, pt, cv::Scalar(255, 0, 0), 10);

        pXYElement++;
    }
    // Draw the left (red) and right (green) sides of the viewing region
    for (int y = 1; y < m_parameters->m_heightOutput - 2; y++)
    {
        cv::Point ptLeft = cv::Point(pXYElement->m_x, pXYElement->m_y);
        cv::line(retVal, ptLeft, ptLeft, cv::Scalar(0, 0, 255), 10);

        pXYElement += m_parameters->m_widthOutput - 1;
        cv::Point ptRight = cv::Point(pXYElement->m_x, pXYElement->m_y);
        cv::line(retVal, ptRight, ptRight, cv::Scalar(0, 255, 0), 10);

        pXYElement++;
    }

    // Draw the points across the bottom of the viewing region in tan
    for (int x = 0; x < m_parameters->m_widthOutput; x++)
    {
        cv::Point pt = cv::Point(pXYElement->m_x, pXYElement->m_y);
        cv::line(retVal, pt, pt, cv::Scalar(74, 136, 175), 10);

        pXYElement++;
    }

    return retVal;
}