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

#include <chrono>
#include <mutex>

#include "ParseArgs.hpp"

enum ETimingType {
	TIMING_INITIALIZATION = 0,
	VARIANT_INITIALIZATION,
	TIMING_CREATE_XYZ_COORDS,
	TIMING_CREATE_LON_LAT_COORDS,
	TIMING_CREATE_XY_COORDS,
	TIMING_FRAME_CALCULATIONS,
	TIMING_CREATE_MAP,
	TIMING_REMAP,
	TIMING_IMAGE_EXTRACTION,
	TIMING_FRAME,
	VARIANT_TERMINATION,
	TIMING_TOTAL,
	TIMING_MAX
};

class TimingStats {

private:
	static TimingStats* c_timingStats;

	// m_durationWarmup holds the very first duration for each timing type.  The "warmup" run
	// is often much longer than the subsequent runs since the kernel may need to be compiled, etc.
	std::chrono::duration<double> m_durationWarmup[TIMING_MAX][ALL_STATS];
	int m_warmupIterations[TIMING_MAX][ALL_STATS];
	// m_iterations holds the number of times a given timing type has been reported (minus the warmup run)
	int m_iterations[TIMING_MAX][ALL_STATS];
	// m_durationsSum holds the sum of all the post-warmup runs
	std::chrono::duration<double> m_durationsSum[TIMING_MAX][ALL_STATS];
	// m_lapIterations holds a "lap" counter that helps when looking at instantaneous runs rather than the total
	// run.  For example if the algorithm takes a lot of time to compute when parameters are changed, but less
	// time when flipping frames, then the lap time can be used to show the latest frame while the m_iterations
	// and m_durationsSum will report the overall results including both the runs where parameters were changed
	// and those where they were not.
	int m_lapIterations[TIMING_MAX][ALL_STATS];
	std::chrono::duration<double> m_lapDurationsSum[TIMING_MAX][ALL_STATS];
	std::mutex m_accessMutex;

public:
	static TimingStats* GetTimingStats();

	TimingStats();
	//~TimingStats();

	void Reset();
	void ResetLap();
	void AddIterationResults(ETimingType timingType, unsigned int uiDevIndex, std::chrono::high_resolution_clock::time_point startTime, std::chrono::high_resolution_clock::time_point endTime);
	std::string GetTypeString(ETimingType timingType);
	std::string GetDevString(unsigned int uiDevIndex);
	void ReportTime(std::string strDesc, std::string typeString, std::string devString, std::chrono::duration<double> durationSum, int numIterations, ETimingType timingType);
	void ReportTimes(bool bIncludeLap);
	std::string GetSummaryLine(std::string strDesc, std::string typeString, std::string devString, std::chrono::duration<double> durationSum, int numIterations, ETimingType timingType);
	std::string SummaryStats(bool bIncludeLap = true);
};
