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
	// Class function that will start a thread for a given instance
	// of this class and call the object's 
	//static unsigned __stdcall startThreadProc(void *);

private:
	sycl::queue* GetNextDeviceQ();
	sycl::queue* GetDeviceQ();
	// Function to run in the sub-thread
	unsigned int threadFunc();

protected:
	sycl::queue* m_pQ;
	Point2D* m_pXYPoints = NULL;
	std::thread *m_pThread = NULL;
	bool m_bThreadStopRequested;

public:
	DpcppBaseAlgorithm(SParameters& parameters);
	virtual ~DpcppBaseAlgorithm();
	std::string GetDeviceDescription();

	virtual cv::Mat GetDebugImage();

	virtual bool StartVariant();
	virtual void StopVariant();

	void StartThread();
	bool RequestStopThread();
	void ThreadWait();

};