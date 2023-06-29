# Copyright 2017 Nitish Mutha (nitishmutha.com)

# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at

#     http://www.apache.org/licenses/LICENSE-2.0

# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from math import pi
import numpy as np
import time

class NFOV():
    def __init__(self, height=1080, width=1080):
        self.PI = pi
        self.PI_2 = pi * 0.5
        self.PI2 = pi * 2.0
        #self.FOV = [10.0 / 180.0 * np.pi, 20.0 / 180.0 * np.pi]
        # The first FOV runs from 0 to 1 and covers the range of values from 0 to 180
        # degree field of view in the horizontal direction.  The second FOV number is vertical
        # direction also from 0 to 180
        self.FOV = [60/180, 60/180]
        self.height = height
        self.width = width
        self.screen_points = self._get_screen_img()

    def _get_coord_rad(self, isCenterPt, center_point=None):
        # Original code used PI for first element and PI/2 for the second element, but since the screen coordinates
        # would be maximum 180 degrees vertical and horizontal seems these should both be
        # PI (half the sphere).
        # The center point makes sense being PI and PI / 2 since the yaw element would be 2 * PI (the
        # 2 * is done separately against the center point) and tilt element would be PI, but since
        # there is a 2 * on the center point, need to multiple by PI / 2.
        #(self.screen_points * 2 - 1) * np.array([self.PI, self.PI_2]) * (
        #    np.ones(self.screen_points.shape) * self.FOV)
        return (center_point * 2 - 1) * np.array([self.PI, self.PI_2]) \
            if isCenterPt \
            else \
            (self.screen_points * 2 - 1) * np.array([self.PI_2, self.PI_2]) * (
                np.ones(self.screen_points.shape) * self.FOV)

    def _get_screen_img(self):
        xx, yy = np.meshgrid(np.linspace(0, 1, self.width), np.linspace(0, 1, self.height))
        return np.array([xx.ravel(), yy.ravel()]).T

    def _calcSphericaltoGnomonic(self, convertedScreenCoord):
        x = convertedScreenCoord.T[0]
        y = convertedScreenCoord.T[1]

        rou = np.sqrt(x ** 2 + y ** 2)
        c = np.arctan(rou)
        sin_c = np.sin(c)
        cos_c = np.cos(c)

        lat = np.arcsin(cos_c * np.sin(self.cp[1]) + (y * sin_c * np.cos(self.cp[1])) / rou)
        lon = self.cp[0] + np.arctan2(x * sin_c, rou * np.cos(self.cp[1]) * cos_c - y * np.sin(self.cp[1]) * sin_c)

        lat = (lat / self.PI_2 + 1.) * 0.5
        lon = (lon / self.PI + 1.) * 0.5

        return np.array([lon, lat]).T

    def _bilinear_interpolation(self, screen_coord):
        uf = np.mod(screen_coord.T[0],1) * self.frame_width  # long - width
        vf = np.mod(screen_coord.T[1],1) * self.frame_height  # lat - height
        #print("uf = ", uf)
        #print("vf = ", vf)
        x0 = np.floor(uf).astype(int)  # coord of pixel to bottom left
        y0 = np.floor(vf).astype(int)
        x2 = np.add(x0, np.ones(uf.shape).astype(int))  # coords of pixel to top right
        y2 = np.add(y0, np.ones(vf.shape).astype(int))
        #print("x0 = ", x0)
        #print("y0 = ", y0)
        #print("x2 = ", x2)
        #print("y2 = ", y2)
        base_y0 = np.multiply(y0, self.frame_width)
        base_y2 = np.multiply(y2, self.frame_width)

        A_idx = np.add(base_y0, x0)
        B_idx = np.add(base_y2, x0)
        C_idx = np.add(base_y0, x2)
        D_idx = np.add(base_y2, x2)

        flat_img = np.reshape(self.frame, [-1, self.frame_channel])
        A = np.take(flat_img, A_idx, axis=0)
        B = np.take(flat_img, B_idx, axis=0)
        C = np.take(flat_img, C_idx, axis=0)
        D = np.take(flat_img, D_idx, axis=0)
        wa = np.multiply(x2 - uf, y2 - vf)
        wb = np.multiply(x2 - uf, vf - y0)
        wc = np.multiply(uf - x0, y2 - vf)
        wd = np.multiply(uf - x0, vf - y0)

        # interpolate
        AA = np.multiply(A, np.array([wa, wa, wa]).T)
        BB = np.multiply(B, np.array([wb, wb, wb]).T)
        CC = np.multiply(C, np.array([wc, wc, wc]).T)
        DD = np.multiply(D, np.array([wd, wd, wd]).T)
        nfov = np.reshape(np.round(AA + BB + CC + DD).astype(np.uint8), [self.height, self.width, 3])
        #import matplotlib.pyplot as plt
        #plt.imshow(nfov)
        #plt.show()

        return nfov

    def toNFOV(self, frame, center_point):
        self.frame = frame
        self.frame_height = frame.shape[0]
        self.frame_width = frame.shape[1]
        self.frame_channel = frame.shape[2]

        self.cp = self._get_coord_rad(center_point=center_point, isCenterPt=True)
        convertedScreenCoord = self._get_coord_rad(isCenterPt=False)
        spericalCoord = self._calcSphericaltoGnomonic(convertedScreenCoord)
        print("spericalCoord = ", spericalCoord)
        return self._bilinear_interpolation(spericalCoord)


# test the class
if __name__ == '__main__':
    #import imageio as im
    #img = im.imread('images/360.jpg')
		# Read image
    import cv2  # Access to OpenCV
		
    img = cv2.imread("O:\ToConvert\Warren\IMG_20230303_172908_00_009_PureShot.jpg", cv2.IMREAD_ANYCOLOR)

    nfov = NFOV()
    # The center point appears to be the point in the equirectangular image where the
    # camera is pointing.  0 in the first entry (Yaw) will be the far left of the equirectangular
    # image which corresponds to straight behind the camera once it is wrapped in the sphere.
    # 0.25 has the camera pointing to the left.  0.5 points directly ahead.  0.75 will point directly
    # to the right, and 1 will point directly behind similar to 0.  For the tilt coordinate,
    # 0 points directly up, 0.5 points straight out, and 1 points straight down.
    # Thus, first element (Yaw) 0 - 1 corresponds to 0 degrees to 360 degrees moving from directly behind sweeping
    # in a clockwise direction towards the front and then at 360 degrees being directly behind again.
    # The second element (tilt) 0 - 1 corresponds to 0 to 180 degrees sweeping vertically from directly
    # up (0) to directly down (180)
    center_point = np.array([0.5, 0.5])  # camera center point (valid range [0,1])
    start_time = time.time()
    flatImg = nfov.toNFOV(img, center_point)
    end_time = time.time()
    print("toNFOV_time time required = ", end_time - start_time)

    cv2.imshow("Flat View", flatImg)
    key = cv2.waitKeyEx(0)
