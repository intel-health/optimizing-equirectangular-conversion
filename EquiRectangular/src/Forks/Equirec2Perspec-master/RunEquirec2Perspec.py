import os
import cv2 
import Equirec2Perspec as E2P 
import time

if __name__ == '__main__':
    equ = E2P.Equirectangular('..\..\\..\\images\\IMG_20230629_082736_00_095.jpg')    # Load equirectangular image
    
    iterations = 100
    start_time = time.perf_counter()
    for i in range(0, iterations, 1):
        #
        # FOV unit is degree 
        # theta is z-axis angle(right direction is positive, left direction is negative)
        # phi is y-axis angle(up direction positive, down direction negative)
        # height and width is output image dimension 
        #
        img = equ.GetPerspective(60, 0, 0, 540, 1080) # Specify parameters(FOV, theta, phi, height, width)
    
    end_time = time.perf_counter()
    aveDuration = (end_time - start_time) / iterations
    print("Average for ", iterations, " frames ", aveDuration, "s ", aveDuration * 1000, "ms ", aveDuration * 1000000, "us ", iterations / (end_time - start_time), " FPS")
    
    cv2.imshow("Flat View", img)
    cv2.waitKeyEx(0)
