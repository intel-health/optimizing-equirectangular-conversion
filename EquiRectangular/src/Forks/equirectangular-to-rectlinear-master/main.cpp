#include "Equi2Rect.hpp"
#include <iostream>
#include <stdio.h>
#include <time.h>
#include "opencv2/core/utils/instrumentation.hpp"


#define LOG(msg) std::cout << msg << std::endl

int main(int argc, const char **argv)
{
    Equi2Rect equi2rect;

    equi2rect.save_rectlinear_image();

    equi2rect.show_rectlinear_image();

    return 0;
}