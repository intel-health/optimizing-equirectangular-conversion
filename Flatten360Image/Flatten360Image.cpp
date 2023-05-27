//==============================================================
// Copyright © 2020 Intel Corporation
//
// SPDX-License-Identifier: MIT
// =============================================================
#include <CL/sycl.hpp>
#if defined(FPGA) || defined(FPGA_EMULATOR)
#include <CL/sycl/INTEL/fpga_extensions.hpp>
#endif
#include <array>
#include <iostream>
#include <opencv2/core/ocl.hpp>

#include "ParseArgs.hpp"
#include "Equi2Rect.hpp"
#include "SerialRemapping.hpp"
#include "SerialRemappingV2.hpp"
#include "ParallelRemapping.hpp"
#include "DpcppRemapping.hpp"
#include "DpcppRemappingV2.hpp"
#include "DpcppRemappingV3.hpp"
#include "DpcppRemappingV4.hpp"
#include "DpcppRemappingV5.hpp"
#include "DpcppRemappingV6.hpp"
#include "DpcppRemappingV7.hpp"
#include "Flatten360Image.h"
#include "TimingStats.hpp"
#include "ConfigurableDeviceSelector.hpp"

using namespace cl::sycl;

#if 0
// Convience data access definitions
constexpr access::mode dp_read = access::mode::read;
constexpr access::mode dp_write = access::mode::write;

// ARRAY type & data size for use in this example
constexpr size_t array_size = 10000;
typedef std::array<int, array_size> IntArray;
#endif

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

// output message for runtime exceptions
#define EXCEPTION_MSG \
  "    If you are targeting an FPGA hardware, please ensure that an FPGA board is plugged to the system, \n\
        set up correctly and compile with -DFPGA  \n\
    If you are targeting the FPGA emulator, compile with -DFPGA_EMULATOR.\n"

