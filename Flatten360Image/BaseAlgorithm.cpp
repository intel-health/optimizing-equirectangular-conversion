#include "BaseAlgorithm.hpp"

BaseAlgorithm::BaseAlgorithm(SParameters& parameters)
{
	m_parameters = &parameters;
}

BaseAlgorithm::~BaseAlgorithm()
{

}

void BaseAlgorithm::FrameCalculations(bool bParametersChanged)
{
	if (bParametersChanged)
	{
		PrintParameters(m_parameters);
	}
	m_bFrameCalcRequired = true;
}