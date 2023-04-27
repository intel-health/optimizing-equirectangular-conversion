#pragma once

#include <chrono>

enum ETimingType {
	TIMING_INITIALIZATION = 0,
	TIMING_CREATE_XYZ_COORDS,
	TIMING_CREATE_LON_LAT_COORDS,
	TIMING_CREATE_XY_COORDS,
	TIMING_FRAME_CALCULATIONS,
	TIMING_CREATE_MAP,
	TIMING_REMAP,
	TIMING_IMAGE_EXTRACTION,
	TIMING_FRAME,
	TIMING_TOTAL,
	TIMING_MAX
};

class TimingStats {

private:
	static TimingStats* c_timingStats;

	std::chrono::system_clock::time_point m_startTimes[TIMING_MAX];
	std::chrono::system_clock::time_point m_endTimes[TIMING_MAX];
	int m_iterations[TIMING_MAX];
	std::chrono::duration<double> m_durationsSum[TIMING_MAX];
	int m_lapIterations[TIMING_MAX];
	std::chrono::duration<double> m_lapDurationsSum[TIMING_MAX];

public:
	static TimingStats* GetTimingStats();

	TimingStats();
	//~TimingStats();

	void Reset();
	void ResetLap();
	void AddIterationResults(ETimingType timingType, std::chrono::system_clock::time_point startTime, std::chrono::system_clock::time_point endTime, bool bReportIteration = false);
	void ReportTime(ETimingType timingType, bool bIncludeLap);
	void ReportTimes(bool bIncludeLap);
};
