#!/usr/bin/env python

# Started from https://stackoverflow.com/questions/30265375/how-to-draw-orthographic-projection-from-equirectangular-projection

# Copyright (C) 2023 Intel Corporation
# SPDX-License-Identifier: Apache-2.0
# Modified the original code since it was using functions that had been deprecated and removed.  Also changed it to plot
# specific views

import math
import numpy as np
import scipy
import cv2  # Access to OpenCV
import scipy.ndimage.interpolation

DEGREE_CONVERSION_FACTOR = 2 * np.pi / 360.0;

for j in range(0, 4):

    if j == 0:
        partialFilename = "-view-0-0-0-60.jpg"
        spinDegrees = -90
        tiltDegrees = 0
    elif j == 1:
        partialFilename = "-view-0-45-0-60.jpg"
        spinDegrees = -90
        tiltDegrees = 0
    elif j == 2:
        partialFilename = "-view-0-74-0-60.jpg"
        spinDegrees = -90
        tiltDegrees = 45
    elif j == 3:
        partialFilename = "-view-0-90-0-60.jpg"
        spinDegrees = -90
        tiltDegrees = 45

    src=cv2.imread("../../../images/full" + partialFilename, cv2.IMREAD_ANYCOLOR)
    outFilename = "../../../images/sphere" + partialFilename
    spin=spinDegrees * DEGREE_CONVERSION_FACTOR
    tilt=tiltDegrees * DEGREE_CONVERSION_FACTOR
    
    size=2048
    frame=0
    # Image pixel co-ordinates
    px=np.arange(-1.0,1.0,2.0/size)+1.0/size
    py=np.arange(-1.0,1.0,2.0/size)+1.0/size
    hx,hy=np.meshgrid(px,py)

    # Compute z of sphere hit position, if pixel's ray hits
    r2=hx*hx+hy*hy
    hit=(r2<=1.0)
    hz=np.where(
        hit,
        -np.sqrt(1.0-np.where(hit,r2,0.0)),
        np.NaN
        )

    cs=math.cos(spin)
    ss=math.sin(spin)
    ms=np.array([[cs,0.0,ss],[0.0,1.0,0.0],[-ss,0.0,cs]])

    ct=math.cos(tilt)
    st=math.sin(tilt)
    mt=np.array([[1.0,0.0,0.0],[0.0,ct,st],[0.0,-st,ct]])

    # Rotate the hit points
    xyz=np.dstack([hx,hy,hz])
    xyz=np.tensordot(xyz,mt,axes=([2],[1]))
    xyz=np.tensordot(xyz,ms,axes=([2],[1]))
    x=xyz[:,:,0]
    y=xyz[:,:,1]
    z=xyz[:,:,2]

    # Compute map position of hit
    latitude =np.where(hit,(0.5+np.arcsin(y)/np.pi)*src.shape[0],0.0)
    longitude=np.where(hit,(1.0+np.arctan2(z,x)/np.pi)*0.5*src.shape[1],0.0)
    latlong=np.array([latitude,longitude])

    # Resample, and zap non-hit pixels
    dst=np.zeros((size,size,3))
    for channel in [0,1,2]:
        dst[:,:,channel]=np.where(
            hit,
            scipy.ndimage.map_coordinates(
                src[:,:,channel],
                latlong,
                order=1
                ),
            0.0
            )

    # Save to the outFilename
    cv2.imwrite(outFilename, dst)
