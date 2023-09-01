To make the images in the EquiRectangular images folder the following steps were taken:
- IMG_20230629_082736_00_095.jpg and IMG_20230629_082736_00_096.jpg were taken with an Insta360 X3 camera using the Interval setting
- generated360image-equirectangular.jpg was created by running python .\360GeneratorV2.py from the src\Python folder
- ImageAndOverlay-equirectangular.jpg was created using Visio and first loading IMG_20230629_082736_00_095.jpg and then 
  overlaying generated360image-equirectangular.jpg directly on top and using Picture Format->Brightness->Picture Correction Options and
  then changing the transparency of the generated360image-equirectangular.jpg to be 50% and saving the resulting image
- The three *-cubemap.jpg files were created by running create-cubemaps.bat from the src\Python folder
- 360SampleV2.py is the main python file used for the intial benchmarking of Python where it also saves the XY coords for efficiency.
- in the folder py360convert-master running "python e2p-example.py" is also instrumented to show time, but it is much slower