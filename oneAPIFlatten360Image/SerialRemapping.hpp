#pragma once

#include "BaseAlgorithm.hpp"
#include "Point2D.hpp"
#include "Point3D.hpp"
#include "SoAPoints3D.hpp"
#include <opencv2/core/mat.hpp>

enum StorageOrder {
	// Array of structures row then column
	E_STORE_ROW_COL,
	// Array of structures column then row
	E_STORE_COL_ROW,
	// Structure of arrays
	E_STORE_SOA
};

class SerialRemapping : public BaseAlgorithm {

private:
	Point3D *m_pXYZPoints = NULL;
	Point2D *m_pLonLatPoints = NULL;
	Point2D *m_pXYPoints = NULL;
	SoAPoints3D m_SoAXYZPoints;
	StorageOrder m_storageOrder;
	cv::Mat m_rotationMatrix;

	// Pass in theta, phi, and psi in radians, not degrees
	void ComputeRotationMatrix(float radTheta, float radPhi, float radPsi);
	void ComputeXYZCoords();
	void ComputeLonLatCoords();
	void ConvertXYZToLonLat();
	void ComputeXYCoords();


public:
	SerialRemapping(SParameters &parameters, StorageOrder storageOrder = E_STORE_ROW_COL);
	SerialRemapping::~SerialRemapping();

	virtual void FrameCalculations();
	virtual cv::Mat ExtractFrameImage();
	virtual cv::Mat GetDebugImage();

	virtual std::string GetDescription();
};