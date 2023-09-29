// Copyright (C) 2023 Intel Corporation
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http ://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// Author: Douglas P. Bogia

// Quick and dirty command line argument passing.  Drafted our own so it should be portable to either Linux or Windows

#include "ParseArgs.hpp"
#include <stdio.h>
#include <string.h>
//#include <math.h>
#include <stdlib.h>
#include <iostream>

void InitializeParameters(SParameters * parameters)
{
    parameters->m_startAlgorithm = 0;
    parameters->m_endAlgorithm = MAX_ALGORITHM;
    parameters->m_algorithm = -1;
    parameters->m_yaw = 0;
    parameters->m_pitch = 0;
    parameters->m_roll = 0;                     // Sometimes referred to as psi.  Should be -180 to 180 degrees
    parameters->m_deltaYaw = 0;
    parameters->m_deltaPitch = 0;
    parameters->m_deltaRoll = 0;
    parameters->m_deltaImage = false;
    parameters->m_fov = 60;
    parameters->m_widthOutput = 1080;
    parameters->m_heightOutput = 540;
    strcpy_s(parameters->m_imgFilename[0], "..\\..\\..\\images\\IMG_20230629_082736_00_095.jpg");
    strcpy_s(parameters->m_imgFilename[1], "..\\..\\..\\images\\ImageAndOverlay-equirectangular.jpg");
    // m_iterations = 0 means interactive
    parameters->m_iterations = 0;
    for (int i = 0; i < 3; i++)
    {
        parameters->m_offsets[i] = 0;
    }
    parameters->m_imageIndex = 0;
    parameters->m_typePreference = "";
    parameters->m_platformName = "";
    parameters->m_deviceName = "";
    parameters->m_driverVersion = "";
}

