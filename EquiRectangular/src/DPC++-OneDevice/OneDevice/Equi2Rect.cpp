// The original code uses the MIT license, but did not have the standard license header added.  Added below
// for completeness.
// MIT License
// 
// Copyright(c) 2018 Ruofan
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software andassociated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, andto permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// 
// The above copyright notice andthis permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// This was adopted from https://github.com/rfn123/equirectangular-to-rectlinear/blob/master/Equi2Rect.cpp
// and then modified to fit into this example framework

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


#include "Equi2Rect.hpp"
#include <math.h>

Equi2Rect::Equi2Rect(SParameters &parameters) : BaseAlgorithm(parameters)
{
    // initialize result image
    img_dst = cv::Mat(m_parameters->m_heightOutput, m_parameters->m_widthOutput, CV_8UC3, cv::Scalar(0, 0, 0));
}

std::string Equi2Rect::GetDescription()
{
    return "V1 Equi2Rect reprojection.";
}

void Equi2Rect::FrameCalculations(bool bParametersChanged)
{
    if (bParametersChanged || m_bFrameCalcRequired)
    {
        // create rotation matrix
        Rot = eul2rotm(m_parameters->m_pitch * DEGREE_CONVERSION_FACTOR, m_parameters->m_yaw * DEGREE_CONVERSION_FACTOR, m_parameters->m_roll * DEGREE_CONVERSION_FACTOR);

        // specify focal length of the final pinhole image
        focal_length = 0.5 * m_parameters->m_widthOutput * 1 / tan(0.5 * m_parameters->m_fov / 180.0 * M_PI);

        // create camera matrix K
        K = (cv::Mat_<float>(3, 3) << focal_length, 0, m_parameters->m_widthOutput / 2,
            0, focal_length, m_parameters->m_heightOutput / 2,
            0, 0, 1);

        BaseAlgorithm::FrameCalculations(bParametersChanged);
    }
}

auto Equi2Rect::eul2rotm(float rotx, float roty, float rotz) -> cv::Mat
{

    cv::Mat R_x = (cv::Mat_<float>(3, 3) << 1, 0, 0,
        0, cos(rotx), -sin(rotx),
        0, sin(rotx), cos(rotx));

    cv::Mat R_y = (cv::Mat_<float>(3, 3) << cos(roty), 0, sin(roty),
        0, 1, 0,
        -sin(roty), 0, cos(roty));

    cv::Mat R_z = (cv::Mat_<float>(3, 3) << cos(rotz), -sin(rotz), 0,
        sin(rotz), cos(rotz), 0,
        0, 0, 1);

    cv::Mat R = R_z * R_y * R_x;

    return R;
}

auto Equi2Rect::reprojection(int x_img, int y_img) -> cv::Vec2d
{
    cv::Mat xyz = (cv::Mat_<float>(3, 1) << (float)x_img, (float)y_img, 1);
    cv::Mat ray3d = Rot * K.inv() * xyz / norm(xyz);

    // get 3d spherical coordinates
    float xp = ray3d.at<float>(0);
    float yp = ray3d.at<float>(1);
    float zp = ray3d.at<float>(2);
    // inverse formula for spherical projection, reference Szeliski book "Computer Vision: Algorithms and Applications" p439.
    float theta = atan2(yp, sqrt(xp * xp + zp * zp));
    float phi = atan2(xp, zp);

    // get 2D point on equirectangular map
    float x_sphere = (((phi * m_parameters->m_image[m_parameters->m_imageIndex].cols) / M_PI + m_parameters->m_image[m_parameters->m_imageIndex].cols) / 2);
    float y_sphere = (theta + M_PI / 2) * m_parameters->m_image[m_parameters->m_imageIndex].rows / M_PI;

    return cv::Vec2d(x_sphere, y_sphere);
}

auto Equi2Rect::bilinear_interpolation() -> void
{
    cv::Vec2d current_pos;
    // variables for bilinear interpolation
    int top_left_x, top_left_y;
    float dx, dy, wtl, wtr, wbl, wbr;
    cv::Vec3b value, bgr;

    // loop over every pixel in output rectlinear image
    for (int v = 0; v < m_parameters->m_heightOutput; ++v)
    {
        for (int u = 0; u < m_parameters->m_widthOutput; ++u)
        {

            // determine corresponding position in the equirectangular panorama
            current_pos = reprojection(u, v);

            // determine the nearest top left pixel for bilinear interpolation
            top_left_x = static_cast<int>(current_pos[0]); // convert the subpixel value to a proper pixel value (top left pixel due to int() operator)
            top_left_y = static_cast<int>(current_pos[1]);

            // if the current position exceeeds the panorama image limit -- leave pixel black and skip to next iteration
            if (current_pos[0] < 0 || top_left_x > m_parameters->m_image[m_parameters->m_imageIndex].cols - 1 || current_pos[1] < 0 || top_left_y > m_parameters->m_image[m_parameters->m_imageIndex].rows - 1)
            {
                continue;
            }

            // initialize weights for bilinear interpolation
            dx = current_pos[0] - top_left_x;
            dy = current_pos[1] - top_left_y;
            wtl = (1.0 - dx) * (1.0 - dy);
            wtr = dx * (1.0 - dy);
            wbl = (1.0 - dx) * dy;
            wbr = dx * dy;

            // determine subpixel value with bilinear interpolation
            bgr = wtl * m_parameters->m_image[m_parameters->m_imageIndex].at<cv::Vec3b>(top_left_y, top_left_x) + wtr * m_parameters->m_image[m_parameters->m_imageIndex].at<cv::Vec3b>(top_left_y, top_left_x + 1) +
                wbl * m_parameters->m_image[m_parameters->m_imageIndex].at<cv::Vec3b>(top_left_y + 1, top_left_x) + wbr * m_parameters->m_image[m_parameters->m_imageIndex].at<cv::Vec3b>(top_left_y + 1, top_left_x + 1);

            // paint the pixel in the output image with the calculated value
            img_dst.at<cv::Vec3b>(cv::Point(u, v)) = bgr;
        }
    }
    return;
}

cv::Mat Equi2Rect::ExtractFrameImage()
{
    bilinear_interpolation();

    return img_dst;
}

void Equi2Rect::StopVariant()
{
}

cv::Mat Equi2Rect::GetDebugImage()
{
    return img_dst;
}

