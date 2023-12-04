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
// 

// This was adopted from https://github.com/rfn123/equirectangular-to-rectlinear/blob/master/Equi2Rect.hpp
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
    auto eul2rotm(float rotx, float roty, float rotz) -> cv::Mat;
    auto bilinear_interpolation() -> void;
    auto reprojection(int x_img, int y_img) -> cv::Vec2d;

    int focal_length;
    cv::Mat Rot;
    cv::Mat K;
    cv::Mat img_dst;
};

#endif