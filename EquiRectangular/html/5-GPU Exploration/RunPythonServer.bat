copy ..\..\images\author.jpg .
copy ..\..\images\VTune-OneDevice-Alg17-View-Zoom.jpg .
copy ..\..\images\VTune-OneDevice-Alg17-Image-Zoom.jpg .
copy ..\..\images\VTune-OneDevice-Alg14-Image-Zoom.jpg .
copy ..\..\images\VTune-TwoDevices-Alg17-ViewAndImage-Zoom.jpg .
copy ..\..\images\VTune-TwoDevices-Alg19-Image-Zoom.jpg .
copy ..\Extras\MakeZoomable.js .
copy ..\Extras\BlogStyles.css .

python -m http.server 8005
