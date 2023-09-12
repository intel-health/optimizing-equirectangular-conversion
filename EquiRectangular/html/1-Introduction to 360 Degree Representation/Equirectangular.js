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

// Derived from page 17 of https://www.cs.rpi.edu/~trink/Courses/RobotManipulation/lectures/lecture6.pdf
function Rodrigues(vec, angle)
{
    n0Squared = vec[0] * vec[0]
    n1Squared = vec[1] * vec[1]
    n2Squared = vec[2] * vec[2]
    cAngle = Math.cos(angle)
    sAngle = Math.sin(angle)
    oneMinusCAngle = 1 - cAngle
    oneMinusSAngle = 1 - sAngle

    return ([ [ n0Squared + (1 - n0Squared) * cAngle, vec[0] * vec[1] * oneMinusCAngle - vec[2] * sAngle, vec[0] * vec[2] * oneMinusCAngle + vec[1] * sAngle, ],
             [ vec[0] * vec[1] * oneMinusCAngle + vec[2] * sAngle, n1Squared + (1 - n1Squared) * cAngle, vec[1] * vec[2] * oneMinusCAngle - vec[0] * sAngle ],
             [ vec[0] * vec[2] * oneMinusCAngle - vec[1] * sAngle, vec[1] * vec[2] * oneMinusCAngle + vec[0] * sAngle, n2Squared + (1 - n2Squared) * cAngle ]]);
}

function matrixMultiply(mat1, mat2)
{
    // Should do bounds checking, but for now assume the matrices are of the right size
    retMat = new Array(mat1.length);
    for (row = 0; row < mat1.length; row++)
    {
        retMat[row] = new Array(mat2[0].length);
        for (col = 0; col < mat2[0].length; col++)
        {
            retMat[row][col] = 0
            for (k = 0; k < mat1[0].length; k++)
            {                
                retMat[row][col] += mat1[row][k] * mat2[k][col]
            }
        }
    }
    
    return retMat;
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
    
    var radTheta = flattenedCanvas.parameters.yaw * DEGREE_CONVERSION_FACTOR;
    var radPhi = flattenedCanvas.parameters.pitch * DEGREE_CONVERSION_FACTOR;
    var radPsi = flattenedCanvas.parameters.roll * DEGREE_CONVERSION_FACTOR;

    R1a = Rodrigues([0, 1, 0], radTheta)
    // The following line of code for R2a is the equivalent of the following python
    // R2a = cv2.Rodrigues(np.dot(R1a, [1, 0, 0]), np.radians(PHI))
    R2a = Rodrigues([R1a[0][0], R1a[1][0], R1a[2][0]], radPhi)
    // The following line of code for R3a is the equivalent of the following python
    // R3a = cv2.Rodrigues(np.dot(R1a, np.dot(R2a, [0, 0, -1])), np.radians(PSI))
    R3a = Rodrigues([R1a[0][0] * -R2a[0][2] + R1a[0][1] * -R2a[1][2] + R1a[0][2] * -R2a[2][2],
                     R1a[1][0] * -R2a[0][2] + R1a[1][1] * -R2a[1][2] + R1a[1][2] * -R2a[2][2],
                     R1a[2][0] * -R2a[0][2] + R1a[2][1] * -R2a[1][2] + R1a[2][2] * -R2a[2][2]], radPsi)

    Ra = matrixMultiply(R3a, R2a)
    Ra = matrixMultiply(Ra, R1a)
    // Increases speed slightly by caching the array values
    Ra00 = Ra[0][0]
    Ra01 = Ra[0][1]
    Ra02 = Ra[0][2]
    Ra10 = Ra[1][0]
    Ra11 = Ra[1][1]
    Ra12 = Ra[1][2]
    Ra20 = Ra[2][0]
    Ra21 = Ra[2][1]
    Ra22 = Ra[2][2]
        
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

            // x = eX * Ra[0][0] + eY * Ra[0][1] + eZ * Ra[0][2];
            // y = eX * Ra[1][0] + eY * Ra[1][1] + eZ * Ra[1][2];
            // z = eX * Ra[2][0] + eY * Ra[2][1] + eZ * Ra[2][2];
            x = eX * Ra00 + eY * Ra01 + eZ * Ra02;
            y = eX * Ra10 + eY * Ra11 + eZ * Ra12;
            z = eX * Ra20 + eY * Ra21 + eZ * Ra22;

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
        case 56:     // 8 key (we will just assume the intention is the * even if not shifted)
        case 106:    // NumpadMultiply
            e.target.parameters.fov -= defaultIncrement;
            updateRequired = true;
            break;
        case 111:    // NumpadDivide
        case 191:    // Main keyboard divide
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


