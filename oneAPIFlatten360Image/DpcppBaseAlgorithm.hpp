#pragma once

#include <sycl/sycl.hpp>
#include "BaseAlgorithm.hpp"

class DpcppBaseAlgorithm : public BaseAlgorithm {
private:
	sycl::queue* m_pQ;
	std::vector<sycl::_V1::platform> m_platforms;
	std::vector<sycl::_V1::platform>::iterator m_curPlatform;
	std::vector<sycl::_V1::device> m_devices;
	std::vector<sycl::_V1::device>::iterator m_curDevice;
	bool m_bAll;
	std::string m_typePreference;
	std::string m_platformName;
	std::string m_deviceName;
	std::string m_driverVersion;

private:
	sycl::queue* GetNextDeviceQ();
	sycl::queue* GetDeviceQ();

public:
	DpcppBaseAlgorithm(SParameters& parameters);
	DpcppBaseAlgorithm::~DpcppBaseAlgorithm();
	std::string GetDeviceDescription();
	virtual bool StartSubAlgorithm(uint algNum);

};