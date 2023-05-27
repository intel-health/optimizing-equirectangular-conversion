#pragma once

// The primary difference between this implementation and DpcppRemapping (V1) is that this one packs all
// the kernel work into a single kernel and minimizes the amount of memory being allocated.

#include <sycl/sycl.hpp>
#include "DpcppBaseAlgorithm.hpp"
#include "Point2D.hpp"
#include "Point3D.hpp"

class DpcppRemappingV5 : public DpcppBaseAlgorithm {
private:

	Point2D* m_pDevXYPoints = NULL;
	unsigned char *m_pFlatImage = NULL;
	unsigned char *m_pFullImage = NULL;
	unsigned char *m_pDevFlatImage = NULL;
	unsigned char *m_pDevFullImage = NULL;
	int m_storageType;
	cv::Mat m_rotationMatrix;
	int m_currentIndex;

private:
	// Pass in theta, phi, and psi in radians, not degrees
	void ComputeRotationMatrix(float radTheta, float radPhi, float radPsi);
#if 0
	void ComputeXYZCoords();
	void ComputeLonLatCoords();
	void ConvertXYZToLonLat();
	void ComputeXYCoords();
#endif

public:

	DpcppRemappingV5(SParameters& parameters);

	virtual void FrameCalculations(bool bParametersChanged);
	virtual cv::Mat ExtractFrameImage();

	virtual std::string GetDescription();

	virtual bool StartVariant();
	virtual void StopVariant();

};
