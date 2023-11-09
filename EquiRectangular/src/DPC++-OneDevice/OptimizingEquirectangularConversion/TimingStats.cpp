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

#include "TimingStats.hpp"
#include <string>
#include <iostream>

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
		m_durationWarmup[i] = std::chrono::duration<double>::zero();
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

void TimingStats::AddIterationResults(ETimingType timingType, std::chrono::high_resolution_clock::time_point startTime, std::chrono::high_resolution_clock::time_point endTime, bool bReportIteration /* = false */)
{
	std::chrono::duration<double> duration = std::chrono::duration<double>(endTime - startTime);

	if (timingType == TIMING_TOTAL)
	{
		m_lapIterations[timingType] = m_lapIterations[TIMING_FRAME];
		m_lapDurationsSum[timingType] = duration;
	}
	else
	{
		if (m_durationWarmup[timingType] == std::chrono::duration<double>::zero())
		{
			m_durationWarmup[timingType] = duration;
		}
		else
		{
			m_iterations[timingType]++;
			m_durationsSum[timingType] += duration;
		}
		m_lapIterations[timingType]++;
		m_lapDurationsSum[timingType] += duration;
	}
}

std::string TimingStats::GetSummaryLine(std::string strDesc, std::string typeString, std::chrono::duration<double> durationSum, int numIterations, ETimingType timingType)
{
	char line1[1024];
	char line2[1024];
	std::string retVal = "";

#define CSV_OUTPUT
#ifdef CSV_OUTPUT
	// The Comma Separated Variables output is useful when loading the output into a program such as Excel
	char const *pFmt = "%15s,%3d,%23s,%12.8f,s,%12.5f,ms,%12.3f,us, ";
	char const *pFmt2 = "FPS, %12.8f\n";
#else
	char const *pFmt = "%15s %3d %23s %12.8fs %12.5fms %12.3fus ";
	char const *pFmt2 = "FPS = %12.8f\n";
#endif
	std::chrono::duration<double> aveDuration = durationSum / numIterations;
	sprintf(line1, pFmt, strDesc.c_str(), numIterations, typeString.c_str(), aveDuration, aveDuration * 1000.0, aveDuration * 1000000.0);
	if (timingType == TIMING_FRAME)
	{
		sprintf(line2, pFmt2, 1.0 / (aveDuration.count() * std::chrono::duration<double>::period::num / std::chrono::duration<double>::period::den));
	}
	else if (timingType == TIMING_TOTAL)
	{
		sprintf(line2, pFmt2, 1.0 / (aveDuration.count() * std::chrono::duration<double>::period::num / std::chrono::duration<double>::period::den));
	}
	else
	{
		sprintf(line2, "\n");
	}

	retVal = line1;
	retVal += line2;

	return retVal;
}

void TimingStats::ReportTime(std::string strDesc, std::string typeString, std::chrono::duration<double> durationSum, int numIterations, ETimingType timingType)
{
	printf("%s", GetSummaryLine(strDesc, typeString, durationSum, numIterations, timingType).c_str());
}

std::string TimingStats::GetTypeString(ETimingType timingType)
{
	std::string strDesc;

	switch (timingType)
	{
	case TIMING_INITIALIZATION:
		strDesc = "Class initialization";
		break;
	case VARIANT_INITIALIZATION:
		strDesc = "Variant initialization";
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
		strDesc = "frame(s)";
		break;
	case VARIANT_TERMINATION:
		strDesc = "Variant cleanup";
		break;
	case TIMING_TOTAL:
		strDesc = "Total";
		break;
	default:
		strDesc = "Unknown";
		break;
	}

	return strDesc;
}

void TimingStats::ReportTimes(bool bIncludeLap)
{
	std::string desc;

	desc = "warmup";
	for (int i = 0; i < ETimingType::TIMING_MAX; i++)
	{
		if (m_durationWarmup[i] != std::chrono::duration<double>::zero())
		{
			std::string typeString = GetTypeString((ETimingType)i);
			ReportTime(desc, typeString, m_durationWarmup[i], 1, (ETimingType)i);
		}
	}
	desc = "times averaging";
	for (int i = 0; i < ETimingType::TIMING_MAX; i++)
	{
		if (m_iterations[i] != 0)
		{
			std::string typeString = GetTypeString((ETimingType)i);
			ReportTime(desc, typeString, m_durationsSum[i], m_iterations[i], (ETimingType)i);
		}
	}
	if (bIncludeLap)
	{
		desc = "lap averaging";
		for (int i = 0; i < ETimingType::TIMING_MAX; i++)
		{
			if (m_lapIterations[i] != 0)
			{
				std::string typeString = GetTypeString((ETimingType)i);
				if (i == ETimingType::TIMING_TOTAL)
				{
					ReportTime("total averaging", typeString, m_lapDurationsSum[i], m_lapIterations[i], (ETimingType)i);
				}
				else
				{
					ReportTime(desc, typeString, m_lapDurationsSum[i], m_lapIterations[i], (ETimingType)i);
				}
			}
		}
	}
}

std::string TimingStats::SummaryStats(bool bIncludeLap /* = true */)
{
	std::string retVal = "";

	if (m_durationWarmup[ETimingType::TIMING_FRAME] != std::chrono::duration<double>::zero())
	{
		retVal += GetSummaryLine("warmup", GetTypeString(ETimingType::TIMING_FRAME), m_durationWarmup[ETimingType::TIMING_FRAME], 1, ETimingType::TIMING_FRAME);
	}
	if (m_iterations[ETimingType::TIMING_FRAME] != 0)
	{
		retVal += GetSummaryLine("times averaging", GetTypeString(ETimingType::TIMING_FRAME), m_durationsSum[ETimingType::TIMING_FRAME], m_iterations[ETimingType::TIMING_FRAME], ETimingType::TIMING_FRAME);
	}
	if (m_lapIterations[ETimingType::TIMING_TOTAL] != 0)
	{
		retVal += GetSummaryLine("total averaging", GetTypeString(ETimingType::TIMING_TOTAL), m_lapDurationsSum[ETimingType::TIMING_TOTAL], m_lapIterations[ETimingType::TIMING_TOTAL], ETimingType::TIMING_TOTAL);
	}
	if (bIncludeLap && m_lapIterations[ETimingType::TIMING_TOTAL] != 0)
	{
		retVal += GetSummaryLine("lap averaging", GetTypeString(ETimingType::TIMING_FRAME), m_lapDurationsSum[ETimingType::TIMING_FRAME], m_lapIterations[ETimingType::TIMING_FRAME], ETimingType::TIMING_FRAME);
	}

	return retVal;
}

