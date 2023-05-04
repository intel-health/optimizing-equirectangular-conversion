// Quick and dirty command line argument passing.  Drafted our own so it should be portable to either Linux or Windows

#include "ParseArgs.hpp"
#include <stdio.h>
#include <string.h>
//#include <math.h>
#include <stdlib.h>

const int INDEX_YAW = 0;
const int INDEX_PITCH = 1;
const int INDEX_ROLL = 2;

void InitializeParameters(SParameters * parameters)
{
    parameters->m_algorithm = 0;
    parameters->m_yaw = 0;
    parameters->m_pitch = 0;
    parameters->m_roll = 0;                     // Sometimes referred to as psi.  Should be -180 to 180 degrees
    parameters->m_deltaYaw = 0;
    parameters->m_deltaPitch = 0;
    parameters->m_deltaRoll = 0;
    parameters->m_fov = 60;
    parameters->m_widthOutput = 1080;
    parameters->m_heightOutput = 540;
    strcpy(parameters->m_imgFilename[0], "image1.jpg");
    strcpy(parameters->m_imgFilename[1], "image2.jpg");
    // m_iterations = 0 means interactive
    parameters->m_iterations = 0;
    // TODO: Add ability to enter yaw,pitch,roll on command line and then have these be added to parameters over each iteration
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
    int valueLength;
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
            if (strnicmp("help", flagStart, flagLength) == 0 || strnicmp("?", flagStart, flagLength) == 0)
            {
                // No error message, but force the Usage to be printed
                errorMessage[0] = NULL;
                bRetVal = false;
//            }
//            else if (strnicmp("showDiffs", flagStart, flagLength) == 0)
//            {
//                g_bShowDiffs = true;
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
                    if (strnicmp("yaw", flagStart, flagLength) == 0)
                    {
                        parameters->m_yaw = atoi(valueStart);
                        if (parameters->m_yaw < -180 || parameters->m_yaw > 180)
                        {
                            sprintf(errorMessage, "Error: Illegal value for yaw (%s)", valueStart);
                            bRetVal = false;
                            break;
                        }
                    }
                    else if (strnicmp("pitch", flagStart, flagLength) == 0)
                    {
                        parameters->m_pitch = atoi(valueStart);
                        if (parameters->m_pitch < -90 || parameters->m_pitch > 90)
                        {
                            sprintf(errorMessage, "Error: Illegal value for pitch (%s)", valueStart);
                            bRetVal = false;
                            break;
                        }
                    }
                    else if (strnicmp("roll", flagStart, flagLength) == 0)
                    {
                        parameters->m_roll = atoi(valueStart);
                        if (parameters->m_roll < 0 || parameters->m_roll > 360)
                        {
                            sprintf(errorMessage, "Error: Illegal value for roll (%s)", valueStart);
                            bRetVal = false;
                            break;
                        }
                    }
                    else if (strnicmp("deltaYaw", flagStart, flagLength) == 0)
                    {
                        parameters->m_deltaYaw = atoi(valueStart);
                        if (parameters->m_deltaYaw < -360 || parameters->m_deltaYaw > 360)
                        {
                            sprintf(errorMessage, "Error: Illegal value for delta yaw (%s)", valueStart);
                            bRetVal = false;
                            break;
                        }
                    }
                    else if (strnicmp("deltaPitch", flagStart, flagLength) == 0)
                    {
                        parameters->m_deltaPitch = atoi(valueStart);
                        if (parameters->m_deltaPitch < -90 || parameters->m_deltaPitch > 90)
                        {
                            sprintf(errorMessage, "Error: Illegal value for delta pitch (%s)", valueStart);
                            bRetVal = false;
                            break;
                        }
                    }
                    else if (strnicmp("deltaRoll", flagStart, flagLength) == 0)
                    {
                        parameters->m_deltaRoll = atoi(valueStart);
                        if (parameters->m_deltaRoll < -360 || parameters->m_deltaRoll > 360)
                        {
                            sprintf(errorMessage, "Error: Illegal value for delta roll (%s)", valueStart);
                            bRetVal = false;
                            break;
                        }
                    }
                    else if (strnicmp("fov", flagStart, flagLength) == 0)
                    {
                        parameters->m_fov = atoi(valueStart);
                        if (parameters->m_fov < 10 || parameters->m_roll > 120)
                        {
                            sprintf(errorMessage, "Error: Illegal value for fov (%s)", valueStart);
                            bRetVal = false;
                            break;
                        }
                    }
                    else if (strnicmp("algorithm", flagStart, flagLength) == 0)
                    {
                        parameters->m_algorithm = atoi(valueStart);
                        if (parameters->m_algorithm < -1 || parameters->m_algorithm > 6)
                        {
                            sprintf(errorMessage, "Error: Illegal value for algorithm (%s)", valueStart);
                            bRetVal = false;
                            break;
                        }
                    }
                    else if (strnicmp("img0", flagStart, flagLength) == 0)
                    {
                        strcpy(parameters->m_imgFilename[0], valueStart);
                    }
                    else if (strnicmp("img1", flagStart, flagLength) == 0)
                    {
                        strcpy(parameters->m_imgFilename[1], valueStart);
                    }
                    else if (strnicmp("widthOutput", flagStart, flagLength) == 0)
                    {
                        parameters->m_widthOutput = atoi(valueStart);
                        if (parameters->m_widthOutput <= 0)
                        {
                            sprintf(errorMessage, "Error: Illegal value for widthOutput (%s)", valueStart);
                            bRetVal = false;
                            break;
                        }
                    }
                    else if (strnicmp("heightOutput", flagStart, flagLength) == 0)
                    {
                        parameters->m_heightOutput = atoi(valueStart);
                        if (parameters->m_heightOutput <= 0)
                        {
                            sprintf(errorMessage, "Error: Illegal value for heightOutput (%s)", valueStart);
                            bRetVal = false;
                            break;
                        }
                    }
                    else if (strnicmp("iterations", flagStart, flagLength) == 0)
                    {
                        parameters->m_iterations = atoi(valueStart);
                    }
                    else if (strnicmp("typePreference", flagStart, flagLength) == 0)
                    {
                        parameters->m_typePreference = valueStart;
                    }
                    else if (strnicmp("platformName", flagStart, flagLength) == 0)
                    {
                        parameters->m_platformName = valueStart;
                    }
                    else if (strnicmp("deviceName", flagStart, flagLength) == 0)
                    {
                        parameters->m_deviceName = valueStart;
                    }
                    else if (strnicmp("driverVersion", flagStart, flagLength) == 0)
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
    printf("--algorithm=N where N is the number of the algorithm to use during the run.  This can be from -1 to 2.  Default is 0.\n");
    printf("   -1 = execute --iterations of each algorithm listed below.\n");
    printf("    0 = serial point by point conversion from equirectangular to flat.  Memory array of structure row/column layout.\n");
    printf("    1 = serial point by point conversion from equirectangular to flat.  Memory array of structure column/row layout.\n");
    printf("    2 = serial point by point conversion from equirectangular to flat.  Memory structure of arrays layout.\n");
    printf("    3 = parallel conversion from equirectangular to flat.  Memory array of structure row/column layout.\n");
    printf("    4 = parallel conversion from equirectangular to flat.  Memory array of structure column/row layout.\n");
    printf("    5 = parallel conversion from equirectangular to flat.  Memory structure of arrays layout.\n");
    printf("    6 = DPC++ conversion from equirectangular to flat.\n");
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
    printf("    -180 to 180 integer degrees.  The negative values are down and positive are up.  0 is straight ahead.  Default is 0\n");
    printf("--roll=N where N defines how level the camera is.  This can run from 0 to 360 degrees.  The rotation is counter clockwise\n");
    printf("    so 90 integer degrees will lift the right side of the 'camera' up to be on top.  180 will flip the 'camera'\n");
    printf("    upside down.  270 will place the left side of the camera on top.  Default is 0\n");
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
