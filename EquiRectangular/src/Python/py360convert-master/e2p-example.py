import sys  # Access to the system
import cv2  # Access to OpenCV
import py360convert

img = cv2.imread("O:\ToConvert\Warren\IMG_20230303_172908_00_009_PureShot.jpg", cv2.IMREAD_ANYCOLOR)

# The code below uses a 3 dimensional image since there are other functions that deal with equirectangle to
# cube conversion or vice versa, but the equirectangle to perspective code used below only uses the first
# two dimensions, so just make the 3rd dimission be None
if len(img.shape) == 2:
    img = img[..., None]

cv2.namedWindow("Equirectangular Original", cv2.WINDOW_NORMAL)
cv2.imshow("Equirectangular Original", img)

FOV = 60
theta = 90
phi = 0

while True:
  flatImg = py360convert.e2p(img, fov_deg=(FOV, FOV), u_deg=theta, v_deg=phi,
                           out_hw=(1080, 1080))

  cv2.imshow("Flat View", flatImg)

  key = cv2.waitKeyEx(0)
  
  if key == 27:           # Escape key
    break;                # Leave the loop and shutdown
  elif key == 2555904:    # Right arrow key
    theta += 30
  elif key == 2621440:    # Down arrow key
    phi -= 30
  elif key == 2424832:    # Left arrow key
    theta -= 30
  elif key == 2490368:    # Up arrow key
    phi += 30
  elif key == 43:         # + key
    FOV -= 10
  elif key == 45:         # - key
    FOV += 10
    
  # Normalize the values so they are always in the range from -180 to 180 or -90 to 90, etc
  if (phi > 90):
    phi = 90
  elif (phi < -90):
    phi = -90
  if (FOV <= 0):
    FOV = 10
  elif (FOV >= 120):
    FOV = 120
  if (theta > 180):
    theta = -180 + theta - 180
  if (theta < -180):
    theta = 180 + theta + 180
  print(key)
  print("FOV = ", FOV)
  print("theta = ", theta)
  print("phi = ", phi)
  
cv2.destroyAllWindows()
