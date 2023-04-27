#include "BaseAlgorithm.hpp"

BaseAlgorithm::BaseAlgorithm(SParameters& parameters)
{
	m_parameters = &parameters;
}

bool BaseAlgorithm::StartSubAlgorithm(uint algNum)
{
	// Default algorithms only have one sub algorithm, but for some of the DPC++
	// algorithms they may run on more than one target device so they can
	// override this and run more than one time.
	return algNum == 0;
}
