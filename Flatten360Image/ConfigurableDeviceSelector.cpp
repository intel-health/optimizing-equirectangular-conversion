#include <algorithm>

#include "ConfigurableDeviceSelector.hpp"

std::string ConfigurableDeviceSelector::c_platform = "";
std::string ConfigurableDeviceSelector::c_device_name = "";
std::string ConfigurableDeviceSelector::c_driver_version = "";
std::map<std::string, int> ConfigurableDeviceSelector::c_type_preference = std::map<std::string, int>();
std::vector<SDeviceInfo> ConfigurableDeviceSelector::c_prev_devices = std::vector<SDeviceInfo>();

void ConfigurableDeviceSelector::set_search(std::string type_preference, std::string platform, std::string device_name, std::string driver_version)
{
	int value = 90000;

	c_type_preference.clear();
	c_prev_devices.clear();
	if (type_preference != "")
	{
		std::string strSearch = str_toupper(type_preference);
		const char* search = strSearch.c_str();
		char* pType = strtok((char *)search, ";");
		while (pType != NULL) 
		{
			c_type_preference.insert_or_assign(pType, value);
			value -= 10000;
			pType = strtok(NULL, ";");
		}
	}
	c_platform = platform;
	c_device_name = device_name;
	c_driver_version = driver_version;
}

// Pass the following function to the sycl::queue to use the criteria that were
// set by set_search to locate the best device to use.
int ConfigurableDeviceSelector::device_selector(const sycl::device& device)
{
	std::string platform_name = device.get_platform().get_info<sycl::info::platform::name>();
	std::string device_name = device.get_info<sycl::info::device::name>();
	std::string driver_version = device.get_info<sycl::info::device::driver_version>();
	int retVal = -2;

	if (c_platform != "" || c_device_name != "" || c_driver_version != "")
	{
		if (platform_name.find(c_platform) != std::string::npos &&
			device_name.find(c_device_name) != std::string::npos &&
			driver_version.find(c_driver_version) != std::string::npos)
		{
			{
				retVal = 100000;
			}
		}
		else
		{
			retVal = -1;
		}
	}
	else
	{
		retVal = 0;
	}
	// If the above filter specifically says (retVal = -1) to skip this device,
	// then we are done.  Else run the type_preference filtering.
	if (retVal != -1)
	{
		if (!c_type_preference.empty())
		{
			auto iter = c_type_preference.find(get_device_type_string(device));
			if (iter != c_type_preference.end())
			{
				retVal += iter->second;
			}
			else
			{
				retVal = -1;
			}
		}
	}

	// Select higher compute units over lower ones
	if (retVal >= 0)
	{
		retVal += device.get_info<sycl::info::device::max_compute_units>();
		// Give preference to newer drivers (this assumes that driver_versions
		// can be compared with strcmp and newer versions will be >).
		std::string key = platform_name + device_name;

		auto iter = find_if(c_prev_devices.begin(), c_prev_devices.end(), 
			[key](SDeviceInfo elem) { return elem.name == key; }
		);

		if (iter != c_prev_devices.end())
		{
			if ((*iter).driver < driver_version)
			{
				(*iter).driver = driver_version;
				retVal = (*iter).rank + 1;
				(*iter).rank = retVal;
			}
		}
		else
		{
			c_prev_devices.emplace_back(key, driver_version, retVal);
		}
	}
//#define SHOW_RESULTS
#ifdef SHOW_RESULTS
	std::cout << "  device_selector retVal = " << retVal << " for platform = " << platform_name << " device name = " << device_name << " driver_version = " << driver_version << std::endl;
#endif
	return retVal;

}

void ConfigurableDeviceSelector::print_platform_info(
	const sycl::platform& platform,
	int col_platform /* = 0 */,
	int col_props_value /* = 30 */
)
{
	two_col_print(col_platform, "Platform:", col_props_value, platform.get_info<sycl::info::platform::name>());
}


void ConfigurableDeviceSelector::print_platform_info(
	const sycl::device& device,
	int col_platform /* = 0 */,
	int col_props_value /* = 30 */
)
{
	print_platform_info(device.get_platform(), col_platform, col_props_value);
}

void ConfigurableDeviceSelector::print_device_info(
	const sycl::device& device, 
	int col_device_info /* = 2 */,
	int col_props /* = 4 */,
	int col_props_value /* = 30 */
)
{
	two_col_print(col_device_info, "Device information:", 0, "");
	two_col_print(col_props, "vendor:", col_props_value, device.get_info<sycl::info::device::vendor>());
	two_col_print(col_props, "name:", col_props_value, device.get_info<sycl::info::device::name>());
	two_col_print(col_props, "type:", col_props_value, get_device_type_string(device));
	two_col_print(col_props, "version:", col_props_value, device.get_info<sycl::info::device::version>());
	two_col_print(col_props, "driver_version:", col_props_value, device.get_info<sycl::info::device::driver_version>());
	two_col_print(col_props, "max_compute_units:", col_props_value, std::to_string(device.get_info<sycl::info::device::max_compute_units>()));
	two_col_print(col_props, "address_bits:", col_props_value, std::to_string(device.get_info<sycl::info::device::address_bits>()));
	two_col_print(col_props, "error_correction_support:", col_props_value, std::to_string(device.get_info<sycl::info::device::error_correction_support>()));
	//two_col_print(col_props, "vendor_id:", col_props_value, std::to_string(device.get_info<sycl::info::device::vendor_id>()));
	//two_col_print(col_props, "profile:", col_props_value, device.get_info<sycl::info::device::profile>());
}

std::string ConfigurableDeviceSelector::get_device_type_string(sycl::_V1::device device)
{
	std::string strType = "Unknown";

	switch (device.get_info<sycl::info::device::device_type>())
	{
	case sycl::info::device_type::cpu:
		strType = "CPU";
		break;
	case sycl::info::device_type::gpu:
		strType = "GPU";
		break;
	case sycl::info::device_type::accelerator:
		strType = "ACC";
		break;
	case sycl::info::device_type::custom:
		strType = "Custom";
		break;
	case sycl::info::device_type::automatic:
		strType = "Automatic";
		break;
	case sycl::info::device_type::host:
		strType = "Host";
		break;
	case sycl::info::device_type::all:
		strType = "All";
		break;
	}

	return strType;
}

void ConfigurableDeviceSelector::two_col_print(int col1_index, std::string col1_text, int col2_index, std::string col2_text)
{
	int numSpaces = col2_index - col1_index - col1_text.length();

	if (col1_index > 0)
	{
		std::cout << std::string(col1_index, ' ');
	}
	std::cout << col1_text;
	if (numSpaces > 0)
	{
		std::cout << std::string(numSpaces, ' ');
	}
	else
	{
		std::cout << ' ';
	}
	std::cout << col2_text << std::endl;
}

std::string ConfigurableDeviceSelector::str_toupper(std::string s) {
	std::transform(s.begin(), s.end(), s.begin(),
		[](unsigned char c) { return std::toupper(c); }
	);
	return s;
}

std::string ConfigurableDeviceSelector::get_device_description(
	const sycl::device& device
)
{
	return device.get_platform().get_info<sycl::info::platform::name>() + " " + device.get_info<sycl::info::device::name>() + " " + device.get_info<sycl::info::device::driver_version>();
}

