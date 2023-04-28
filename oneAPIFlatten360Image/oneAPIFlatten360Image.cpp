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

#include "ParseArgs.hpp"
#include "SerialRemapping.hpp"
#include "ParallelRemapping.hpp"
#include "DpcppRemapping.hpp"
#include "oneAPIFlatten360Image.h"
#include "TimingStats.hpp"
#include "ConfigurableDeviceSelector.hpp"

using namespace cl::sycl;

// Convience data access definitions
constexpr access::mode dp_read = access::mode::read;
constexpr access::mode dp_write = access::mode::write;

// ARRAY type & data size for use in this example
constexpr size_t array_size = 10000;
typedef std::array<int, array_size> IntArray;

// output message for runtime exceptions
#define EXCEPTION_MSG \
  "    If you are targeting an FPGA hardware, please ensure that an FPGA board is plugged to the system, \n\
        set up correctly and compile with -DFPGA  \n\
    If you are targeting the FPGA emulator, compile with -DFPGA_EMULATOR.\n"

//************************************
// Function description: initialize the array from 0 to array_size-1
//************************************
void initialize_array(IntArray &a) {
  for (size_t i = 0; i < a.size(); i++) a[i] = i;
}

//************************************
// Function description: create a device queue with the default selector or
// explicit FPGA selector when FPGA macro is defined
//    return: DPC++ queue object
//************************************
queue create_device_queue() {
  // create device selector for the device of your interest
#ifdef FPGA_EMULATOR
  // DPC++ extension: FPGA emulator selector on systems without FPGA card
  INTEL::fpga_emulator_selector dselector;
#elif defined(FPGA)
  // DPC++ extension: FPGA selector on systems with FPGA card
  INTEL::fpga_selector dselector;
#else
  // the default device selector: it will select the most performant device
  // available at runtime.
  default_selector dselector;
#endif

  // create an async exception handler so the program fails more gracefully.
  auto ehandler = [](cl::sycl::exception_list exceptionList) {
    for (std::exception_ptr const &e : exceptionList) {
      try {
        std::rethrow_exception(e);
      } catch (cl::sycl::exception const &e) {
        std::cout << "Caught an asynchronous DPC++ exception, terminating the "
                     "program."
                  << std::endl;
        std::cout << EXCEPTION_MSG;
        std::terminate();
      }
    }
  };

  try {
    // create the devices queue with the selector above and the exception
    // handler to catch async runtime errors the device queue is used to enqueue
    // the kernels and encapsulates all the states needed for execution
    queue q(dselector, ehandler);

    return q;
  } catch (cl::sycl::exception const &e) {
    // catch the exception from devices that are not supported.
    std::cout << "An exception is caught when creating a device queue."
              << std::endl;
    std::cout << EXCEPTION_MSG;
    std::terminate();
  }
}

