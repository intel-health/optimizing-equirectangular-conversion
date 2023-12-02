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


#include <CL/sycl.hpp>
#if defined(FPGA) || defined(FPGA_EMULATOR)
#include <CL/sycl/INTEL/fpga_extensions.hpp>
#endif
#include <array>
#include <iostream>
#include <opencv2/core/ocl.hpp>
#include <opencv2/core/utils/logger.hpp>

#include "ParseArgs.hpp"
#include "DpcppRemapping.hpp"
#include "DpcppRemappingV2.hpp"
#include "DpcppRemappingV3.hpp"
#include "DpcppRemappingV4.hpp"
#include "DpcppRemappingV5.hpp"
#include "DpcppRemappingV6.hpp"
#include "DpcppRemappingV7.hpp"
#include "OptimizingEquirectangularConversion.h"
#include "TimingStats.hpp"
#include "ConfigurableDeviceSelector.hpp"

using namespace cl::sycl;

#ifdef VTUNE_API
// Assuming you have 2023 of VTune installed, you will need to add
// $(VTUNE_PROFILER_2023_DIR)\sdk\include
// to the include directories for any configurations that will be profiled.  Also add
// $(VTUNE_PROFILER_2023_DIR)\sdk\lib64
// to the linker additional directories area
#include "ittnotify.h"
#pragma comment(lib, "libittnotify.lib")
__itt_domain* pittTests_domain = __itt_domain_createA("localDomain");
// Create string handle for denoting when the kernel is running
wchar_t const *pPrintParameters = _T("Print Parameters");
__itt_string_handle *handle_print_parameters = __itt_string_handle_create(pPrintParameters);
#endif

void NormalizeParameters(SParameters &parameters)
{
    if (parameters.m_pitch > 90)
    {
        parameters.m_pitch = 90;
    }
    else if (parameters.m_pitch < -90)
    {
        parameters.m_pitch = -90;
    }
    if (parameters.m_fov < 10)
    {
        parameters.m_fov = 10;
    }
    else if (parameters.m_fov > 120)
    {
        parameters.m_fov = 120;
    }
    if (parameters.m_yaw > 180)
    {
        // Wrap around to the other side of the 360 view
        parameters.m_yaw = (-180 + parameters.m_yaw) % 360 - 180;
    }
    else if (parameters.m_yaw < -180)
    {
        parameters.m_yaw = (180 + parameters.m_yaw) % 360 + 180;
    }
    if (parameters.m_roll < 0)
    {
        parameters.m_roll = 360 + (parameters.m_roll % -360);
    }
    else if (parameters.m_roll >= 360)
    {
        parameters.m_roll = parameters.m_roll % 360;
    }
}

void UpdateParameters(SParameters &parameters)
{
    parameters.m_yaw += parameters.m_deltaYaw;
    parameters.m_pitch += parameters.m_deltaPitch;
    parameters.m_roll += parameters.m_deltaRoll;
    if (parameters.m_deltaImage)
    {
        parameters.m_imageIndex = (parameters.m_imageIndex + 1) % 2;
    }
}

