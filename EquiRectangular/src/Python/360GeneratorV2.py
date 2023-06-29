# Code to take 6 cube faces (Front, Right, Back, Left, Above, and Below) and convert them into an equirectangular format

import sys  # Access to the system
import cv2  # Access to OpenCV
import numpy as np

# First generate the 6 cube faces so they are recognizable

#genWidth = 11968
#genHeight = 5984
#cubeWidth = 5984
#cubeHeight = 5984

genWidth = 1024
genHeight = 512
cubeWidth = 512
cubeHeight = 512

font = cv2.FONT_HERSHEY_SIMPLEX
fontScale = 3
fontLineWidth = 4
fontColor = (255, 255, 255)

def GenerateCubeFace(text, color, lineColor = (74, 136, 175), lineWidth = 2, bQuarters = True):
    flatImg = np.zeros((cubeHeight, cubeWidth, 3), np.uint8)
    flatImg[:] = color      # (B, G, R)
    textsize = cv2.getTextSize(text, font, fontScale, lineWidth)[0]
    textX = int((flatImg.shape[1] - textsize[0]) / 2)
    textY = int((flatImg.shape[0] + textsize[1]) / 2)
    cv2.putText(flatImg, text, (textX, textY), font, fontScale, fontColor, fontLineWidth)

    text = "TL"
    textsize = cv2.getTextSize(text, font, fontScale, lineWidth)[0]
    textX = 5
    textY = 5 + textsize[1]
    cv2.putText(flatImg, text, (textX, textY), font, fontScale, fontColor, fontLineWidth)
    
    text = "TR"
    textsize = cv2.getTextSize(text, font, fontScale, lineWidth)[0]
    textX = cubeWidth - 5 - textsize[0]
    textY = 5 + textsize[1]
    cv2.putText(flatImg, text, (textX, textY), font, fontScale, fontColor, fontLineWidth)
    
    text = "BL"
    textsize = cv2.getTextSize(text, font, fontScale, lineWidth)[0]
    textX = 5
    textY = cubeHeight - 5
    cv2.putText(flatImg, text, (textX, textY), font, fontScale, fontColor, fontLineWidth)
    
    text = "BR"
    textsize = cv2.getTextSize(text, font, fontScale, lineWidth)[0]
    textX = cubeWidth - 5 - textsize[0]
    textY = cubeHeight - 5
    cv2.putText(flatImg, text, (textX, textY), font, fontScale, fontColor, fontLineWidth)
    
    width = cubeWidth - 1
    height = cubeHeight - 1
    midX = int(cubeWidth/2 - 1)
    midY = int(cubeHeight/2 - 1)
    # Now we want to draw a line showing where each of the 45 degree marks are (the outer edges of each
    # Cube Face and both horizontal and vertical center lines)
    cv2.line(flatImg, (0, 0), (width, 0), lineColor, 2)
    cv2.line(flatImg, (0, 0), (0, height), lineColor, 2)
    cv2.line(flatImg, (width, 0), (width, height), lineColor, 2)
    cv2.line(flatImg, (0, height), (width, height), lineColor, 2)
    cv2.line(flatImg, (midX, 0), (midX, height), lineColor, 2)
    cv2.line(flatImg, (0, midY), (width, midY), lineColor, 2)
  
    return flatImg

##########################################################################################################
# Being main program  
genImg = np.zeros((genHeight, genWidth, 3), np.uint8)
genImg[:] = (0, 0, 0)

leftRightImg = np.zeros((genHeight, genWidth, 3), np.uint8)
leftRightImg[:] = (0, 0, 0)

frontImg = GenerateCubeFace("Front", (0, 0, 96))
rightImg = GenerateCubeFace("Right", (0, 96, 96))
backImg = GenerateCubeFace("Back", (0, 96, 0))
leftImg = GenerateCubeFace("Left", (96, 96, 0))
aboveImg = GenerateCubeFace("Above", (96, 0, 0))
belowImg = GenerateCubeFace("Below", (96, 0, 96))

# Go through each point on the equirectangular image to compute the pixel value to show
for j in range(0, genHeight-1):
    v = j / genHeight
    theta = v * np.pi
    cosTheta = np.cos(theta)
    negSinTheta = np.sin(theta) * -1
    for i in range(0, genWidth-1):
        u = i / genWidth
        phi = u * 2 * np.pi
        
        # Compute the unit vector x, y, and z coordinates
        # This assumes the radius (r) is 1
        x = np.sin(phi) * negSinTheta
        y = cosTheta
        z = np.cos(phi) * negSinTheta

        a = np.max((np.abs(x), np.abs(y), np.abs(z)))
        
        xa = x / a
        ya = y / a
        za = z / a
        
        if (xa == 1):
            # Right
            xPixel = -za
            yPixel = -ya
            img = rightImg
        elif (xa == -1):
            # Left
            xPixel = za
            yPixel = -ya
            img = leftImg
        elif (ya == 1):
            # Up
            xPixel = xa
            yPixel = za
            img = aboveImg
        elif (ya == -1):
            # Below
            xPixel = xa
            yPixel = -za
            img = belowImg
        elif (za == 1):
            # Front
            xPixel = xa
            yPixel = -ya
            img = frontImg
        elif (za == -1):
            # Back
            xPixel = -xa
            yPixel = -ya
            img = backImg

        xPixel = np.abs((int)((((xPixel + 1.0) / 2.0)) * (cubeWidth - 1.0)));
        yPixel = np.abs((int)((((yPixel + 1.0) / 2.0)) * (cubeHeight - 1.0)));

        genImg[j, i] = img[yPixel, xPixel]


cv2.imwrite("..\..\images\generated360image.jpg", genImg)

while True:
    cv2.namedWindow("Generated View", cv2.WINDOW_NORMAL)
    cv2.imshow("Generated View", genImg)
    
    key = cv2.waitKeyEx(0)
    if key == 27:               # Escape key
        break;                  # Leave the loop and shutdown
    