bool ParseArgs(int argc, char** argv, SParameters *parameters, char *errorMessage)
{
    char* flagStart;
    char* valueStart;
    char* equalSign;
    int argLength;
    int flagLength;
    bool bRetVal = true;

    for (int i = 1; i < argc; i++)
    {
        flagStart = NULL;
        valueStart = NULL;
        // See if the argument is a flag (starts with - or /)
        if (argv[i][0] == '-' || argv[i][0] == '/')
        {
            int j = 1;

            argLength = strlen(argv[i]);
            // Allow for multiple flag start characters (e.g., --)
            while (j < argLength && argv[i][j] == argv[i][0])
            {
                j++;
            }

            // See if the flag has a value assigned (e.g., --flag=value)
            flagStart = &(argv[i][j]);
            equalSign = strchr(flagStart, '=');
            if (equalSign == NULL)
            {
                flagLength = argLength - j;
            }
            else
            {
                flagLength = equalSign - flagStart;
                // valueStart will point at the value portion of the flag (-flag=value) and the value portion will
                // be a null terminated string
                valueStart = (equalSign + 1);
            }
        }
        if (flagStart == NULL)
        {
            sprintf(errorMessage, "Error: unknown argument = %s", argv[i]);
            bRetVal = false;
            break;
        }
        else
        {
            // We will be a bit sloppy, we will look for the first matching flags that matches the flag that is passed
            // in.  Some of the flags have a number of characters that are the same (e.g. -inputType and -inputSize both
            // start with the same 5 characters) so if a person enters -input=5, we will accept it against the first matching
            // flag possibility.  If we wanted to be more robust, we should only accept if there are not two or more
            // matches.

            // Check all the flags that do not require a value (or we will ignore the value) first.
            // Once those are all done, the else portion will first check that a value exists since all the
            // remaining flags require a value
            if (_strnicmp("help", flagStart, flagLength) == 0 || _strnicmp("?", flagStart, flagLength) == 0)
            {
                // No error message, but force the Usage to be printed
                errorMessage[0] = NULL;
                bRetVal = false;
            }
            else if (_strnicmp("deltaImage", flagStart, flagLength) == 0)
            {
                parameters->m_deltaImage = true;
            }
            else
            {
                if (valueStart == NULL)
                {
                    sprintf(errorMessage, "Error: Unknown flag or missing value for %s", argv[i]);
                    bRetVal = false;
                    break;
                }
                else
                {
                    if (_strnicmp("yaw", flagStart, flagLength) == 0)
                    {
                        parameters->m_yaw = atoi(valueStart);
                        if (parameters->m_yaw < -180 || parameters->m_yaw > 180)
                        {
                            sprintf(errorMessage, "Error: Illegal value for yaw (%s).  Must be -180 to 180.", valueStart);
                            bRetVal = false;
                            break;
                        }
                    }
                    else if (_strnicmp("pitch", flagStart, flagLength) == 0)
                    {
                        parameters->m_pitch = atoi(valueStart);
                        if (parameters->m_pitch < -90 || parameters->m_pitch > 90)
                        {
                            sprintf(errorMessage, "Error: Illegal value for pitch (%s).  Must be -90 to 90.", valueStart);
                            bRetVal = false;
                            break;
                        }
                    }
                    else if (_strnicmp("roll", flagStart, flagLength) == 0)
                    {
                        parameters->m_roll = atoi(valueStart);
                        if (parameters->m_roll < 0 || parameters->m_roll > 360)
                        {
                            sprintf(errorMessage, "Error: Illegal value for roll (%s).  Must be 0 to 360.", valueStart);
                            bRetVal = false;
                            break;
                        }
                    }
                    else if (_strnicmp("deltaYaw", flagStart, flagLength) == 0)
                    {
                        parameters->m_deltaYaw = atoi(valueStart);
                        if (parameters->m_deltaYaw < -360 || parameters->m_deltaYaw > 360)
                        {
                            sprintf(errorMessage, "Error: Illegal value for delta yaw (%s).  Must be -360 to 360.", valueStart);
                            bRetVal = false;
                            break;
                        }
                    }
                    else if (_strnicmp("deltaPitch", flagStart, flagLength) == 0)
                    {
                        parameters->m_deltaPitch = atoi(valueStart);
                        if (parameters->m_deltaPitch < -90 || parameters->m_deltaPitch > 90)
                        {
                            sprintf(errorMessage, "Error: Illegal value for delta pitch (%s).  Must be -90 to 90.", valueStart);
                            bRetVal = false;
                            break;
                        }
                    }
                    else if (_strnicmp("deltaRoll", flagStart, flagLength) == 0)
                    {
                        parameters->m_deltaRoll = atoi(valueStart);
                        if (parameters->m_deltaRoll < -360 || parameters->m_deltaRoll > 360)
                        {
                            sprintf(errorMessage, "Error: Illegal value for delta roll (%s).  Must be -360 to 360.", valueStart);
                            bRetVal = false;
                            break;
                        }
                    }
                    else if (_strnicmp("fov", flagStart, flagLength) == 0)
                    {
                        parameters->m_fov = atoi(valueStart);
                        if (parameters->m_fov < 10 || parameters->m_roll > 120)
                        {
                            sprintf(errorMessage, "Error: Illegal value for fov (%s).  Must be 10 to 120.", valueStart);
                            bRetVal = false;
                            break;
                        }
                    }
                    else if (_strnicmp("algorithm", flagStart, flagLength) == 0)
                    {
                        parameters->m_algorithm = atoi(valueStart);
                        if (parameters->m_algorithm < -1 || parameters->m_algorithm > MAX_ALGORITHM)
                        {
                            sprintf(errorMessage, "Error: Illegal value for algorithm (%s).  Must be -1 to %d.", valueStart, MAX_ALGORITHM);
                            bRetVal = false;
                            break;
                        }
                    }
                    else if (_strnicmp("startAlgorithm", flagStart, flagLength) == 0)
                    {
                        parameters->m_startAlgorithm = atoi(valueStart);
                        if (parameters->m_startAlgorithm < 0 || parameters->m_startAlgorithm > MAX_ALGORITHM)
                        {
                            sprintf(errorMessage, "Error: Illegal value for startAlgorithm (%s).  Must be -1 to %d.", valueStart, MAX_ALGORITHM);
                            bRetVal = false;
                            break;
                        }
                    }
                    else if (_strnicmp("endAlgorithm", flagStart, flagLength) == 0)
                    {
                        parameters->m_endAlgorithm = atoi(valueStart);
                        if (parameters->m_endAlgorithm < -1 || parameters->m_endAlgorithm > MAX_ALGORITHM)
                        {
                            sprintf(errorMessage, "Error: Illegal value for m_endAlgorithm (%s).  Must be -1 to %d.", valueStart, MAX_ALGORITHM);
                            bRetVal = false;
                            break;
                        }
                    }
                    else if (_strnicmp("img0", flagStart, flagLength) == 0)
                    {
                        strcpy_s(parameters->m_imgFilename[0], valueStart);
                    }
                    else if (_strnicmp("img1", flagStart, flagLength) == 0)
                    {
                        strcpy_s(parameters->m_imgFilename[1], valueStart);
                    }
                    else if (_strnicmp("widthOutput", flagStart, flagLength) == 0)
                    {
                        parameters->m_widthOutput = atoi(valueStart);
                        if (parameters->m_widthOutput <= 0)
                        {
                            sprintf(errorMessage, "Error: Illegal value for widthOutput (%s).  Must be a positive number.", valueStart);
                            bRetVal = false;
                            break;
                        }
                    }
                    else if (_strnicmp("heightOutput", flagStart, flagLength) == 0)
                    {
                        parameters->m_heightOutput = atoi(valueStart);
                        if (parameters->m_heightOutput <= 0)
                        {
                            sprintf(errorMessage, "Error: Illegal value for heightOutput (%s).  Must be a positive number.", valueStart);
                            bRetVal = false;
                            break;
                        }
                    }
                    else if (_strnicmp("iterations", flagStart, flagLength) == 0)
                    {
                        parameters->m_iterations = atoi(valueStart);
                    }
                    else if (_strnicmp("typePreference", flagStart, flagLength) == 0)
                    {
                        parameters->m_typePreference = valueStart;
                    }
                    else if (_strnicmp("platformName", flagStart, flagLength) == 0)
                    {
                        parameters->m_platformName = valueStart;
                    }
                    else if (_strnicmp("deviceName", flagStart, flagLength) == 0)
                    {
                        parameters->m_deviceName = valueStart;
                    }
                    else if (_strnicmp("driverVersion", flagStart, flagLength) == 0)
                    {
                        parameters->m_driverVersion = valueStart;
                    }
                    else
                    {
                        sprintf(errorMessage, "Error: Unknown flag = %s", argv[i]);
                        bRetVal = false;
                        break;
                    }
                }
            }
        }
    }

    return bRetVal;
}

