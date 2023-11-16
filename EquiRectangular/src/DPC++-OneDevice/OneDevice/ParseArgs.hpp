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

#pragma once

#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif

#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

const int MAX_PATH = 1024;
const int MAX_ERROR_MESSAGE = 1024;
const double DEGREE_CONVERSION_FACTOR = 2 * M_PI / 360.0;
const int MAX_ALGORITHM = 11;

typedef struct _SParameters {
	// m_algorithm defines the algorithm to use during the current run of the program
	int			m_algorithm;
	// m_startAlgorithm can be used to define an algorithm number to begin and then the program
	// will run each algorithm from that point to m_endAlgorithm.
	int			m_startAlgorithm;
	// m_endAlgorithm can be used to run up to and including the m_endAlgorithm.  This is useful
	// for running through a range of algorithms from m_startAlgorithm to m_endAlgorithm (inclusive).
	int			m_endAlgorithm;
	// m_yaw defines the yaw of the viewer's perspective (moving the camera lens left or right direction).
	// Also sometimes referred to as theta or pan.
	// This can run from -180 to 180.  Negative values are to the left of center and positive to the right.  0 is 
	// straight ahead (center of equirectangular image)
	int			m_yaw;
	// m_pitch defines the pitch of the viewer's perspective (up or down).  
	// Sometimes referred to as phi or tilt.
	// This can run from -180 to 180 degrees.  0 is straight ahead.  Positive to 90 is moving upward until straight up.
	// Positive from 90 to 180 the camera is moving upside down and facing behind until it is fully upside down and
	// facing directly behind.  Negative to -90 is moving downward until straight down.  Negative from -90 to -180
	// causes the camera to be upside down until it is fully upside down and facing directly behind.
	int			m_pitch;
	// m_roll defines how level the left and right sides of the camera are.  This can run from 0 to 360.  The rotation is counter clockwise.
	// 0 the left and right sides of the camera are level (the original picture may not be level, but the perspective on the image
	// is level).  90 degrees lifts the right side of the "camera" to be on top.  180 will flip the "camera"
	// upside down.  270 will place the left side of the camera on top.
	int			m_roll;
	// The m_delta* variables hold the amount to add to the current setting during each iteration.  This can be used
	// to force recalculations for algorithms that cache information related to the yaw, pitch, and roll settings.
	int			m_deltaYaw;
	int			m_deltaPitch;
	int			m_deltaRoll;
	// m_deltaImage tells whether to toggle between the two loaded images with each iteration to simulate a
	// video stream
	bool		m_deltaImage;
	// m_fov holds the field of view in degrees from 10 to 120.
	int			m_fov;
	// m_imgFilename holds the path to the images to load.  This should be an equirectangular (360) image.
	char		m_imgFilename[2][MAX_PATH];
	// m_widthOutput is the width of the output image to produce if save is requested
	int			m_widthOutput;
	// m_heightOutput is the width of the output image to produce if save is requested
	int			m_heightOutput;
	// m_iterations is the number of times to automatically iterate over the algorithm for averaging the time required
	int			m_iterations;
	// m_offsets captures the amount of offset to add automatically each iteration (only used if m_iterations is positive).
	// Index 0 is yaw, index 1 is pitch, index 2 is roll.
	int			m_offsets[3];
	// m_image holds an array of images.  This is used to simulate frames from a video (only 2, but can flip back and forth between them)
	cv::Mat		m_image[2];
	// m_imageIndex holds the index into m_image that is the current image being used.
	int			m_imageIndex;
	// m_typePreference provides a way to specify the type of the device to select when
	// running.  This can be "" to allow any type to be selected or it can be CPU, GPU, or
	// ACC (accelerator such as FPGA).  It can also be a semi-colon (;) separated priority
	// list.  Thus, GPU;CPU will prioritize any GPUs that are found and only select a CPU
	// if no GPUs are in the system.  If this is set to "", then any of the types
	// will be considered depending on how the other parameters are set.
	std::string m_typePreference;
	// m_platformName provides a mechanism to specify a specific platform name to use when
	// looking for devices.  This can have strings like "Intel(R) Level-Zero" or
	// "Intel(R) OpenCL" to select from the given platform name.  This can also be
	// "all" to select devices from all the platforms or "list" to list all platform names
	// that are present on the current system.  If this is set to "", then any of the platforms
	// will be considered depending on how the other parameters are set.
	std::string m_platformName;
	// m_deviceName provides a mechanism to specify a device name to use when using
	// oneAPI.  This can have strings like "Intel(R) Core(TM) i7-8665U CPU @ 1.90GHz" or
	// "Intel(R) UHD Graphics 620" or just substrings from the device name such as "Graphics"
	// This can also be "list" to list the available devices or "all" to run on all devices
	// that were found.  If this is set to "", then any of the devices
	// will be considered depending on how the other parameters are set.
	std::string m_deviceName;
	// m_driverVersion is a filter that will restrict the selected device to one that has
	// the given version number.  If this is set to "", then any of the versions
	// will be considered depending on how the other parameters are set.
	std::string m_driverVersion;
	bool m_bShowFrames;

	_SParameters();

	bool operator!=(const struct _SParameters& a) const
	{
		return (a.m_algorithm != m_algorithm || a.m_yaw != m_yaw || m_pitch != a.m_pitch || m_roll != a.m_roll || m_fov != a.m_fov || m_widthOutput != a.m_widthOutput || m_heightOutput != a.m_heightOutput);
	}
} SParameters;


void InitializeParameters(SParameters *parameters);
bool ParseArgs(int argc, char **argv, SParameters *parameters, char *errorMessage);
void PrintUsage(char *pProgramName, char *pMessage);
void PrintParameters(SParameters* parameters);
