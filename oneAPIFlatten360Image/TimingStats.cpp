#include "TimingStats.hpp"
#include <string>

TimingStats* TimingStats::c_timingStats = new TimingStats();

TimingStats *TimingStats::GetTimingStats()
{
	return c_timingStats;
}

TimingStats::TimingStats()
{
	Reset();
	ResetLap();
}

void TimingStats::Reset()
{
	for (int i = 0; i < ETimingType::TIMING_MAX; i++)
	{
		m_iterations[i] = 0;
		m_durationsSum[i] = std::chrono::duration<double>::zero();
	}
}

void TimingStats::ResetLap()
{
	for (int i = 0; i < ETimingType::TIMING_MAX; i++)
	{
		m_lapIterations[i] = 0;
		m_lapDurationsSum[i] = std::chrono::duration<double>::zero();
	}
}

void TimingStats::AddIterationResults(ETimingType timingType, std::chrono::system_clock::time_point startTime, std::chrono::system_clock::time_point endTime, bool bReportIteration /* = false */)
{
	std::chrono::duration<double> duration = std::chrono::duration<double>(endTime - startTime);

	m_iterations[timingType]++;
	m_durationsSum[timingType] += duration;
	m_lapIterations[timingType]++;
	m_lapDurationsSum[timingType] += duration;
}

void TimingStats::ReportTime(ETimingType timingType, bool bIncludeLap)
{
	std::string strDesc;

	switch (timingType)
	{
	case TIMING_INITIALIZATION:
		strDesc = "Initialization";
		break;
	case TIMING_CREATE_XYZ_COORDS:
		strDesc = "Create XYZ Coords";
		break;
	case TIMING_CREATE_LON_LAT_COORDS:
		strDesc = "Create Lon Lat Coords";
		break;
	case TIMING_CREATE_XY_COORDS:
		strDesc = "Create XY Coords";
		break;
	case TIMING_FRAME_CALCULATIONS:
		strDesc = "Frame Calculations";
		break;
	case TIMING_CREATE_MAP:
		strDesc = "Create Map";
		break;
	case TIMING_REMAP:
		strDesc = "Remap";
		break;
	case TIMING_IMAGE_EXTRACTION:
		strDesc = "Image extraction";
		break;
	case TIMING_FRAME:
		strDesc = "Frame";
		break;
	case TIMING_TOTAL:
		strDesc = "Total";
		break;
	}
	std::chrono::duration<double> aveDuration = m_durationsSum[timingType] / m_iterations[timingType];
	printf("%23s %3d times averaging %12.8fs %12.5fms %12.3fus ", strDesc.c_str(), m_iterations[timingType], aveDuration, aveDuration * 1000.0, aveDuration * 1000000.0);
	if (timingType == TIMING_FRAME)
	{
		printf("FPS = %12.8f ", 1.0 / (aveDuration.count() * std::chrono::duration<double>::period::num / std::chrono::duration<double>::period::den));
	}
	if (bIncludeLap && m_lapIterations[timingType] > 0)
	{
		aveDuration = m_lapDurationsSum[timingType] / m_lapIterations[timingType]; 
		printf("%3d lap averaging %12.8fs %12.5fms %12.3fus", m_lapIterations[timingType], aveDuration, aveDuration * 1000.0, aveDuration * 1000000.0);
		if (timingType == TIMING_FRAME)
		{
			printf(" FPS = %12.8f", 1.0 / (aveDuration.count() * std::chrono::duration<double>::period::num / std::chrono::duration<double>::period::den));
		}
	}
	printf("\n");
}

void TimingStats::ReportTimes(bool bIncludeLap)
{
	for (int i = 0; i < ETimingType::TIMING_MAX; i++)
	{
		if (m_iterations[i] != 0)
		{
			ReportTime((ETimingType)i, bIncludeLap);
		}
	}
}