int main(int argc, char** argv) {
    SParameters parameters;
    // For this demo, just support 2 devices (CPU and GPU)
    SParameters devParameters[MAX_DEVICES];
    DpcppBaseAlgorithm *pDevAlg[MAX_DEVICES];
    // The following variables are for interactions between the main thread and the worker thread(s)
    std::mutex requestWorkMutex;
    std::condition_variable requestWorkCondVar;
    // availableDevices is a mask indicating which devices can be assigned work
    unsigned int availableDevices = ALL_DEVICES_MASK;
    // requestWork is used with the above mutex and condition variable
    // to indicate which devices are being requested to start working
    unsigned int requestWork = 0;
    // m_workCompletedMutex is the mutex that controls access to the m_workCompleted
    // variable
    std::mutex workCompletedMutex;
    // m_workCompletedCondVar
    std::condition_variable workCompletedCondVar;
    // m_workCompleted is used with the above mutex and condition variable
    // so devices can report when they have completed the requested work and are now
    // available.
    unsigned int workCompleted = 0;

    char errorMessage[MAX_ERROR_MESSAGE];

    if (!ParseArgs(argc, argv, &parameters, errorMessage))
    {
        PrintUsage(argv[0], errorMessage);
        exit(1);
    }

#ifdef VTUNE_API
    wchar_t const *pThreadName = _T("Main thread");

    __itt_thread_set_name(pThreadName);
#endif

    std::string description;
    std::chrono::high_resolution_clock::time_point initStartTime;
    std::chrono::high_resolution_clock::time_point initEndTime;
    std::chrono::high_resolution_clock::time_point variantInitStartTime;
    std::chrono::high_resolution_clock::time_point variantInitStopTime;
    std::chrono::high_resolution_clock::time_point totalTimeStart;
    std::chrono::high_resolution_clock::time_point totalTimeEnd;
    TimingStats *pTimingStats = TimingStats::GetTimingStats();
    bool bInteractive = parameters.m_iterations < 1;
    int iteration = 0;
    int finishedIterations = 0;
    int startAlgorithm;
    int endAlgorithm;
    int key;
    bool bDoIterations = true;
    cv::Mat debugImg;
    cv::Mat flatImg;
    bool bRunningVariant;
    bool bVariantValid;
    std::vector<std::string> summaryStats;
    int origYaw = parameters.m_yaw;
    int origPitch = parameters.m_pitch;
    int origRoll = parameters.m_roll;

    //cv::utils::logging::setLogLevel(cv::utils::logging::LogLevel::LOG_LEVEL_SILENT);
    //cv::utils::logging::setLogLevel(cv::utils::logging::LogLevel::LOG_LEVEL_FATAL);
    cv::utils::logging::setLogLevel(cv::utils::logging::LogLevel::LOG_LEVEL_ERROR);
    //cv::utils::logging::setLogLevel(cv::utils::logging::LogLevel::LOG_LEVEL_WARNING);
    if (parameters.m_algorithm > -1)
    {
        startAlgorithm = parameters.m_algorithm;
        endAlgorithm = parameters.m_algorithm;
    }
    else
    {
        startAlgorithm = parameters.m_startAlgorithm;
        if (parameters.m_endAlgorithm == -1)
        {
            endAlgorithm = MAX_ALGORITHM;
        }
        else
        {
            endAlgorithm = parameters.m_endAlgorithm;
        }
    }

    // See if we just need to list the platforms / devices
    if (parameters.m_platformName == "list" || parameters.m_deviceName == "list")
    {
        try {
            for (auto platform : sycl::platform::get_platforms())
            {
                ConfigurableDeviceSelector::print_platform_info(platform);

                for (auto device : platform.get_devices())
                {
                    ConfigurableDeviceSelector::print_device_info(device);
                }
                std::cout << std::endl;
            }
        }
        catch (std::exception const& e) {
            // catch the exception from devices that are not supported.
            std::cout << "An exception is caught when enumerating platforms and devices."
                << std::endl;
            std::cout << e.what() << std::endl;
        }

        exit(0);
    }

    // Adjust the width to be a factor of 16 and the height to be a factor of 8 to make the
    // parallel algorithms not need to worry about odd sized images.  Later this could be relaxed
    // by making an internal space that meets these criteria and then pulling the user requested
    // size from within that space.
    if (parameters.m_widthOutput % 16 != 0)
    {
        parameters.m_widthOutput = ((parameters.m_widthOutput / 16) + 1) * 16;
    }
    if (parameters.m_heightOutput % 8 != 0)
    {
        parameters.m_heightOutput = ((parameters.m_heightOutput / 8) + 1) * 8;
    }
    for (unsigned int i = 0; i < MAX_DEVICES; i++)
    {
        // read src image0
        printf("Loading Image0\n");
        devParameters[i].m_image[0] = cv::imread(parameters.m_imgFilename[0], cv::IMREAD_COLOR);
        if (devParameters[i].m_image[0].empty())
        {
            printf("Error: Could not load image 0 from %s\n", parameters.m_imgFilename[0]);
            throw std::invalid_argument("Error: Could not load image 0.");
        }
        // read src image1
        printf("Loading Image1\n");
        devParameters[i].m_image[1] = cv::imread(parameters.m_imgFilename[1], cv::IMREAD_COLOR);
        if (devParameters[i].m_image[1].empty())
        {
            printf("Error: Could not load image 1 from %s\n", parameters.m_imgFilename[1]);
            throw std::invalid_argument("Error: Could not load image 1.");
        }
    }
    printf("Images loaded.\n");
    int algorithm = startAlgorithm;
    unsigned int uiDevIndex = 0;

    for (unsigned int uiMask = 1; uiMask < ALL_DEVICES_MASK; uiMask <<= 1)
    {
        devParameters[uiDevIndex] = parameters;
        devParameters[uiDevIndex].m_uiMyMask = uiMask;
        devParameters[uiDevIndex].m_uiDevIndex = uiDevIndex;
        devParameters[uiDevIndex].m_pRequestWorkMutex = &requestWorkMutex;
        devParameters[uiDevIndex].m_pRequestWorkCondVar = &requestWorkCondVar;
        devParameters[uiDevIndex].m_pRequestWork = &requestWork;
        devParameters[uiDevIndex].m_pWorkCompletedMutex = &workCompletedMutex;
        devParameters[uiDevIndex].m_pWorkCompletedCondVar = &workCompletedCondVar;
        devParameters[uiDevIndex].m_pWorkCompleted = &workCompleted;
        uiDevIndex++;
    }
    devParameters[0].m_typePreference = "CPU";
    devParameters[1].m_typePreference = "GPU";
    // Uncomment enable / disable the code lines below to select a specific iGPU.  If
    // neither line is enabled, then oneAPI will select the best GPU.
    // Specifically select the OpenCL driver for the GPU versus Level Zero
    devParameters[1].m_platformName = "OpenCL";
    // Specifically select the Level-Zero driver for the GPU versus Level Zero
    //devParameters[1].m_platformName = "Level-Zero";

    try {
        while (algorithm <= endAlgorithm)
        {
            initStartTime = std::chrono::high_resolution_clock::now();
            switch (algorithm)
            {
            case 5:
                for (unsigned int i = 0; i < MAX_DEVICES; i++)
                {
                    pDevAlg[i] = new DpcppRemapping(devParameters[i]);
                }
                break;
            case 6:
                for (unsigned int i = 0; i < MAX_DEVICES; i++)
                {
                    pDevAlg[i] = new DpcppRemappingV2(devParameters[i]);
                }
                break;
            case 7:
                for (unsigned int i = 0; i < MAX_DEVICES; i++)
                {
                    pDevAlg[i] = new DpcppRemappingV3(devParameters[i]);
                }
                break;
            case 8:
                for (unsigned int i = 0; i < MAX_DEVICES; i++)
                {
                    pDevAlg[i] = new DpcppRemappingV4(devParameters[i]);
                }
                break;
            case 9:
                for (unsigned int i = 0; i < MAX_DEVICES; i++)
                {
                    pDevAlg[i] = new DpcppRemappingV5(devParameters[i]);
                }
                break;
            case 10:
                for (unsigned int i = 0; i < MAX_DEVICES; i++)
                {
                    pDevAlg[i] = new DpcppRemappingV6(devParameters[i]);
                }
                break;
            case 11:
                for (unsigned int i = 0; i < MAX_DEVICES; i++)
                {
                    pDevAlg[i] = new DpcppRemappingV7(devParameters[i]);
                }
                break;
            }

            if (pDevAlg[0] != NULL && pDevAlg[1] != NULL)
            {
                for (unsigned int i = 0; i < MAX_DEVICES; i++)
                {
                    pDevAlg[i]->StartThread();
                }
                initEndTime = std::chrono::high_resolution_clock::now();

                bVariantValid = true;
                while (bVariantValid)
                {
                    variantInitStartTime = std::chrono::high_resolution_clock::now();
                    description = "";
                    for (unsigned int i = 0; i < MAX_DEVICES; i++)
                    {
                        bVariantValid = (bVariantValid && pDevAlg[i]->StartVariant());
                        description += pDevAlg[i]->GetDescription();
                        description += " ";
                    }

                    if (bVariantValid)
                    {
                        // Reset the perspective back to the inital values so each algorithm
                        // works from the same baseline
                        parameters.m_yaw = origYaw;
                        parameters.m_pitch = origPitch;
                        parameters.m_roll = origRoll;
                        parameters.m_imageIndex = 0;
                        iteration = 0;
                        finishedIterations = 0;

                        pTimingStats->Reset();
                        pTimingStats->AddIterationResults(ETimingType::TIMING_INITIALIZATION, GENERAL_STATS, initStartTime, initEndTime);
                        pTimingStats->AddIterationResults(ETimingType::VARIANT_INITIALIZATION, GENERAL_STATS, variantInitStartTime, std::chrono::high_resolution_clock::now());
                        bRunningVariant = true;
                        totalTimeStart = std::chrono::high_resolution_clock::now();
                        while (bRunningVariant)
                        {
                            pTimingStats->ResetLap();
#ifdef VTUNE_API
                            // We only care about profiling the specific calculations so resume VTune data collection
                            // here and then pause after our iterations.  The user should start VTune paused to ignore
                            // tracing the above code as well.
                            __itt_resume();
#endif
                            try
                            {
                                if (bDoIterations)
                                {
                                    do
                                    {
                                        unsigned int newWork = 0;
                                        unsigned int finishedWork = 0;

                                        if (iteration < parameters.m_iterations)
                                        {
                                            uiDevIndex = 0;

                                            for (unsigned int uiMask = 1; (uiMask < ALL_DEVICES_MASK) && (iteration < parameters.m_iterations); uiMask <<= 1)
                                            {
                                                if ((uiMask & availableDevices) == uiMask)
                                                {
                                                    NormalizeParameters(parameters);
                                                    // Copy the interesting parameters over to the device parameters structure
                                                    devParameters[uiDevIndex] = parameters;
                                                    newWork |= uiMask;
                                                    // Update the parameters for the next time we can give out work
                                                    UpdateParameters(parameters);
                                                    // Clear the device from the available devices list while it is working
                                                    availableDevices &= ~uiMask;
                                                    iteration++;
                                                }
                                                uiDevIndex++;
                                            }

                                            // Create a code block so the requestWorkLock will be automatically released
                                            {
                                                std::lock_guard<std::mutex> requestWorkLock(requestWorkMutex);
                                                // We have the lock, select the layers that should do the work (in this case all)
                                                requestWork |= newWork;
                                            }
                                            // Let all sub-Layers know about the request so they can determine if it applies
                                            requestWorkCondVar.notify_all();

                                        }
                                        // Now wait for one or more to be done.  Do it in a code block to make sure
                                        // the lock is released in all cases.
                                        {
                                            std::unique_lock<std::mutex> workCompletedLock(workCompletedMutex);
                                            workCompletedCondVar.wait(workCompletedLock, [&] {
                                                return workCompleted != 0;
                                                });
                                            finishedWork = workCompleted;
                                            // Clear the variable for the next time.
                                            workCompleted = 0;
                                        }

                                        availableDevices |= finishedWork;
                                        uiDevIndex = 0;

                                        for (unsigned int uiMask = 1; uiMask < ALL_DEVICES_MASK; uiMask <<= 1)
                                        {
                                            if ((uiMask & finishedWork) == uiMask)
                                            {
                                                if (parameters.m_bShowFrames)
                                                {
                                                    flatImg = devParameters[uiDevIndex].m_FlatImg;
                                                    char windowText[1024];

                                                    sprintf(windowText, "Algorithm %d Flat View %s", algorithm, description.c_str());

                                                    cv::imshow(windowText, flatImg);
                                                    // Waiting for a key for 1 millisecond gives OpenCV a hint that it
                                                    // should show the frame
                                                    key = cv::waitKeyEx(1);
                                                }

                                                finishedIterations++;
                                            }
                                            uiDevIndex++;
                                        }
                                    } while (finishedIterations < parameters.m_iterations);
#ifdef VTUNE_API
                                    __itt_pause();
#endif
                                    totalTimeEnd = std::chrono::high_resolution_clock::now();

                                    pTimingStats->AddIterationResults(ETimingType::TIMING_TOTAL, GENERAL_STATS, totalTimeStart, totalTimeEnd);

                                    printf("Algorithm description: %s\n", description.c_str());
                                    pTimingStats->ReportTimes(true);

                                }
                            }
                            catch (cl::sycl::exception const &e) {
                                std::cout << "SYCL exception caught during main loop " << e.what() << std::endl;

                            }
                            catch (std::exception const &e) {
                                // catch the exception from devices that are not supported.
                                std::cout << "Exception caught during main loop." << std::endl;
                                std::cout << e.what() << std::endl;
                            }

                            bDoIterations = true;

                            //if (bDebug)
                            //{
                            //    // When debugging, we want to show the original image with an outline of the Region of Interest
                            //    // The color is BGR format
                            //    debugImg = pAlg->GetDebugImage();

                            //    cv::namedWindow("Debug View", cv::WINDOW_NORMAL);
                            //    cv::imshow("Debug View", debugImg);
                            //}
                            bRunningVariant = false;
                        }

                        variantInitStopTime = std::chrono::high_resolution_clock::now();
                        for (unsigned int i = 0; i < MAX_DEVICES; i++)
                        {
                            pDevAlg[i]->StopVariant();
                        }
                        pTimingStats->AddIterationResults(ETimingType::VARIANT_TERMINATION, 0, variantInitStopTime, std::chrono::high_resolution_clock::now());
                        summaryStats.push_back(description + "\n" + pTimingStats->SummaryStats(true));
                    }
                }


                // Need to let all the threads know that we want to command them to stop.
                for (unsigned int i = 0; i < MAX_DEVICES; i++)
                {
                    pDevAlg[i]->RequestStopThread();
                }

                // Create a code block so the requestWorkLock will be automatically released
                {
                    std::lock_guard<std::mutex> requestWorkLock(requestWorkMutex);
                    // We have the lock, select the layers that should do the work (in this case all)
                    requestWork |= ALL_DEVICES_MASK;
                }

                // Let all sub-Layers know about the request so they can determine if it applies
                requestWorkCondVar.notify_all();

                for (unsigned int i = 0; i < MAX_DEVICES; i++)
                {
                    pDevAlg[i]->ThreadWait();
                    delete pDevAlg[i];
                    pDevAlg[i] = NULL;
                }
            }
            algorithm++;
        }
    }
    catch (cl::sycl::exception const &e) {
        std::cout << "SYCL exception caught during main loop " << e.what() << std::endl;
    }
    catch (std::exception const &e) {
        // catch the exception from devices that are not supported.
        std::cout << "Exception caught during main loop." << std::endl;
        std::cout << e.what() << std::endl;
    }


    // Make the text be in green (see codeproject.com/Tips/5255355/How-to-Put-Color-on-Windows-Console for colors)
    printf("\033[32m");
    printf("All done!  Summary of all runs:\n");
    printf("\033[0m");
    for (auto &element : summaryStats)
    {
        printf("%s\n", element.c_str());
    }
    if (!bInteractive)
    {
        key = cv::waitKeyEx(0);             // Show windows and wait for any key close down
    }

    return 0;
}