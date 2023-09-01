# This code generates a view of what it would be like to take an image inside a sphere with all the 10 degree longitude and latitude lines are drawn
# and draws this as an equirectangular surface.

import sys  # Access to the system
import cv2  # Access to OpenCV
import numpy as np

genWidth = 11968
genHeight = 5984

font = cv2.FONT_HERSHEY_SIMPLEX
fontScale = 3
fontLineWidth = 10

lineWidth = 10
# Color is bgr
backgroundColor = (0, 0, 0)
horizontalLineColor = (74, 136, 175)
verticalLineColor = (136, 175, 74)
equatorLineColor = (0, 0, 255)
meridianLineColor = (192, 192, 192)
horizontalFontColor = equatorLineColor
verticalFontColor = meridianLineColor
degreeIncrement = 10

# Begin main program  
genImg = np.zeros((genHeight, genWidth, 3), np.uint8)
genImg[:] = backgroundColor

# Step through each of the degree increments that were requested in the vertical direction
for j in range(0, 179, degreeIncrement):
    y = int((j / 180) * genHeight)
    # Draw horizontal
    if (j == 90):
        cv2.line(genImg, (0, y), (genWidth, y), equatorLineColor, lineWidth)
    else:
        cv2.line(genImg, (0, y), (genWidth, y), horizontalLineColor, lineWidth)
    text = str(90 - j)
    textsize = cv2.getTextSize(text, font, fontScale, fontLineWidth)[0]
    textX = int(genWidth * 0.5) + 10
    textY = y - textsize[1] + 40
    cv2.putText(genImg, text, (textX, textY), font, fontScale, horizontalFontColor, fontLineWidth)

        
# Step through each of the degree increments that were requested in the horizontal direction
for i in range(0, 359, degreeIncrement):
    x = int((i / 360) * genWidth)
    # Draw vertical
    if (i == 180):
        cv2.line(genImg, (x, 0), (x, genHeight), meridianLineColor, lineWidth)
    else:
        cv2.line(genImg, (x, 0), (x, genHeight), verticalLineColor, lineWidth)
    text = str(-180 + i)
    textsize = cv2.getTextSize(text, font, fontScale, fontLineWidth)[0]
    textX = x - textsize[0]
    textY = int(genHeight * 0.5) - 30
    cv2.putText(genImg, text, (textX, textY), font, fontScale, verticalFontColor, fontLineWidth)

cv2.imwrite("..\..\images\globe-equirectangular.jpg", genImg)

while True:
    cv2.namedWindow("Generated View", cv2.WINDOW_NORMAL)
    cv2.imshow("Generated View", genImg)
    
    key = cv2.waitKeyEx(0)
    if key == 27:               # Escape key
        break;                  # Leave the loop and shutdown



