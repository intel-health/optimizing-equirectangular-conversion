#include "Equi2Rect.hpp"
#include <math.h>
#include <chrono>>

Equi2Rect::Equi2Rect()
{
    // viewport size
    viewport.width = 1080;
    viewport.height = 540;

    // specify viewing direction
    viewport.pan_angle = 0;
    viewport.tilt_angle = 0;
    viewport.roll_angle = 0;

    // create rotation matrix
    Rot = eul2rotm(viewport.tilt_angle, viewport.pan_angle, viewport.roll_angle);

    // specify focal length of the final pinhole image
    focal_length = 672;
    ;

    // create camera matrix K
    K = (cv::Mat_<double>(3, 3) << focal_length, 0, viewport.width / 2,
         0, focal_length, viewport.height / 2,
         0, 0, 1);

#ifdef CACHE_ROT_KINV
    RotKinv = Rot * K.inv();
#endif
    // read src image
    img_src = cv::imread("..\\..\\..\\images\\IMG_20230629_082736_00_095.jpg", cv::IMREAD_COLOR);
    if (img_src.empty())
    {
        throw std::invalid_argument("Error: Could not load image!");
    }

    // initialize result image
    img_dst = cv::Mat(viewport.height, viewport.width, CV_8UC3, cv::Scalar(0, 0, 0));
}

auto Equi2Rect::save_rectlinear_image() -> void
{
    std::chrono::high_resolution_clock::time_point startTime = std::chrono::high_resolution_clock::now();
    int iterations = 100;

    for (int i = 0; i < iterations; i++)
    {
        viewport.pan_angle = i % 360;
        this->bilinear_interpolation();
    }
    std::chrono::high_resolution_clock::time_point stopTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> aveDuration = std::chrono::duration<double>(stopTime - startTime) / iterations;
    printf("Average for %d frames %12.8fs %12.5fms %12.3fus %12.8f FPS\n", iterations, aveDuration, aveDuration * 1000.0, aveDuration * 1000000.0, 1.0 / (aveDuration.count() * std::chrono::duration<double>::period::num / std::chrono::duration<double>::period::den));

    cv::imwrite("..\\..\\..\\images\\pano_rect.jpg", img_dst);
}

auto Equi2Rect::show_rectlinear_image() -> void
{
    cv::namedWindow("projected image", cv::WINDOW_AUTOSIZE);
    cv::imshow("projected image", img_dst);
    cv::waitKey(0);
    cv::destroyWindow("projected image");
}

auto Equi2Rect::eul2rotm(double rotx, double roty, double rotz) -> cv::Mat
{

    cv::Mat R_x = (cv::Mat_<double>(3, 3) << 1, 0, 0,
                   0, cos(rotx), -sin(rotx),
                   0, sin(rotx), cos(rotx));

    cv::Mat R_y = (cv::Mat_<double>(3, 3) << cos(roty), 0, sin(roty),
                   0, 1, 0,
                   -sin(roty), 0, cos(roty));

    cv::Mat R_z = (cv::Mat_<double>(3, 3) << cos(rotz), -sin(rotz), 0,
                   sin(rotz), cos(rotz), 0,
                   0, 0, 1);

    cv::Mat R = R_z * R_y * R_x;

    return R;
}

auto Equi2Rect::reprojection(int x_img, int y_img) -> cv::Vec2d
{
    cv::Mat xyz = (cv::Mat_<double>(3, 1) << (double)x_img, (double)y_img, 1);
#ifdef CACHE_ROT_KINV
    cv::Mat ray3d = RotKinv * xyz / norm(xyz);
#else
    cv::Mat ray3d = Rot * K.inv() * xyz / norm(xyz);
#endif
    // get 3d spherical coordinates
    double xp = ray3d.at<double>(0);
    double yp = ray3d.at<double>(1);
    double zp = ray3d.at<double>(2);
    // inverse formula for spherical projection, reference Szeliski book "Computer Vision: Algorithms and Applications" p439.
    double theta = atan2(yp, sqrt(xp * xp + zp * zp));
    double phi = atan2(xp, zp);

    // get 2D point on equirectangular map
    double x_sphere = (((phi * img_src.cols) / M_PI + img_src.cols) / 2);
    double y_sphere = (theta + M_PI / 2) * img_src.rows / M_PI;

    return cv::Vec2d(x_sphere, y_sphere);
}

auto Equi2Rect::bilinear_interpolation() -> void
{
    cv::Vec2d current_pos;
    // variables for bilinear interpolation
    int top_left_x, top_left_y;
    double dx, dy, wtl, wtr, wbl, wbr;
    cv::Vec3b value, bgr;

    // loop over every pixel in output rectlinear image
    for (int v = 0; v < viewport.height; ++v)
    {
        for (int u = 0; u < viewport.width; ++u)
        {

            // determine corresponding position in the equirectangular panorama
            current_pos = reprojection(u, v);

            // determine the nearest top left pixel for bilinear interpolation
            top_left_x = static_cast<int>(current_pos[0]); // convert the subpixel value to a proper pixel value (top left pixel due to int() operator)
            top_left_y = static_cast<int>(current_pos[1]);

            // if the current position exceeeds the panorama image limit -- leave pixel black and skip to next iteration
            if (current_pos[0] < 0 || top_left_x > img_src.cols - 1 || current_pos[1] < 0 || top_left_y > img_src.rows - 1)
            {
                continue;
            }

            // initialize weights for bilinear interpolation
            dx = current_pos[0] - top_left_x;
            dy = current_pos[1] - top_left_y;
            wtl = (1.0 - dx) * (1.0 - dy);
            wtr = dx * (1.0 - dy);
            wbl = (1.0 - dx) * dy;
            wbr = dx * dy;

            // determine subpixel value with bilinear interpolation
            bgr = wtl * img_src.at<cv::Vec3b>(top_left_y, top_left_x) + wtr * img_src.at<cv::Vec3b>(top_left_y, top_left_x + 1) +
                  wbl * img_src.at<cv::Vec3b>(top_left_y + 1, top_left_x) + wbr * img_src.at<cv::Vec3b>(top_left_y + 1, top_left_x + 1);

            // paint the pixel in the output image with the calculated value
            img_dst.at<cv::Vec3b>(cv::Point(u, v)) = bgr;
        }
    }
    return;
}