// Copyright (C) 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <sycl/sycl.hpp>
#include "BaseAlgorithm.hpp"
#include "Point2D.hpp"
//#include "Point3D.hpp"

const int STORAGE_TYPE_INIT = -1;
const int STORAGE_TYPE_USM = 0;
const int STORAGE_TYPE_DEVICE = 1;
const int STORAGE_TYPE_MAX = 2;

class DpcppBaseAlgorithm : public BaseAlgorithm {
private:
	std::vector<sycl::_V1::platform> m_platforms;
	std::vector<sycl::_V1::platform>::iterator m_curPlatform;
	std::vector<sycl::_V1::device> m_devices;
	std::vector<sycl::_V1::device>::iterator m_curDevice;
	bool m_bAll;
	bool m_bInitRequired;
	std::string m_typePreference;
	std::string m_platformName;
	std::string m_deviceName;
	std::string m_driverVersion;

private:
	sycl::queue* GetNextDeviceQ();
	sycl::queue* GetDeviceQ();

protected:
	sycl::queue* m_pQ;
	Point2D* m_pXYPoints = NULL;

public:
	DpcppBaseAlgorithm(SParameters& parameters);
	virtual ~DpcppBaseAlgorithm();
	std::string GetDeviceDescription();

	virtual cv::Mat GetDebugImage();

	virtual bool StartVariant();
	virtual void StopVariant();

};