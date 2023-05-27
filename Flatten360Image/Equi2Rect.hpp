// This was adopted from https://github.com/rfn123/equirectangular-to-rectlinear/blob/master/Equi2Rect.hpp
// and then modified to fit into this example framework

#pragma once

#ifndef EQUI2RECT_HPP
#define EQUI2RECT_HPP

#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "BaseAlgorithm.hpp"

class Equi2Rect : public BaseAlgorithm
{
public:
    Equi2Rect(SParameters &parameters);
    virtual void FrameCalculations(bool bParametersChanged);
    virtual cv::Mat ExtractFrameImage();
    virtual cv::Mat GetDebugImage();

    virtual std::string GetDescription();

    virtual void StopVariant();

private:
    auto eul2rotm(double rotx, double roty, double rotz) -> cv::Mat;
    auto bilinear_interpolation() -> void;
    auto reprojection(int x_img, int y_img) -> cv::Vec2d;

    int focal_length;
    cv::Mat Rot;
    cv::Mat K;
    cv::Mat img_dst;
};

#endif