// PrintUsage can be called to print out how to operate this program.
void PrintUsage(char* pProgramName, char* pMessage)
{
    printf("%s\n", pMessage);
    printf("Usage: %s [flags]\n", pProgramName);
    printf("Where: flags can be zero or more of the following (all flags are case insensitive):\n");
    printf("--algorithm=N where N is the number of the algorithm to use during the run. If this is set non-negative, it takes\n");
    printf("   precedence over --startAlgorithm and --endAlgorithm.  Defaults to - 1.\n");
    printf("   -1 = execute algorithms from --startAlgorithm to --endAlgorithm.\n");
    printf("    0 = serial point by point conversion from equirectangular to flat.  Memory array of structure row/column layout.\n");
    printf("    1 = serial point by point conversion from equirectangular to flat.  Memory array of structure column/row layout.\n");
    printf("    2 = serial point by point conversion from equirectangular to flat.  Memory structure of arrays layout.\n");
    printf("    3 = parallel conversion from equirectangular to flat.  Memory array of structure row/column layout.\n");
    printf("    4 = parallel conversion from equirectangular to flat.  Memory array of structure column/row layout.\n");
    printf("    5 = parallel conversion from equirectangular to flat.  Memory structure of arrays layout.\n");
    printf("    6 = DPC++ conversion from equirectangular to flat.\n");
    printf("--deltaImage is a flag to indicate that the image should be changed between each iteration to\n");
    printf("    simulate a video stream.\n");
    printf("--deltaPitch=N where N is the amount of pitch to add each iteration (up or down).  This can run from\n");
    printf("    -90 to 90 integer degrees.  The negative values are down and positive are up.  Default is 0\n");
    printf("--deltaRoll=N where N is the amount of roll to add each iteration.  This can run from -360 to 360 degrees.\n");
    printf("    Default is 0\n");
    printf("--deltaYaw=N where N is the amount of yaw to add each iteration.  This can run from -360 to 360 degrees.\n");
    printf("    Default is 0.\n");
    printf("--deviceName=value where value is a string to match against device names to");
    printf("   select the best device to run the code on.  Other options include all to\n");
    printf("   run on all devices or list to list device options.  Only used for DPC++ algorithms.\n");
    printf("   Defaults to empty string (select any)\n");
    printf("--driverVersion=value where value is a version number to select.  Only used for\n");
    printf("   DPC++ algorithms.  Defaults to empty string (select any)\n");
    printf("--endAlgorithm=N where N denotes the last algorithm to run.  Use -1 to run to end of all algorithms.\n");
    printf("    Defaults to -1");
    printf("--fov the number of integer degrees wide to use when flattening the image.  This can be from 1 to 120.  Default is 60.\n");
    printf("--heightOutput=N where N is the number of pixels height the flattened image will be.  Default is 540.\n");
    printf("--help|-h|-? means to display the usage message\n");
    printf("--img0=filePath where filePath is the path to an equirectangular image to load for the first frame.  Default image1.jpg.\n");
    printf("--img1=filePath where filePath is the path to an equirectangular image to load for the second frame.  Default image2.jpg.\n");
    printf("--iterations=N where N is the number of iterations.  Defaults to -1 if --algorithm=-1; otherwise 0 (interactive)\n");
    printf("--platformName=value where value is a string to match against platform names.\n");
    printf("   Other options include all to run on all platforms or list to list the platforms.\n");
    printf("   Only used for DPC++ algorithms.  Defaults to empty string (select any)\n");
    printf("--pitch=N where N is the pitch of the viewer's perspective (up or down).  This can run from\n");
    printf("    -90 to 90 integer degrees.  The negative values are down and positive are up.  0 is straight ahead.  Default is 0\n");
    printf("--roll=N where N defines how level the camera is.  This can run from 0 to 360 degrees.  The rotation is counter clockwise\n");
    printf("    so 90 integer degrees will lift the right side of the 'camera' up to be on top.  180 will flip the 'camera'\n");
    printf("    upside down.  270 will place the left side of the camera on top.  Default is 0\n");
    printf("--startAlgorithm=N where N defines the first algorithm number to run and then all algorithms up to and including\n");
    printf("    --endAlgorithm will be run in succession.  Defaults to 0.\n");
    printf("--typePreference=type1;type2;... where the types can be CPU, GPU, or \n");
    printf("    ACC (for Accelerator such as FPGA.  The first item is highest preference.\n");
    printf("--widthOutput=N where N is the number of pixels width the flattened image will be.  Default is 1080.\n");
    printf("--yaw=N where N defines the yaw of the viewer's perspective (left or right angle).  This can run from\n");
    printf("    -180 to 180 integer degrees.  Negative values are to the left of center and positive to the right.  0 is\n");
    printf("    straight ahead (center of equirectangular image).  Default is 0.\n");
    printf("\n");
    //printf("NOTE: You can set the environment variable OPENCV_OPENCL_DEVICE to :GPU or :CPU to target a specific HW usage.");
}

void PrintParameters(SParameters* parameters)
{
    printf("Output image size %dx%d\n", parameters->m_widthOutput, parameters->m_heightOutput);
    printf("Using algorithm = %d\n", parameters->m_algorithm);
    printf("Perspective: yaw = %d, pitch = %d, roll = %d\n", parameters->m_yaw, parameters->m_pitch, parameters->m_roll);
    printf("Field of view = %d\n", parameters->m_fov);
    printf("Input file = %s\n", parameters->m_imgFilename[parameters->m_imageIndex]);
}
