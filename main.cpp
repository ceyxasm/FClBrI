#include "Halide.h"
#include <opencv2/opencv.hpp>
#include "FClBrI.h"
using namespace Halide;

void display_image(cv::Mat image) 
{
    cv::namedWindow("Image", cv::WINDOW_NORMAL);
    cv::imshow("Image", image);
    cv::waitKey(0);
    cv::destroyAllWindows();
}
int main(int argc, char** argv) {

    // Create a Halide buffer from the input image
    Halide::Buffer<uint8_t> input = load_image("images/rgb.png");

    // Transform the image using the Halide pipeline
    Halide::Buffer<uint8_t> output = brighten(input, 0.5);

    // Try different functions as above

    // Save the output image using OpenCV
    cv::Mat output_mat(output.height(), output.width(), CV_8UC3, output.data());
    display_image(output_mat);
    printf("Success!\n");
    return 0;
}
