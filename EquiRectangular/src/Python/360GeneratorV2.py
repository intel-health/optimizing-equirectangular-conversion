# This was adapted from the code found in https://stackoverflow.com/questions/34250742/converting-a-cubemap-into-equirectangular-panorama
# Code to take 6 cube faces (Front, Right, Back, Left, Above, and Below) and convert them into an equirectangular format

import sys  # Access to the system
import cv2  # Access to OpenCV
import numpy as np

# First generate the 6 cube faces so they are recognizable

genWidth = 11968
genHeight = 5984
cubeWidth = 5984
cubeHeight = 5984

#genWidth = 1024
#genHeight = 512
#cubeWidth = 512
#cubeHeight = 512

font = cv2.FONT_HERSHEY_SIMPLEX
fontScale = cubeWidth / 200
fontLineWidth = int(cubeWidth / 100)
fontColor = (255, 255, 255)

lineWidth = int(cubeWidth / 100)

def GenerateCubeFace(text, color, lineWidth = 5, horizontalLineColor = (74, 136, 175), verticalLineColor = (136, 175, 74)):
    flatImg = np.zeros((cubeHeight, cubeWidth, 3), np.uint8)
    flatImg[:] = color      # (B, G, R)
    
    width = cubeWidth - 1
    height = cubeHeight - 1
    # Draw lines every 10 degrees
    for i in range(0, 100, 10):
        if i == 0:
            X = 0
            Y = 0
        else:
            X = int(cubeWidth * (i / 90) - 1)
            Y = int(cubeHeight * (i / 90) - 1)
        # Draw horizontal
        cv2.line(flatImg, (0, Y), (width, Y), horizontalLineColor, lineWidth)
        # Draw vertical
        cv2.line(flatImg, (X, 0), (X, height), verticalLineColor, lineWidth)
  
    
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
    
    #cv2.namedWindow("Debug View", cv2.WINDOW_NORMAL)
    #cv2.imshow("Debug View", flatImg)
    #key = cv2.waitKeyEx(0)
    
    return flatImg

##########################################################################################################
# Begin main program  
genImg = np.zeros((genHeight, genWidth, 3), np.uint8)
genImg[:] = (0, 0, 0)

leftRightImg = np.zeros((genHeight, genWidth, 3), np.uint8)
leftRightImg[:] = (0, 0, 0)

frontImg = GenerateCubeFace("Front", (40, 40, 96), lineWidth)
rightImg = GenerateCubeFace("Right", (40, 96, 96), lineWidth)
backImg = GenerateCubeFace("Back", (40, 96, 40), lineWidth)
leftImg = GenerateCubeFace("Left", (96, 96, 40), lineWidth)
aboveImg = GenerateCubeFace("Above", (96, 40, 40), lineWidth)
belowImg = GenerateCubeFace("Below", (96, 40, 96), lineWidth)

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


cv2.imwrite("..\..\images\generated360image-equirectangular.jpg", genImg)

while True:
    cv2.namedWindow("Generated View", cv2.WINDOW_NORMAL)
    cv2.imshow("Generated View", genImg)
    
    key = cv2.waitKeyEx(0)
    if key == 27:               # Escape key
        break;                  # Leave the loop and shutdown
    
