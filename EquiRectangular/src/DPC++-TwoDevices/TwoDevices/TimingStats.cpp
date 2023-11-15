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
	{
		std::lock_guard<std::mutex> requestWorkLock(m_accessMutex);

		for (int i = 0; i < ETimingType::TIMING_MAX; i++)
		{
			for (int j = 0; j < ALL_STATS; j++)
			{
				m_iterations[i][j] = 0;
				m_warmupIterations[i][j] = 0;
				m_durationsSum[i][j] = std::chrono::duration<double>::zero();
				m_durationWarmup[i][j] = std::chrono::duration<double>::zero();
			}
		}
	}
}

void TimingStats::ResetLap()
{
	{
		std::lock_guard<std::mutex> requestWorkLock(m_accessMutex);

		for (int i = 0; i < ETimingType::TIMING_MAX; i++)
		{
			for (int j = 0; j < ALL_STATS; j++)
			{
				m_lapIterations[i][j] = 0;
				m_lapDurationsSum[i][j] = std::chrono::duration<double>::zero();
			}
		}
	}
}

void TimingStats::AddIterationResults(ETimingType timingType, unsigned int uiDevIndex, std::chrono::high_resolution_clock::time_point startTime, std::chrono::high_resolution_clock::time_point endTime)
{
	std::chrono::duration<double> duration = std::chrono::duration<double>(endTime - startTime);

	{
		std::lock_guard<std::mutex> requestWorkLock(m_accessMutex);

		if (timingType == TIMING_TOTAL)
		{
			m_lapIterations[timingType][uiDevIndex] = 0;
			for (int j = 0; j < MAX_DEVICES; j++)
			{
				m_lapIterations[timingType][uiDevIndex] += m_lapIterations[TIMING_FRAME][j];
			}
			m_lapDurationsSum[timingType][uiDevIndex] = duration;
		}
		else
		{
			if (m_warmupIterations[timingType][uiDevIndex] < 1)
			{
				m_durationWarmup[timingType][uiDevIndex] = duration;
				m_warmupIterations[timingType][uiDevIndex]++;
			}
			else
			{
				m_iterations[timingType][uiDevIndex]++;
				m_durationsSum[timingType][uiDevIndex] += duration;
			}
			m_lapIterations[timingType][uiDevIndex]++;
			m_lapDurationsSum[timingType][uiDevIndex] += duration;
		}
	}
}

std::string TimingStats::GetSummaryLine(std::string strDesc, std::string typeString, std::string devString, std::chrono::duration<double> durationSum, int numIterations, ETimingType timingType)
{
	char line1[1024];
	char line2[1024];
	std::string retVal = "";

#define CSV_OUTPUT
#ifdef CSV_OUTPUT
	// The Comma Separated Variables output is useful when loading the output into a program such as Excel
	char const *pFmt = "%3s,%15s,%3d,%23s,%12.8f,s,%12.5f,ms,%12.3f,us, ";
	char const *pFmt2 = "FPS, %12.8f\n";
#else
	char const *pFmt = "%3s %15s %3d %23s %12.8fs %12.5fms %12.3fus ";
	char const *pFmt2 = "FPS = %12.8f\n";
#endif
	std::chrono::duration<double> aveDuration = durationSum / numIterations;
	sprintf(line1, pFmt, devString.c_str(), strDesc.c_str(), numIterations, typeString.c_str(), aveDuration, aveDuration * 1000.0, aveDuration * 1000000.0);
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

void TimingStats::ReportTime(std::string strDesc, std::string typeString, std::string devString, std::chrono::duration<double> durationSum, int numIterations, ETimingType timingType)
{
	printf("%s", GetSummaryLine(strDesc, typeString, devString, durationSum, numIterations, timingType).c_str());
}

std::string TimingStats::GetDevString(unsigned int uiDevIndex)
{
	std::string strDev = "Unknown";

	if (uiDevIndex == 0)
	{
		strDev = "CPU";
	}
	else if (uiDevIndex == 1)
	{
		strDev = "GPU";
	}
	else if (uiDevIndex == GENERAL_STATS)
	{
		strDev = "All";
	}

	return strDev;
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

	for (int j = 0; j < ALL_STATS; j++)
	{
		std::string devString = GetDevString(j);

		desc = "warmup";
		for (int i = 0; i < ETimingType::TIMING_MAX; i++)
		{
			if (m_durationWarmup[i][j] != std::chrono::duration<double>::zero())
			{
				std::string typeString = GetTypeString((ETimingType)i);
				ReportTime(desc, typeString, devString, m_durationWarmup[i][j], m_warmupIterations[i][j], (ETimingType)i);
			}
		}
		desc = "times averaging";
		for (int i = 0; i < ETimingType::TIMING_MAX; i++)
		{
			if (m_iterations[i][j] != 0)
			{
				std::string typeString = GetTypeString((ETimingType)i);
				ReportTime(desc, typeString, devString, m_durationsSum[i][j], m_iterations[i][j], (ETimingType)i);
			}
		}
		if (bIncludeLap)
		{
			desc = "lap averaging";
			for (int i = 0; i < ETimingType::TIMING_MAX; i++)
			{
				if (m_lapIterations[i][j] != 0)
				{
					std::string typeString = GetTypeString((ETimingType)i);
					if (i == ETimingType::TIMING_TOTAL)
					{
						ReportTime("total averaging", typeString, devString, m_lapDurationsSum[i][j], m_lapIterations[i][j], (ETimingType)i);
					}
					else
					{
						ReportTime(desc, typeString, devString, m_lapDurationsSum[i][j], m_lapIterations[i][j], (ETimingType)i);
					}
				}
			}
		}
	}
}

std::string TimingStats::SummaryStats(bool bIncludeLap /* = true */)
{
	std::string retVal = "";

	for (int j = 0; j < ALL_STATS; j++)
	{
		std::string devString = GetDevString(j);

		if (m_durationWarmup[ETimingType::TIMING_FRAME][j] != std::chrono::duration<double>::zero())
		{
			retVal += GetSummaryLine("warmup", GetTypeString(ETimingType::TIMING_FRAME), devString, m_durationWarmup[ETimingType::TIMING_FRAME][j], m_warmupIterations[ETimingType::TIMING_FRAME][j], ETimingType::TIMING_FRAME);
		}
		if (m_iterations[ETimingType::TIMING_FRAME][j] != 0)
		{
			retVal += GetSummaryLine("times averaging", GetTypeString(ETimingType::TIMING_FRAME), devString, m_durationsSum[ETimingType::TIMING_FRAME][j], m_iterations[ETimingType::TIMING_FRAME][j], ETimingType::TIMING_FRAME);
		}
		if (m_lapIterations[ETimingType::TIMING_TOTAL][j] != 0)
		{
			retVal += GetSummaryLine("total averaging", GetTypeString(ETimingType::TIMING_TOTAL), devString, m_lapDurationsSum[ETimingType::TIMING_TOTAL][j], m_lapIterations[ETimingType::TIMING_TOTAL][j], ETimingType::TIMING_TOTAL);
		}
		if (bIncludeLap && m_lapIterations[ETimingType::TIMING_FRAME][j] != 0)
		{
			retVal += GetSummaryLine("lap averaging", GetTypeString(ETimingType::TIMING_FRAME), devString, m_lapDurationsSum[ETimingType::TIMING_FRAME][j], m_lapIterations[ETimingType::TIMING_FRAME][j], ETimingType::TIMING_FRAME);
		}
	}

	return retVal;
}