//************************************
// Demonstrate summation of arrays both in scalar on CPU and parallel on device
//************************************
int main(int argc, char** argv) {
    SParameters parameters;
    char errorMessage[MAX_ERROR_MESSAGE];

    InitializeParameters(&parameters);
    if (!ParseArgs(argc, argv, &parameters, errorMessage))
    {
        PrintUsage(argv[0], errorMessage);
        exit(1);
    }

    std::string description;
    BaseAlgorithm* pAlg = NULL;
    std::chrono::system_clock::time_point initStartTime;
    std::chrono::system_clock::time_point initEndTime;
    std::chrono::system_clock::time_point variantInitStartTime;
    std::chrono::system_clock::time_point variantInitStopTime;
    std::chrono::system_clock::time_point frameStartTime;
    std::chrono::system_clock::time_point frameEndTime;
    std::chrono::system_clock::time_point totalTimeStart;
    std::chrono::system_clock::time_point totalTimeEnd;
    std::chrono::system_clock::time_point extractionStartTime;
    TimingStats *pTimingStats = TimingStats::GetTimingStats();
    bool bInteractive = parameters.m_iterations <= 1;
    int iteration = 0;
    SParameters prevParameters;
    int startAlgorithm;
    int endAlgorithm;
    int key;
    bool bDebug = false;
    bool bEnteringData = false;
    bool bDoIterations = true;
    int sign = 1;
    cv::Mat debugImg;
    cv::Mat flatImg;
    int delta = 10;
    int prevDelta = 10;
    bool bRunningVariant;
    bool bVariantValid;
    std::vector<std::string> summaryStats;

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


//#define CHECK_OPENCL
#ifdef CHECK_OPENCL
    if (!cv::ocl::haveOpenCL())
    {
        std::cout << "OpenCL is not available..." << std::endl;
        //return;
    }

    cv::ocl::Context context;
    if (!context.create(cv::ocl::Device::TYPE_ALL))
    {
        std::cout << "Failed creating the context..." << std::endl;
        //return;
    }

    std::cout << context.ndevices() << " CPU devices are detected." << std::endl; //This bit provides an overview of the OpenCL devices you have in your computer
    for (int i = 0; i < context.ndevices(); i++)
    {
        cv::ocl::Device device = context.device(i);
        std::cout << "name:              " << device.name() << std::endl;
        std::cout << "available:         " << device.available() << std::endl;
        std::cout << "imageSupport:      " << device.imageSupport() << std::endl;
        std::cout << "OpenCL_C_Version:  " << device.OpenCL_C_Version() << std::endl;
        std::cout << std::endl;
    }

    cv::ocl::Device(context.device(0)); //Here is where you change which GPU to use (e.g. 0 or 1)
#endif


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
    // read src image1
    printf("Loading Image1\n");
    parameters.m_image[0] = cv::imread(parameters.m_imgFilename[0], cv::IMREAD_COLOR);
    if (parameters.m_image[0].empty())
    {
        printf("Error: Could not load image 1 from %s\n", parameters.m_imgFilename[0]);
        throw std::invalid_argument("Error: Could not load image 1.");
    }
    // read src image2
    printf("Loading Image2\n");
    parameters.m_image[1] = cv::imread(parameters.m_imgFilename[1], cv::IMREAD_COLOR);
    if (parameters.m_image[1].empty())
    {
        printf("Error: Could not load image 2 from %s\n", parameters.m_imgFilename[1]);
        throw std::invalid_argument("Error: Could not load image 2.");
    }
    printf("Images loaded.\n");
    int algorithm = startAlgorithm;
    try {

        while (algorithm <= endAlgorithm)
        {
            initStartTime = std::chrono::system_clock::now();
            switch (algorithm)
            {
            case 0:
                pAlg = new Equi2Rect(parameters);
                break;
            case 1:
                pAlg = new SerialRemapping(parameters);
                break;
            case 2:
                pAlg = new SerialRemappingV2(parameters);
                break;
            case 3:
                pAlg = new DpcppRemapping(parameters);
                break;
            case 4:
                pAlg = new DpcppRemappingV2(parameters);
                break;
            case 5:
                pAlg = new DpcppRemappingV3(parameters);
                break;
            case 6:
                pAlg = new DpcppRemappingV4(parameters);
                break;
            case 7:
                pAlg = new DpcppRemappingV5(parameters);
                break;
            case 8:
                pAlg = new DpcppRemappingV6(parameters);
                break;
            case 9:
                pAlg = new DpcppRemappingV7(parameters);
                break;
            }

            initEndTime = std::chrono::system_clock::now();

            bVariantValid = true;
            while (bVariantValid)
            {
                variantInitStartTime = std::chrono::system_clock::now();
                bVariantValid = pAlg->StartVariant();

                if (bVariantValid)
                {
                    pTimingStats->Reset();
                    iteration = 0;

                    pTimingStats->AddIterationResults(ETimingType::TIMING_INITIALIZATION, initStartTime, initEndTime);
                    pTimingStats->AddIterationResults(ETimingType::VARIANT_INITIALIZATION, variantInitStartTime, std::chrono::system_clock::now());
                    bRunningVariant = true;
                    totalTimeStart = std::chrono::system_clock::now();
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

                                    bool bParametersChanged = prevParameters != parameters;
                                    //                                if (bParametersChanged)
                                    //                                {
                                    //#ifdef VTUNE_API
                                    //                                    __itt_task_begin(pittTests_domain, __itt_null, __itt_null, handle_print_parameters);
                                    //#endif
                                    //                                    PrintParameters(&parameters);
                                    //#ifdef VTUNE_API
                                    //                                    __itt_task_end(pittTests_domain);
                                    //#endif
                                    //                                }

                                    frameStartTime = std::chrono::system_clock::now();
                                    pAlg->FrameCalculations(bParametersChanged);
                                    pTimingStats->AddIterationResults(ETimingType::TIMING_FRAME_CALCULATIONS, frameStartTime, std::chrono::system_clock::now());
                                    prevParameters = parameters;
                                    extractionStartTime = std::chrono::system_clock::now();
                                    flatImg = pAlg->ExtractFrameImage();
                                    frameEndTime = std::chrono::system_clock::now();
                                    pTimingStats->AddIterationResults(ETimingType::TIMING_IMAGE_EXTRACTION, extractionStartTime, frameEndTime);
                                    pTimingStats->AddIterationResults(ETimingType::TIMING_FRAME, frameStartTime, frameEndTime, bInteractive);
                                    iteration++;
                                    if (!bInteractive)
                                    {
                                        parameters.m_yaw += parameters.m_deltaYaw;
                                        parameters.m_pitch += parameters.m_deltaPitch;
                                        parameters.m_roll += parameters.m_deltaRoll;
                                        if (parameters.m_deltaImage)
                                        {
                                            parameters.m_imageIndex = (parameters.m_imageIndex + 1) % 2;
                                        }
                                    }
                                } while (iteration < parameters.m_iterations);
#ifdef VTUNE_API
                                __itt_pause();
#endif
                                totalTimeEnd = std::chrono::system_clock::now();

                                description = pAlg->GetDescription();
                                if (bInteractive)
                                {
                                    char windowText[1024];

                                    sprintf(windowText, "Algorithm %d Flat View %s", algorithm, description.c_str());

                                    cv::imshow(windowText, flatImg);
                                }
                                else
                                {
                                    pTimingStats->AddIterationResults(ETimingType::TIMING_TOTAL, totalTimeStart, totalTimeEnd);
                                }

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

                        if (bDebug)
                        {
                            // When debugging, we want to show the original image with an outline of the Region of Interest
                            // The color is BGR format
                            debugImg = pAlg->GetDebugImage();

                            cv::namedWindow("Debug View", cv::WINDOW_NORMAL);
                            cv::imshow("Debug View", debugImg);

                        }
                        if (bInteractive)
                        {
                            key = cv::waitKeyEx(0);

                            if (key >= 48 && key <= 57)
                            {
                                if (bEnteringData)
                                {
                                    delta = sign * (abs(delta) * 10 + (key - 48));
                                }
                                else
                                {
                                    bEnteringData = true;
                                    prevDelta = delta;
                                    delta = sign * (key - 48);
                                }
                                bDoIterations = false;
                            }
                            else if (key == 45)             // Minus key (-)
                            {
                                delta = 0;
                                sign = -1;
                                bDoIterations = false;
                            }
                            else
                            {
                                bEnteringData = false;
                                sign = 1;
                                if (key == 2555904)         // Right arrow key
                                {
                                    parameters.m_yaw += delta;
                                }
                                else if (key == 2424832)    // Left arrow key
                                {
                                    parameters.m_yaw -= delta;
                                }
                                else if (key == 2621440)    // Down arrow key
                                {
                                    parameters.m_pitch -= delta;
                                }
                                else if (key == 2490368)    // Up arrow key
                                {
                                    parameters.m_pitch += delta;
                                }
                                else if (key == 2162688)    // Page Up key (roll right upwards)
                                {
                                    parameters.m_roll -= delta;
                                }
                                else if (key == 2228224)    // Page Down key (roll right downwards)
                                {
                                    parameters.m_roll += delta;
                                }
                                else if (key == 2359296)    // Home key (roll left upwards)
                                {
                                    parameters.m_roll += delta;
                                }
                                else if (key == 2293760)    // End key (roll left downwards)
                                {
                                    parameters.m_roll -= delta;
                                }
                                else if (key == 42)         // * key
                                {
                                    parameters.m_fov -= delta;
                                }
                                else if (key == 47)         // / key
                                {
                                    parameters.m_fov += delta;
                                }
                                else if (key == 100)        // d key (toggle debug mode)
                                {
                                    bDebug = !bDebug;
                                    if (!bDebug)
                                    {
                                        cv::destroyWindow("Debug View");
                                    }
                                    bDoIterations = false;
                                }
                                else if (key == 102)        // f key (change frame)
                                {
                                    parameters.m_imageIndex = (parameters.m_imageIndex + 1) % 2;
                                }
                                else if (key == 97)         // a key (algorithm selection)
                                {
                                    // Adjust the variables so we drop out of both while loops and
                                    // come back into the for loop with the correct algorithm
                                    algorithm = delta - 1;
                                    endAlgorithm = delta;
                                    bVariantValid = false;
                                    bRunningVariant = false;
                                    // Treat delta as temporary and restore the previous delta value
                                    delta = prevDelta;
                                }
                                else if (key == 112)        // p key (set pitch to temporary delta)
                                {
                                    parameters.m_pitch = delta;
                                    // Treat delta as temporary and restore the previous delta value
                                    delta = prevDelta;
                                }
                                else if (key == 114)        // r key (set roll to temporary delta)
                                {
                                    parameters.m_roll = delta;
                                    // Treat delta as temporary and restore the previous delta value
                                    delta = prevDelta;
                                }
                                else if (key == 121)        // y key (set yaw to temporary delta)
                                {
                                    parameters.m_yaw = delta;
                                    // Treat delta as temporary and restore the previous delta value
                                    delta = prevDelta;
                                }
                                else if (key == 27 || key == 113)   // Esc or q key to quit
                                {
                                    // Stop running the variant and move on
                                    bRunningVariant = false;
                                }
                                else
                                {
                                    printf("Unassigned keystroke = %d\n", key);
                                }
                                prevDelta = delta;
                            }
                        }
                        else
                        {
                            bRunningVariant = false;
                        }
                    }

                    variantInitStopTime = std::chrono::system_clock::now();
                    pAlg->StopVariant();
                    pTimingStats->AddIterationResults(ETimingType::VARIANT_TERMINATION, variantInitStopTime, std::chrono::system_clock::now());
                    summaryStats.push_back(description + "\n" + pTimingStats->SummaryStats(false));
                }
            }

            delete pAlg;
            pAlg = NULL;
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
    printf("\033[32m");
    printf("It is possible that the exit will hang due to a driver not unloading properly.\n");
    printf("It is safe to force the program to stop since no more computing is being done.\n");
    printf("\033[0m");
    if (!bInteractive)
    {
        key = cv::waitKeyEx(0);             // Show windows and wait for any key close down
    }
#if 0
    // create int array objects with "array_size" to store the input and output
  // data
  IntArray addend_1, addend_2, sum_scalar, sum_parallel;

  // Initialize input arrays with values from 0 to array_size-1
  initialize_array(addend_1);
  initialize_array(addend_2);

  // Compute vector addition in DPC++
  VectorAddInDPCPP(addend_1, addend_2, sum_parallel);

  // Computes the sum of two arrays in scalar for validation
  for (size_t i = 0; i < sum_scalar.size(); i++)
    sum_scalar[i] = addend_1[i] + addend_2[i];

  // Verify that the two sum arrays are equal
  for (size_t i = 0; i < sum_parallel.size(); i++) {
    if (sum_parallel[i] != sum_scalar[i]) {
      std::cout << "fail" << std::endl;
      return -1;
    }
  }
  std::cout << "success" << std::endl;
#endif

  return 0;
}