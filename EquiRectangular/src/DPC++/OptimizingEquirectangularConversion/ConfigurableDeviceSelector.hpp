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

#include <CL/sycl.hpp>
#include <map>

struct SDeviceInfo {
	std::string name;
	std::string driver;
	int rank;

	SDeviceInfo(std::string _name, std::string _driver, int _rank)
	{
		name = _name;
		driver = _driver;
		rank = _rank;
	}
};
class ConfigurableDeviceSelector
{
private:
	static std::map<std::string, int> c_type_preference;
	static std::vector<SDeviceInfo> c_prev_devices;
	static std::string c_platform;
	static std::string c_device_name;
	static std::string c_driver_version;

public:
	// type_preference is a list of preferences in priority order.  The type values can be:
	//   CPU, GPU, ACC (accelerator e.g., FPGA). For instance, to give highest priorty to GPU, 
	//   then CPU, then Accelerator use "GPU,CPU,ACC".  If this is blank, then all device types
	//   are accepted with no prioritization.
	// platform allows specifying what platform to select.  If the provided string is
	//   found anywhere in the platform name, then that platform will be given higher
	//   priority.  If it is not found, the platform/device will be ignored.  Set to "" to consider
	//   all platforms.
	// device_name allows specifying what device to select.  If the provide string is
	//   found anywhere in the device name, then that device will be given higher priority.
	//   If it is not found, the device will be ignored.  Set to "" to consider all
	//   devices.
	// driver_version allows specifying what driver version to select.  If the provide string is
	//   found anywhere in the driver version info, then that device will be given higher priority.
	//   If it is not found, the device will be ignored.  Set to "" to consider all
	//   driver versions.
	// NOTE: The list of possible platforms and devices can be filtered using the
	//   environment variable SYCL_DEVICE_ALLOWLIST
	//   see https://intel.github.io/llvm-docs/EnvironmentVariables.html
	static void set_search(std::string type_preference, std::string platform, std::string device_name, std::string driver_version);

	// Pass the following function to the sycl::queue to use the criteria that were
	// set by set_search to locate the best device to use.
	static int device_selector(const sycl::device& device);

	static void print_platform_info(
		const sycl::platform& platform,
		int col_platform = 0,
		int col_props_value = 30
	);

	static void print_platform_info(
		const sycl::device& device,
		int col_platform = 0,
		int col_props_value = 30
	);

	static void print_device_info(
		const sycl::device& device,
		int col_device_info = 2,
		int col_props = 4,
		int col_props_value = 30
	);

	static std::string str_toupper(std::string s);

	static std::string get_device_type_string(sycl::_V1::device device);

	static std::string get_device_description(
		const sycl::device& device
	);
	// two_col_print will generate a table like output
	// col1_index is zero based column number index (0 is left most) and indicates the
	//   number of spaces to output prior to printing col1_text
	// col1_text is the text to print in the first column
	// col2_index is zero based column number index and indicates the column number where
	//   it is desirable for the col2_text to start.  Note if col1_text extends beyond
	//   col2_index location, then a single space will be added prior to output of col2_text
	// col2_text is the text to print in the second column
	static void two_col_print(int col1_index, std::string col1_text, int col2_index, std::string col2_text);
};
