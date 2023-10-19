# Copyright (C) 2023 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

# This code was written to leverage the code from 
# https://github.com/fuenwang/Equirec2Perspec/blob/master/Equirec2Perspec.py but was also
# modified by Doug Bogia to test timing, add psi (roll) and tailor to a specific use case

import sys  # Access to the system
import getopt   # Access to getopt
import cv2  # Access to OpenCV
import Equirec2Perspec as E2P
import time

img = []
FOV = 90
window_width = 1080
window_height = 540
theta = 0           # Equivalent to yaw or pan (moving the camera lens left or right)
phi = 0             # Equivalent to pitch or tilt (moving the camera lens up or down)
psi = 0             # Equivalent to roll (raising or lowering the one side of the camera
                    # while keeping the other steady)
bDebug = False
bEnteringDelta = False
delta = 10
imgIndex = 0
img1 = "..\\..\\images\\IMG_20230629_082736_00_095.jpg"
img2 = "..\\..\\images\\ImageAndOverlay-equirectangular.jpg"

def usage():
    print("Usage: ", sys.argv[0], " [flags]")
    print("Where: flags can be zero or more of the following (all flags are case insensitive):")
    print("    --fov the number of integer degrees wide to use when flattening the image.  This can be from 1 to 120.")
    print("        Default is 90.")
    print("    --help|-h means to display the usage message");
    print("    --img0=filePath where filePath is the path to an equirectangular image to load for the first frame.")
    print("        One interesting one is  ..\..\images\globe-equirectangular.jpg")
    print("        Default is ", img1)
    print("    --img1=filePath where filePath is the path to an equirectangular image to load for the second frame.")
    print("        Default is ", img2)
    print("    --pitch=N where N is the pitch of the viewer's perspective (up or down).  This can run from")
    print("        -90 to 90 integer degrees.  The negative values are down and positive are up.  0 is straight ahead.")
    print("        Default is 0");
    print("    --roll=N where N defines how level the camera is.  This can run from 0 to 360 degrees.")
    print("        The rotation is counter clockwise so 90 integer degrees will lift the right side of the 'camera'")
    print("        up to be on top.  180 will flip the 'camera' upside down.  270 will place the left side of the")
    print("        camera on top.  Default is 0");
    print("    --yaw=N where N defines the yaw of the viewer's perspective (left or right angle).  This can run from");
    print("        -180 to 180 integer degrees.  Negative values are to the left of center and positive to the right.  0 is");
    print("        straight ahead (center of equirectangular image).  -90 is directly left.  Default is 0.");

try:
    opts, args = getopt.getopt(sys.argv[1:], "h", ["help", "img1=", "img2=", "fov=", "pitch=", "roll=", "yaw="])
except getopt.GetoptError as err:
    # print help information and exit:
    print(err)  # will print something like "option -a not recognized"
    usage()
    sys.exit(2)

for o, a in opts:
    if o in ("-h", "--help"):
        usage()
        sys.exit()
    elif o in ("--img1"):
        img1 = a
    elif o in ("--img2"):
        img2 = a
    elif o in ("--fov"):
        FOV = int(a)
    elif o in ("--pitch"):
        phi = int(a)
    elif o in ("--roll"):
        psi = int(a)
    elif o in ("--yaw"):
        theta = int(a)
    else:
        assert False, "unhandled option"

if (len(sys.argv) == 2 and sys.argv[1] == "globe"):
    img.append(cv2.imread("..\\..\\images\\globe-equirectangular.jpg", cv2.IMREAD_ANYCOLOR))
else:
    img.append(cv2.imread(img1, cv2.IMREAD_ANYCOLOR))
    img.append(cv2.imread(img2, cv2.IMREAD_ANYCOLOR))

cv2.namedWindow("Equirectangular Original", cv2.WINDOW_NORMAL)

