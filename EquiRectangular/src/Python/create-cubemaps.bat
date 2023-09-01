REM Copyright (C) 2023 Intel Corporation
REM SPDX-License-Identifier: Apache-2.0

python .\py360convert-master\convert360 --convert e2c --i ../../images/IMG_20230629_082736_00_095.jpg --o ../../images/IMG_20230629_082736_00_095-cubemap.jpg --w 11968

python .\py360convert-master\convert360 --convert e2c --i ../../images/ImageAndOverlay-equirectangular.jpg --o ../../images/ImageAndOverlay-cubemap.jpg --w 11968

python .\py360convert-master\convert360 --convert e2c --i ../../images/generated360image-equirectangular.jpg --o ../../images/generated360image-cubemap.jpg --w 11968

python .\py360convert-master\convert360 --convert e2c --i ../../images/globe-equirectangular.jpg --o ../../images/globe-cubemap.jpg --w 11968