//************************************
// Compute vector addition in DPC++ on device: sum of the data is returned in
// 3rd parameter "sum_parallel"
//************************************
void VectorAddInDPCPP(const IntArray &addend_1, const IntArray &addend_2,
                      IntArray &sum_parallel) {
  queue q = create_device_queue();

  // print out the device information used for the kernel code
  std::cout << "Device: " << q.get_device().get_info<info::device::name>()
            << std::endl;

  // create the range object for the arrays managed by the buffer
  range<1> num_items{array_size};

  // create buffers that hold the data shared between the host and the devices.
  //    1st parameter: pointer of the data;
  //    2nd parameter: size of the data
  // the buffer destructor is responsible to copy the data back to host when it
  // goes out of scope.
  buffer<int, 1> addend_1_buf(addend_1.data(), num_items);
  buffer<int, 1> addend_2_buf(addend_2.data(), num_items);
  buffer<int, 1> sum_buf(sum_parallel.data(), num_items);

  // submit a command group to the queue by a lambda function that
  // contains the data access permission and device computation (kernel)
  q.submit([&](handler &h) {
    // create an accessor for each buffer with access permission: read, write or
    // read/write the accessor is the only mean to access the memory in the
    // buffer.
    auto addend_1_accessor = addend_1_buf.get_access<dp_read>(h);
    auto addend_2_accessor = addend_2_buf.get_access<dp_read>(h);

    // the sum_accessor is used to store (with write permision) the sum data
    auto sum_accessor = sum_buf.get_access<dp_write>(h);

    // Use parallel_for to run array addition in parallel on device. This
    // executes the kernel.
    //    1st parameter is the number of work items to use
    //    2nd parameter is the kernel, a lambda that specifies what to do per
    //    work item. the parameter of the lambda is the work item id of the
    //    current item.
    // DPC++ supports unnamed lambda kernel by default.
    h.parallel_for(num_items, [=](id<1> i) {
      sum_accessor[i] = addend_1_accessor[i] + addend_2_accessor[i];
    });
  });

  // q.submit() is an asynchronously call. DPC++ runtime enqueues and runs the
  // kernel asynchronously. at the end of the DPC++ scope the buffer's data is
  // copied back to the host.
}

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
    const int width = parameters.m_widthOutput;
    const int height = parameters.m_heightOutput;
    const int indexes = 3;

    std::string description;
    BaseAlgorithm* pAlg = NULL;
    int prevAlg = -1;
    std::chrono::system_clock::time_point initStartTime;
    std::chrono::system_clock::time_point initEndTime;
    std::chrono::system_clock::time_point frameStartTime;
    std::chrono::system_clock::time_point frameEndTime;
    std::chrono::system_clock::time_point totalTimeStart;
    std::chrono::system_clock::time_point totalTimeEnd;
    std::chrono::system_clock::time_point extractionStartTime;
    std::chrono::system_clock::time_point startTime = std::chrono::system_clock::now();
    TimingStats *pTimingStats = TimingStats::GetTimingStats();
    bool bInteractive = parameters.m_iterations <= 1;
    int iteration = 0;
    SParameters prevParameters;
    int startAlgorithm;
    int endAlgorithm;
    int key;
    bool bDebug = false;
    bool bRunning = true;
    bool bEnteringData = false;
    cv::Mat debugImg;
    cv::Mat flatImg;
    int delta = 10;
    uint subAlg;
    bool bActiveSubAlg;


    if (parameters.m_algorithm < 0)
    {
        startAlgorithm = 0;
        endAlgorithm = 3;
        if (bInteractive)
        {
            bInteractive = false;
            parameters.m_iterations = 1;
        }
    }
    else
    {
        startAlgorithm = parameters.m_algorithm;
        endAlgorithm = startAlgorithm + 1;
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
    // read src image1
    parameters.m_image[0] = cv::imread(parameters.m_imgFilename[0], cv::IMREAD_COLOR);
    if (parameters.m_image[0].empty())
    {
        printf("Error: Could not load image 1 from %s\n", parameters.m_imgFilename[0]);
        throw std::invalid_argument("Error: Could not load image 1.");
    }
    // read src image2
    parameters.m_image[1] = cv::imread(parameters.m_imgFilename[1], cv::IMREAD_COLOR);
    if (parameters.m_image[1].empty())
    {
        printf("Error: Could not load image 2 from %s\n", parameters.m_imgFilename[1]);
        throw std::invalid_argument("Error: Could not load image 2.");
    }
    for (int algorithm = startAlgorithm; algorithm < endAlgorithm; algorithm++)
    {
        subAlg = 0;
        parameters.m_algorithm = algorithm;
        do
        {
            pTimingStats->Reset();
            iteration = 0;
            if (prevAlg != algorithm)
            {
                initStartTime = std::chrono::system_clock::now();
                switch (algorithm)
                {
                case 0:
                    pAlg = new SerialRemapping(parameters, E_STORE_ROW_COL);
                    break;
                case 1:
                    pAlg = new SerialRemapping(parameters, E_STORE_COL_ROW);
                    break;
                case 2:
                    // TODO: Implement
                    pAlg = new DpcppRemapping(parameters);
                    break;
                case 3:
                    // TODO: Implement
                    //pAlg = new ParallelRemapping(parameters);
                    break;
                case 4:
                    // TODO: Implement
                    //pAlg = new SerialRemapping(parameters, E_STORE_SOA);
                    break;
                case 5:
                    // TODO: Implement
                    break;
                case 6:
                    // TODO: Implement
                    break;
                }
                pTimingStats->AddIterationResults(ETimingType::TIMING_INITIALIZATION, initStartTime, std::chrono::system_clock::now());
                prevAlg = algorithm;
            }

            if (prevParameters.m_imageIndex != parameters.m_imageIndex)
            {
                cv::namedWindow("Equirectangular original", cv::WINDOW_NORMAL | cv::WINDOW_KEEPRATIO);
                cv::imshow("Equirectangular original", parameters.m_image[parameters.m_imageIndex]);
                prevParameters.m_imageIndex = parameters.m_imageIndex;
            }

            bActiveSubAlg = pAlg->StartSubAlgorithm(subAlg++);
            while (bActiveSubAlg)
            {
                pTimingStats->ResetLap();
                totalTimeStart = std::chrono::system_clock::now();
                do
                {
                    frameStartTime = std::chrono::system_clock::now();
                    if (prevParameters != parameters)
                    {
                        prevParameters = parameters;
                        PrintParameters(&parameters);
                        pAlg->FrameCalculations();
                        pTimingStats->AddIterationResults(ETimingType::TIMING_FRAME_CALCULATIONS, frameStartTime, std::chrono::system_clock::now());
                    }
                    extractionStartTime = std::chrono::system_clock::now();
                    flatImg = pAlg->ExtractFrameImage();
                    frameEndTime = std::chrono::system_clock::now();
                    pTimingStats->AddIterationResults(ETimingType::TIMING_IMAGE_EXTRACTION, extractionStartTime, frameEndTime);
                    pTimingStats->AddIterationResults(ETimingType::TIMING_FRAME, frameStartTime, frameEndTime, bInteractive);
                    iteration++;
                } while (iteration < parameters.m_iterations);
                totalTimeEnd = std::chrono::system_clock::now();

                description = pAlg->GetDescription();
                if (bInteractive)
                {
                    char windowText[80];

                    sprintf(windowText, "Algorithm %d Flat View %s", parameters.m_algorithm, description.c_str());

                    cv::imshow(windowText, flatImg);
                }
                else
                {
                    pTimingStats->AddIterationResults(ETimingType::TIMING_TOTAL, totalTimeStart, totalTimeEnd);
                }

                description = pAlg->GetDescription();
                printf("Algorithm description: %s\n", description.c_str());
                pTimingStats->ReportTimes(true);

                if (bDebug)
                {
                    // When debugging, we want to show the original image with an outline of the Region of Interest
                    // The color is BGR
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
                            delta = delta * 10 + (key - 48);
                        }
                        else
                        {
                            bEnteringData = true;
                            delta = key - 48;
                        }
                    }
                    else
                    {
                        bEnteringData = false;
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
                        else if (key == 43)         // + key
                        {
                            parameters.m_fov -= delta;
                        }
                        else if (key == 45)         // - key
                        {
                            parameters.m_fov += delta;
                        }
                        else if (key == 100)        // d key (toggle debug mode)
                        {
                            bDebug = !bDebug;
                        }
                        else if (key == 102)        // f key(change frame)
                        {
                            parameters.m_imageIndex = (parameters.m_imageIndex + 1) % 2;
                        }
                        else if (key == 27 || key == 113)   // Esc or q key to quit
                        {
                            bActiveSubAlg = pAlg->StartSubAlgorithm(subAlg++);
                            bRunning = bActiveSubAlg;
                        }
                        else
                        {
                            printf("Unassigned keystroke = %d\n", key);
                        }

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
                            parameters.m_yaw = -180 + parameters.m_yaw - 180;
                        }
                        else if (parameters.m_yaw < -180)
                        {
                            parameters.m_yaw = 180 + parameters.m_yaw + 180;
                        }
                        if (parameters.m_roll < 0)
                        {
                            parameters.m_roll = 360 + parameters.m_roll;
                        }
                        else if (parameters.m_roll >= 360)
                        {
                            parameters.m_roll -= 360;
                        }
                    }
                }
                else
                {
                    bActiveSubAlg = pAlg->StartSubAlgorithm(subAlg++);
                    pTimingStats->Reset();
                    iteration = 0;
                }
            }
        } while (bInteractive && bRunning);	// Stop looping when they hit Esc or q

        delete pAlg;
        pAlg = NULL;
    }

    // Make the text be in green (see codeproject.com/Tips/5255355/How-to-Put-Color-on-Windows-Console for colors)
    printf("\033[32m");
    printf("All done!\n");
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