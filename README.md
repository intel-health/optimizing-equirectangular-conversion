# README

This repository contains information about equirectangular images (360 degree panoramic images), conversion to rectilinear views (selecting a particular viewing direction and "flattening" the image,
a code framework to test out optimization ideas for efficiently extracting images, and web pages that explain all the work.

# Dependencies

The code depends on the following:

- [Intel&reg; oneAPI Base Tool Kit](https://www.intel.com/content/www/us/en/developer/tools/oneapi/toolkits.html).  All testing used version 2023.2
- Python.  Standard version](https://www.python.org/) or Intel&reg; Distribution for Python (https://www.intel.com/content/www/us/en/developer/tools/oneapi/distribution-for-python.html).
All testing was done with both installed on the machine.
- [OpenCV](https://sourceforge.net/projects/opencvlibrary/).  All testing used version 4.6 (specifically https://sourceforge.net/projects/opencvlibrary/files/4.6.0/opencv-4.6.0-vc14_vc15.exe/download).

It is recommended to install the tools listed above in the order listed.  OpenCV utilizes cmake which is included in the oneAPI Base Tool Kit.  It also uses Python at build time.


