copy ..\..\images\IMG_20230629_082736_00_095.jpg .
copy ..\..\images\globe-equirectangular.jpg .
copy ..\..\images\globe-cubemap.jpg .
copy ..\..\images\generated360image-cubemap.jpg .
copy ..\..\images\generated360image-equirectangular.jpg .
copy ..\..\images\ImageAndOverlay-equirectangular.jpg .

python -m http.server
