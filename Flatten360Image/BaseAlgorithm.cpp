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
