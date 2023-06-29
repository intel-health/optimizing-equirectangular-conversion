// Copyright (C) 2023 Intel Corporation
//
// This software and the related documents are Intel copyrighted materials, 
// and your use of them is governed by the express license under which
// they were provided to you ("License"). Unless the License provides
// otherwise, you may not use, modify, copy, publish, distribute, disclose or
// transmit this software or the related documents without Intel's prior
// written permission.
//
// This software and the related documents are provided as is, with no
// express or implied warranties, other than those that are expressly stated
// in the License.
#include "BaseAlgorithm.hpp"
#include <iostream>

BaseAlgorithm::BaseAlgorithm(SParameters& parameters)
{
	m_parameters = &parameters;
	m_bVariantRun = false;
}

BaseAlgorithm::~BaseAlgorithm()
{

}

void BaseAlgorithm::FrameCalculations(bool bParametersChanged)
{
	m_bFrameCalcRequired = false;
}

bool BaseAlgorithm::StartVariant()
{
	bool bRetVal = true;

	m_bFrameCalcRequired = true;
	if (m_bVariantRun)
	{
		bRetVal = false;
	}
	else
	{
		m_bVariantRun = true;
	}

	return bRetVal;
}
