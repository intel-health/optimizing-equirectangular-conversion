// Copyright (C) 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

const defaultYaw = 0;
const defaultPitch = 0;
const defaultRoll = 0;
const defaultFoV = 90;
const defaultIncrement = 10;
const DEGREE_CONVERSION_FACTOR = 2 * Math.PI / 360.0;

// arrayInfo is an array of arrays.  Each sub-array should have element 1: the URL for the image to load, 2: the canvas id where to place
// the flattened image, 3: the canvas id where to place the raw image.  Both canvases must exist on the web page, but the display state 
// can be set to none for the canvas elements that should not be seen by the user.
function displayFlattened(arrayInfo) {
    for (i = 0; i < arrayInfo.length; i++)
    {
        var imageURL;
        var flattenedCanvas
        var rawCanvas
        
        imageInfo = arrayInfo[i]
        if (imageInfo.length == 3)
        {
            imageURL = imageInfo[0]
            flattenedCanvas = document.getElementById(imageInfo[1])
            rawCanvas = document.getElementById(imageInfo[2])
        }
        else
        {
            // Illegal number of array elements.
            return;
        }
        
        var img = new Image();
        img.onload = imageLoaded;
        img.data_flattenedCanvas = flattenedCanvas
        img.data_rawCanvas = rawCanvas
        img.src = imageURL;		
        
        flattenedCanvas.onmousedown = mouseDown;
        flattenedCanvas.onmousemove = mouseMove;
        flattenedCanvas.onmouseup = mouseUp;
        flattenedCanvas.onmousewheel = mouseScroll;
        flattenedCanvas.onkeydown = keyDown;
        // Remember the corresponding rawCanvas
        flattenedCanvas.rawCanvas = rawCanvas
        flattenedCanvas.parameters = {}
        flattenedCanvas.parameters.yaw = defaultYaw;
        flattenedCanvas.parameters.pitch = defaultPitch;
        flattenedCanvas.parameters.roll = defaultRoll;
        flattenedCanvas.parameters.fov = defaultFoV;
    }
}

function imageLoaded()
{
	var   data_flattenedCanvasCtx = this.data_flattenedCanvas.getContext("2d", { willReadFrequently: true });
   	var   data_rawCanvasCanvasCtx = this.data_rawCanvas.getContext("2d", { willReadFrequently: true });

	this.data_rawCanvas.width = this.width;
	this.data_rawCanvas.height = this.height;

 	// Draw image into the raw image context
	data_rawCanvasCanvasCtx.drawImage(this, 0, 0);

    displayFlattenedImage(this.data_flattenedCanvas);
}

