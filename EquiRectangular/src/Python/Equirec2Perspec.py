# From https://github.com/fuenwang/Equirec2Perspec/blob/master/Equirec2Perspec.py

# Copyright (C) 2023 Intel Corporation
# Modifications by Doug Bogia to add timing, add psi (roll) and tailor to a specific use case

import os
import sys
import cv2
import numpy as np
import time

def xyz2lonlat(xyz):
    atan2 = np.arctan2
    asin = np.arcsin

    norm = np.linalg.norm(xyz, axis=-1, keepdims=True)
    xyz_norm = xyz / norm
    x = xyz_norm[..., 0:1]
    y = xyz_norm[..., 1:2]
    z = xyz_norm[..., 2:]

    lon = atan2(x, z)
    lat = asin(y)
    lst = [lon, lat]

    out = np.concatenate(lst, axis=-1)
    return out

def lonlat2XY(lonlat, shape):
    X = (lonlat[..., 0:1] / (2 * np.pi) + 0.5) * (shape[1] - 1)
    Y = (lonlat[..., 1:] / (np.pi) + 0.5) * (shape[0] - 1)
    lst = [X, Y]
    out = np.concatenate(lst, axis=-1)

    return out 

class Equirectangular:
    def __init__(self, img):
        self._img = img
        [self._height, self._width, _] = self._img[0].shape
        self.XY = None
        self.FOV = None
        self.THETA = None
        self.PHI = None
        self.psi = None
        self.height = None
        self.width = None
        #cp = self._img.copy()  
        #w = self._width
        #self._img[:, :w/8, :] = cp[:, 7*w/8:, :]
        #self._img[:, w/8:, :] = cp[:, :7*w/8, :]
    

    def GetPerspective(self, imgIndex, FOV, THETA, PHI, psi, height, width):
        #
        # THETA is left/right angle, PHI is up/down angle, psi is the roll, all in degrees
        #
        # f may be focal length
        bChanged = False
        start_time = time.perf_counter()
        if (FOV != self.FOV or THETA != self.THETA or PHI != self.PHI or self.psi != psi or height != self.height or width != self.width):
          bChanged = True
          self.FOV = FOV
          self.THETA = THETA
          self.PHI = PHI
          self.psi = psi
          self.height = height
          self.width = width
          f = 0.5 * width * 1 / np.tan(0.5 * FOV / 180.0 * np.pi)
          cx = (width - 1) / 2.0
          cy = (height - 1) / 2.0
          K = np.array([
                  [f, 0, cx],
                  [0, f, cy],
                  [0, 0,  1],
                ], np.float32)
          # Generate the multiplicative inverse of K satisfying dot(a, ainv) = dot(ainv, a) = eye(a.shape[0])
          K_inv = np.linalg.inv(K)
          k_inv_time = time.perf_counter()
          # Possible Optimization: The inverse matrix could have just been defined as follows and not define K at all.
          # Another optimization is that the matrix only changes if width or FOV change, so could skip computing
          # every call to this function if that was calcluated outside the function with either value changes.  Probably
          # not a huge savings, but if it were streaming video, every bit helps.
          #K_inv = np.array([
          #                   [1/f,  0, -cx / f],
          #                   [0,  1/f, -cy / f],
          #                   [0,    0,       1],
          #                  ], np.float32)
          #
          # Generate an array of values from 0 to width, including 0, but excluding width
          x = np.arange(width)
          # Generate an array of values from 0 to height, including 0, but excluding height
          y = np.arange(height)
          # Generate two array of arrays where the x sub-arrays will run [0, width) and y from [0, height)
          x, y = np.meshgrid(x, y)
          # Generate an array of arrays where each sub-array contains width number of 1s
          z = np.ones_like(x)
          # Generate a 3 dimensional array of indexes into the flattened image where z is always 1
          # The axis=-1 concatenates around the last dimension (z)
          xyz = np.concatenate([x[..., None], y[..., None], z[..., None]], axis=-1)
          # Matrix multiply to convert to equirectangular sphere coordinates
          xyz = xyz @ K_inv.T
          xyz_time = time.perf_counter()

          x_axis = np.array([1.0, 0.0, 0.0], np.float32)
          y_axis = np.array([0.0, 1.0, 0.0], np.float32)
          z_axis = np.array([0.0, 0.0, -1.0], np.float32)
          R1, _ = cv2.Rodrigues(y_axis * np.radians(THETA))
          R2, _ = cv2.Rodrigues(np.dot(R1, x_axis) * np.radians(PHI))
          R3, _ = cv2.Rodrigues(np.dot(R1, np.dot(R2, z_axis)) * np.radians(psi))
          R = R3 @ R2 @ R1
          xyz = xyz @ R.T
          lonlat = xyz2lonlat(xyz) 
          lonlat_time = time.perf_counter()
          self.XY = lonlat2XY(lonlat, shape=self._img[imgIndex].shape).astype(np.float32)
          XY_time = time.perf_counter()
        persp = cv2.remap(self._img[imgIndex], self.XY[..., 0], self.XY[..., 1], cv2.INTER_CUBIC, borderMode=cv2.BORDER_WRAP)
        persp_time = time.perf_counter()

        if bChanged:
          print("k_inv_time time required = ", k_inv_time - start_time)
          print("xyz_time time required = ", xyz_time - k_inv_time)
          print("lonlat_time time required = ", lonlat_time - xyz_time)
          print("XY_time time required = ", XY_time - lonlat_time)
          print("persp_time time required = ", persp_time - XY_time)
        else:
          print("persp_time time required = ", persp_time - start_time)
        print("Total time required = ", persp_time - start_time)

        return persp, self.XY