equ = E2P.Equirectangular(img)
while True:
  cv2.imshow("Equirectangular Original", img[imgIndex])

  # Parameters are 
  # FOV is the number of degrees to include in the view
  # theta - is left/right angle (-180 to 180 degrees, but other numbers work too).  0 is straight ahead in the
  #         equirectangular image positive numbers (0 to 180) move towards the right and then behind.
  #         Negative numbers (0 to -180) work towards the left.  180 (or -180) stitches the left and right edges 
  #         together
  # phi - is up/down angle.  0 is mid point of image.  -90 looks directly down.  90 looks directly up.
  # height - The height of the flattened image
  # width - The width of the flattened image
  start_time = time.perf_counter()
  flatImg, XY = equ.GetPerspective(imgIndex, FOV, theta, phi, psi, height=window_height, width=window_width)
  end_time = time.perf_counter()
  print("Time required = ", end_time - start_time)
  print("Max FPS = ", 1 / (end_time - start_time))

  cv2.imshow("Flat View", flatImg)

  if bDebug:
    # When debugging, we want to show the original image with an outline of the Region of Interest
    # The color is BGR
    debugImg = img[imgIndex].copy()
    (height, width) = XY.shape[:2]
    for pt in XY[0]:
      cv2.line(debugImg, (int(pt[0]), int(pt[1])), (int(pt[0]), int(pt[1])), (255, 0, 0), 10)
    for y in range(1, height-2):
      cv2.line(debugImg, (int(XY[y][0][0]), int(XY[y][0][1])), (int(XY[y][0][0]), int(XY[y][0][1])), (0, 0, 255), 10)
      cv2.line(debugImg, (int(XY[y][width-1][0]), int(XY[y][width-1][1])), (int(XY[y][width-1][0]), int(XY[y][width-1][1])), (0, 255, 0), 10)
    for pt in XY[height-1]:
      cv2.line(debugImg, (int(pt[0]), int(pt[1])), (int(pt[0]), int(pt[1])), (74, 136, 175), 10)

    cv2.namedWindow("Debug View", cv2.WINDOW_NORMAL)
    cv2.imshow("Debug View", debugImg)
    
  key = cv2.waitKeyEx(0)
  
  if key == 27:           # Escape key
    break;                # Leave the loop and shutdown
  elif key == 113:         # q key (quit)
    break;
  
  if key >= 48 and key <= 57: # Entering numbers so we will use that to build up the delta amount
    if bEnteringDelta:
      delta = delta * 10 + (key - 48)
    else:
      bEnteringDelta = True
      delta = key - 48
  else:
    bEnteringDelta = False
    if key == 2555904:      # Right arrow key
      theta += delta
    elif key == 2621440:    # Down arrow key
      phi -= delta
    elif key == 2424832:    # Left arrow key
      theta -= delta
    elif key == 2490368:    # Up arrow key
      phi += delta
    elif key == 2162688:    # Page Up key (roll right upwards)
      psi += delta;
    elif key == 2228224:    # Page Down key (roll right downwards)
      psi -= delta;
    elif key == 2359296:    # Home key (roll left upwards)
      psi -= delta;
    elif key == 2293760:    # End key (roll left downwards)
      psi += delta;
    elif key == 42:         # * key
      FOV -= delta
    elif key == 47:         # / key
      FOV += delta
    elif key == 100:        # d key (toggle debug mode)
      bDebug = not bDebug
    elif key == 102:        # f key (change frame)
      if (len(img) > 1):
        imgIndex = (imgIndex + 1) % len(img)
    
  # Normalize the values so they are always in the range from -180 to 180 or -90 to 90, etc
  if (phi > 90):
    phi = 90
  elif (phi < -90):
    phi = -90
  if (phi > 180):
    phi = -180 + phi - 180
  elif (phi < -180):
    phi = 180 + theta + 180
  if (FOV <= 0):
    FOV = 10
  elif (FOV >= 120):
    FOV = 120
  if (theta > 180):
    theta = -180 + theta - 180
  if (theta < -180):
    theta = 180 + theta + 180
  if (psi >= 360):
    psi = psi - 360
  elif (psi < 0):
    psi = psi + 360
  print("Key = %d (0x%x)" % (key, key))
  print("FOV = ", FOV)
  print("theta (yaw/pan) = ", theta)
  print("phi (pitch/tilt) = ", phi)
  print("psi (roll) = ", psi)
  
cv2.destroyAllWindows()