function displayFlattenedImage(flattenedCanvas)
{
    var rawCanvas = flattenedCanvas.rawCanvas;
    var rawCanvasCtx = rawCanvas.getContext("2d");
    var flattenedCanvasCtx = flattenedCanvas.getContext("2d");
	var flattenedImgData = flattenedCanvasCtx.getImageData(0, 0, flattenedCanvas.width, flattenedCanvas.height);
	var flattenedPixels = flattenedImgData.data;
	var rawImgData = rawCanvasCtx.getImageData(0, 0, rawCanvas.width, rawCanvas.height);
	var rawPixels = rawImgData.data;

    if (flattenedCanvas.parameters.pitch > 90)
    {
        flattenedCanvas.parameters.pitch = 90;
    }
    else if (flattenedCanvas.parameters.pitch < -90)
    {
        flattenedCanvas.parameters.pitch = -90;
    }
    if (flattenedCanvas.parameters.fov < 10)
    {
        flattenedCanvas.parameters.fov = 10;
    }
    else if (flattenedCanvas.parameters.fov > 120)
    {
        flattenedCanvas.parameters.fov = 120;
    }
    if (flattenedCanvas.parameters.yaw > 180)
    {
        // Wrap around to the other side of the 360 view
        flattenedCanvas.parameters.yaw = (-180 + flattenedCanvas.parameters.yaw) % 360 - 180;
    }
    else if (flattenedCanvas.parameters.yaw < -180)
    {
        flattenedCanvas.parameters.yaw = (180 + flattenedCanvas.parameters.yaw) % 360 + 180;
    }
    if (flattenedCanvas.parameters.roll < 0)
    {
        flattenedCanvas.parameters.roll = 360 + (flattenedCanvas.parameters.roll % -360);
    }
    else if (flattenedCanvas.parameters.roll >= 360)
    {
        flattenedCanvas.parameters.roll = flattenedCanvas.parameters.roll % 360;
    }
    

    var f = 0.5 * flattenedCanvas.width * 1 / Math.tan(0.5 * flattenedCanvas.parameters.fov / 180.0 * Math.PI);
    var cx = (flattenedCanvas.width - 1.0) / 2.0;
    var cy = (flattenedCanvas.height - 1.0) / 2.0;
    var invf;
    var translatecx;
    var translatecy;
    var imageWidth = rawCanvas.width;
    var imageHeight = rawCanvas.height;
    var xDiv = 2 * Math.PI;

    // In concept, the intent of this section of the code is to create an inverse of the intrinsic matrix K
    // (see https://ksimek.github.io/2013/08/13/intrinsic for explanation).  However, in this case we will
    // not represent as an actual matrix, but just calculate the individual values and then use them
    // to initialze the 2D array of XYZ points.
    // Initialize the inverse of the intrinsic matrix K
    //K_inv = (Mat_<float>(3, 3) <<
    //	1 / f, 0, -cx / f,
    //	0, 1 / f, -cy / f,
    //	0, 0, 1);
    invf = 1.0 / f;
    translatecx = -cx * invf;
    translatecy = -cy * invf;
    
    // Now calculate the rotational matrix elements.  See (1) in https://faculty.sites.iastate.edu/jia/files/inline-files/rotation.pdf
    // However, in our case the X axis goes to the right (i.e., is equal to Y axis in the article), the Y axis goes up (i.e., is
    // equal to the Z axis), and the Z axis comes outward (i.e., is equal to the X axis).  This we need to translate the phi, theta, and
    // psi values.
    var radPhi = flattenedCanvas.parameters.roll * DEGREE_CONVERSION_FACTOR;
    var radTheta = flattenedCanvas.parameters.yaw * DEGREE_CONVERSION_FACTOR;
    var radPsi = flattenedCanvas.parameters.pitch * DEGREE_CONVERSION_FACTOR;
    var cosPhi = Math.cos(radPhi)
    var cosTheta = Math.cos(radTheta)
    var cosPsi = Math.cos(radPsi)
    var sinPhi = Math.sin(radPhi)
    var sinTheta = Math.sin(radTheta)
    var sinPsi = Math.sin(radPsi)
    var m00 = cosPhi * cosTheta;
    var m01 = cosPhi * sinTheta * sinPsi - sinPhi * cosPsi;
    var m02 = cosPhi * sinTheta * cosPsi + sinPhi * sinPsi;
    var m10 = sinPhi * cosTheta;
    var m11 = sinPhi * sinTheta * sinPsi + cosPhi * cosPsi;
    var m12 = sinPhi * sinTheta * cosPsi - cosPhi * sinPsi;
    var m20 = -sinTheta;
    var m21 = cosTheta * sinPsi;
    var m22 = cosTheta * cosPsi;

    for (var row = 0; row < flattenedCanvas.height; row++)
    {
        for (var col = 0; col < flattenedCanvas.width; col++)
        {
            var x = col * invf + translatecx;
            var y = row * invf + translatecy;
            var z = 1.0;
            var norm;

            // Calculate xyz * R, save the initial x, y, and z values for the computation
            var eX = x;
            var eY = y;
            var eZ = z;

            x = eX * m00 + eY * m01 + eZ * m02;
            y = eX * m10 + eY * m11 + eZ * m12;
            z = eX * m20 + eY * m21 + eZ * m22;

            norm = Math.sqrt(x * x + y * y + z * z);

            x = Math.atan2(x / norm, z / norm);
            y = Math.asin(y / norm);

            m_x = Math.round((x / xDiv + 0.5) * imageWidth);
            m_y = Math.round((y / Math.PI + 0.5) * imageHeight);
            
            var	flattenedOffset = 4 * (row * flattenedCanvas.width + col);
            var	rawOffset = 4 * (m_y * rawCanvas.width + m_x);
            
            flattenedPixels[flattenedOffset]     = rawPixels[rawOffset];
            flattenedPixels[flattenedOffset+1]   = rawPixels[rawOffset+1];
            flattenedPixels[flattenedOffset+2]   = rawPixels[rawOffset+2];
            flattenedPixels[flattenedOffset+3]   = rawPixels[rawOffset+3];
        }
    }
    flattenedCanvasCtx.putImageData(flattenedImgData, 0, 0);
}

function mouseDown(e) {
    e.target.bMouseDown = true;
    e.target.lastX=e.clientX;
	e.target.lastY=e.clientY;	
}

function mouseMove(e) {
    if (e.target.bMouseDown)
    {
        e.target.parameters.yaw += ((e.target.lastX - e.clientX) / e.target.clientWidth) * e.target.parameters.fov;
        e.target.parameters.pitch -= ((e.target.lastY - e.clientY) / e.target.clientHeight) * e.target.parameters.fov;
        e.target.lastX=e.clientX;
        e.target.lastY=e.clientY;	
        displayFlattenedImage(e.target);
    }
}

function mouseUp(e) {
    e.target.bMouseDown = false;
}

function mouseScroll(e) {
    e.target.parameters.fov += -Math.sign(e.wheelDelta) * defaultIncrement;
    displayFlattenedImage(e.target);
    e.preventDefault();
}

function keyDown(e) {
    var updateRequired = false;
    switch (e.keyCode)
    {
        case 33:    // PageUp
            e.target.parameters.roll -= defaultIncrement;
            updateRequired = true;
            break;
        case 34:    // PageDown
            e.target.parameters.roll += defaultIncrement;
            updateRequired = true;
            break;
        case 35:    // End
            e.target.parameters.roll -= defaultIncrement;
            updateRequired = true;
            break;
        case 36:    // Home
            e.target.parameters.roll += defaultIncrement;
            updateRequired = true;
            break;
        case 37:    // ArrowLeft
            e.target.parameters.yaw -= defaultIncrement;
            updateRequired = true;
            break;
        case 38:    // ArrowUp
            e.target.parameters.pitch += defaultIncrement;
            updateRequired = true;
            break;
        case 39:    // ArrowRight
            e.target.parameters.yaw += defaultIncrement;
            updateRequired = true;
            break;
        case 40:    // ArrowDown
            e.target.parameters.pitch -= defaultIncrement;
            updateRequired = true;
            break;
        case 106:    // NumpadMultiply
            e.target.parameters.fov -= defaultIncrement;
            updateRequired = true;
            break;
        case 111:    // NumpadDivide
            e.target.parameters.fov += defaultIncrement;
            updateRequired = true;
            break;
    }
    if (updateRequired)
    {
        var startTime = new Date();
        displayFlattenedImage(e.target);
        var endTime = new Date();
        console.log((1000.0/(endTime.getTime()-startTime.getTime())).toFixed(1));
        e.preventDefault();
    }
    
}


