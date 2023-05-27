#pragma once

#include <chrono>

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
	std::chrono::duration<double> m_durationWarmup[TIMING_MAX];
	// m_iterations holds the number of times a given timing type has been reported (minus the warmup run)
	int m_iterations[TIMING_MAX];
	// m_durationsSum holds the sum of all the post-warmup runs
	std::chrono::duration<double> m_durationsSum[TIMING_MAX];
	// m_lapIterations holds a "lap" counter that helps when looking at instantaneous runs rather than the total
	// run.  For example if the algorithm takes a lot of time to compute when parameters are changed, but less
	// time when flipping frames, then the lap time can be used to show the latest frame while the m_iterations
	// and m_durationsSum will report the overall results including both the runs where parameters were changed
	// and those where they were not.
	int m_lapIterations[TIMING_MAX];
	std::chrono::duration<double> m_lapDurationsSum[TIMING_MAX];

public:
	static TimingStats* GetTimingStats();

	TimingStats();
	//~TimingStats();

	void Reset();
	void ResetLap();
	void AddIterationResults(ETimingType timingType, std::chrono::system_clock::time_point startTime, std::chrono::system_clock::time_point endTime, bool bReportIteration = false);
	std::string GetTypeString(ETimingType timingType);
	void ReportTime(std::string strDesc, std::string typeString, std::chrono::duration<double> durationSum, int numIterations, ETimingType timingType);
	void ReportTimes(bool bIncludeLap);
	std::string GetSummaryLine(std::string strDesc, std::string typeString, std::chrono::duration<double> durationSum, int numIterations, ETimingType timingType);
	std::string SummaryStats(bool bIncludeLap = true);
};
