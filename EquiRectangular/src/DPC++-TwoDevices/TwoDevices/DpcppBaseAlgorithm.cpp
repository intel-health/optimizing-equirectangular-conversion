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
#include "TimingStats.hpp"

DpcppBaseAlgorithm::DpcppBaseAlgorithm(SParameters& parameters) : BaseAlgorithm(parameters)
{
    m_pQ = NULL;
    m_bInitRequired = true;
    m_bThreadStopRequested = false;
}

DpcppBaseAlgorithm::~DpcppBaseAlgorithm()
{
    delete m_pQ;
    m_pQ = NULL;
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
        if (m_pParameters->m_platformName == "all" || m_pParameters->m_deviceName == "all")
        {
            m_platforms = sycl::platform::get_platforms();
            m_curPlatform = m_platforms.begin();
            if (m_curPlatform != m_platforms.end())
            {
                m_devices = m_curPlatform->get_devices();
                m_curDevice = m_devices.begin();
            }
            m_typePreference = m_pParameters->m_typePreference;
            m_platformName = m_curPlatform->get_info<sycl::info::platform::name>();
            m_deviceName = m_curDevice->get_info<sycl::info::device::name>();
            m_driverVersion = m_curDevice->get_info<sycl::info::device::driver_version>();
            m_bAll = true;
        }
        else
        {
            m_typePreference = m_pParameters->m_typePreference;
            m_platformName = m_pParameters->m_platformName;
            m_deviceName = m_pParameters->m_deviceName;
            m_driverVersion = m_pParameters->m_driverVersion;

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

    m_pParameters->m_image[m_pParameters->m_imageIndex].copyTo(retVal);

    // Draw the points across the top of the viewing region using blue
    for (int x = 0; x < m_pParameters->m_widthOutput; x++)
    {
        cv::Point pt = cv::Point(pXYElement->m_x, pXYElement->m_y);
        cv::line(retVal, pt, pt, cv::Scalar(255, 0, 0), 10);

        pXYElement++;
    }
    // Draw the left (red) and right (green) sides of the viewing region
    for (int y = 1; y < m_pParameters->m_heightOutput - 2; y++)
    {
        cv::Point ptLeft = cv::Point(pXYElement->m_x, pXYElement->m_y);
        cv::line(retVal, ptLeft, ptLeft, cv::Scalar(0, 0, 255), 10);

        pXYElement += m_pParameters->m_widthOutput - 1;
        cv::Point ptRight = cv::Point(pXYElement->m_x, pXYElement->m_y);
        cv::line(retVal, ptRight, ptRight, cv::Scalar(0, 255, 0), 10);

        pXYElement++;
    }

    // Draw the points across the bottom of the viewing region in tan
    for (int x = 0; x < m_pParameters->m_widthOutput; x++)
    {
        cv::Point pt = cv::Point(pXYElement->m_x, pXYElement->m_y);
        cv::line(retVal, pt, pt, cv::Scalar(74, 136, 175), 10);

        pXYElement++;
    }

    return retVal;
}

void DpcppBaseAlgorithm::StartThread()
{
    m_pThread = new std::thread([this] {threadFunc(); });

    //m_hThread = reinterpret_cast<HANDLE>(
    //    ::_beginthreadex(
    //        0, // security
    //        0, // stack size
    //        startThreadProc, // thread routine
    //        static_cast<void *>(this), // thread arg
    //        0, // initial state flag
    //        &m_threadId // thread ID
    //    )
    //    );
    if (m_pThread == NULL)
    {
        throw std::exception("failed to create thread");
    }
}

// The caller should also call ThreadWait to wait for the thread to end and clean up the object
bool DpcppBaseAlgorithm::RequestStopThread()
{
    // Return false if a thread was not running for this object
    bool bRetVal = false;

    if (m_pThread != NULL)
    {
        m_bThreadStopRequested = true;
        bRetVal = true;
    }

    return bRetVal;
}

// This should only be called after RequestStopThread and from any other
// thread except the one that is being waited upon (that would cause a
// deadlock).  The function waits for the thread to terminate
void DpcppBaseAlgorithm::ThreadWait()
{
    if (m_pThread != NULL)
    {
        m_pThread->join();
        delete m_pThread;
        m_pThread = NULL;
    }
}

//unsigned __stdcall DpcppBaseAlgorithm::startThreadProc(void *a_param)
//{
//    DpcppBaseAlgorithm *obj = static_cast<DpcppBaseAlgorithm *>(a_param);
//
//    return obj->threadFunc();
//}

unsigned int DpcppBaseAlgorithm::threadFunc()
{
    SParameters prevParameters = *m_pParameters;
    std::chrono::high_resolution_clock::time_point frameStartTime;
    std::chrono::high_resolution_clock::time_point extractionStartTime;
    std::chrono::high_resolution_clock::time_point frameEndTime;
    TimingStats *pTimingStats = TimingStats::GetTimingStats();

    while (!m_bThreadStopRequested)
    {
        // We will await a command from the commanding layer that we should do
        // our work
        {
            std::unique_lock<std::mutex> requestWorkLock(*(m_pParameters->m_pRequestWorkMutex));
            m_pParameters->m_pRequestWorkCondVar->wait(requestWorkLock, [this] {return (*(m_pParameters->m_pRequestWork) & m_pParameters->m_uiMyMask) == m_pParameters->m_uiMyMask; });
            // Now that we know we are meant to do work, clear out our mask from
            // the request, and release the lock in case other layers need to see it too
            *(m_pParameters->m_pRequestWork) &= ~m_pParameters->m_uiMyMask;
        }

        if (!m_bThreadStopRequested)
        {
            bool bParametersChanged = prevParameters != *m_pParameters;

            frameStartTime = std::chrono::high_resolution_clock::now();
            FrameCalculations(bParametersChanged);
            pTimingStats->AddIterationResults(ETimingType::TIMING_FRAME_CALCULATIONS, m_pParameters->m_uiDevIndex, frameStartTime, std::chrono::high_resolution_clock::now());
            prevParameters = *m_pParameters;
            extractionStartTime = std::chrono::high_resolution_clock::now();
            m_pParameters->m_FlatImg = ExtractFrameImage();
            frameEndTime = std::chrono::high_resolution_clock::now();
            pTimingStats->AddIterationResults(ETimingType::TIMING_IMAGE_EXTRACTION, m_pParameters->m_uiDevIndex, extractionStartTime, frameEndTime);
            pTimingStats->AddIterationResults(ETimingType::TIMING_FRAME, m_pParameters->m_uiDevIndex, frameStartTime, frameEndTime);

            {
                // Note that we are done with the work
                std::lock_guard<std::mutex> workCompletedLock(*(m_pParameters->m_pWorkCompletedMutex));
                *(m_pParameters->m_pWorkCompleted) |= m_pParameters->m_uiMyMask;
            }
            m_pParameters->m_pWorkCompletedCondVar->notify_one();
        }
    }
}

