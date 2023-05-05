#include "Halide.h"
#include <opencv2/opencv.hpp>

using namespace Halide;

// Function to load the input image using OpenCV
Halide::Buffer<uint8_t> load_image(const std::string& filename) {
    cv::Mat input_mat = cv::imread(filename, cv::IMREAD_COLOR);
    Halide::Buffer<uint8_t> input(input_mat.data, input_mat.cols, input_mat.rows, input_mat.channels());
    return input;
}

// Function to brighten the image
Halide::Buffer<uint8_t> brighten(const Halide::Buffer<uint8_t>& input, float brightness) {
    Halide::Func brighter;
    Halide::Var x, y, c;

    Halide::Expr value = input(x, y, c);
    value = Halide::cast<float>(value);
    value = value * brightness;
    value = Halide::min(value, 255.0f);
    value = Halide::cast<uint8_t>(value);

    brighter(x, y, c) = value;

    Halide::Buffer<uint8_t> output(input.width(), input.height(), input.channels());
    brighter.realize(output);
    return output;

}

// function to perform a kxk convoution with stride s
Halide::Buffer<uint8_t> convolution(Halide::Buffer<uint8_t> input_img, int kernel_size, int stride) {
    Halide::Var x, y, c, kx, ky;
    Halide::Func clamped = Halide::BoundaryConditions::repeat_edge(input_img);
    Halide::Func kernel, convolved;
    kernel(kx, ky) = Halide::cast<float>(1) / (kernel_size * kernel_size);

    Halide::RDom r(0, kernel_size, 0, kernel_size);
    Halide::Expr value = clamped(x * stride + r.x - kernel_size / 2,
        y * stride + r.y - kernel_size / 2, c) *
        kernel(r.x, r.y);
    convolved(x, y, c) = Halide::cast<uint8_t>(Halide::sum(value));

    kernel.compute_root();
    convolved.parallel(y).parallel(x);

    // Calculate the output dimensions
    int output_width = (input_img.width() - kernel_size + stride) / stride;
    int output_height = (input_img.height() - kernel_size + stride) / stride;

    // Realize the output buffer
    Halide::Buffer<uint8_t> output_img(output_width, output_height, input_img.channels());
    convolved.realize(output_img);

    return output_img;
}

// function to convert an RGB image to grayscale
Halide::Buffer<uint8_t> RGB2Gray(Halide::Buffer<uint8_t> input_img) {
    // Check if the input image has 3 channels (RGB)
    if (input_img.channels() != 3) {
        throw std::runtime_error("Input image must have 3 channels (RGB)");
    }

    // Define the Halide variables and functions
    Halide::Var x, y, c;
    Halide::Func rgb_to_gray;

    // Define the grayscale conversion formula
    Halide::Expr R = Halide::cast<float>(input_img(x, y, 0));
    Halide::Expr G = Halide::cast<float>(input_img(x, y, 1));
    Halide::Expr B = Halide::cast<float>(input_img(x, y, 2));
    Halide::Expr gray = 0.33f * R + 0.33f * G + 0.33f * B;

    rgb_to_gray(x, y, c) = Halide::cast<uint8_t>(gray);

    // Define the schedule
    Halide::Var xo, yo, xi, yi;
    rgb_to_gray.tile(x, y, xo, yo, xi, yi, 32, 32).vectorize(xi, 8).parallel(yo);

    // Realize the function and generate the output image
    Halide::Buffer<uint8_t> output_img(input_img.width(), input_img.height(), 1);
    rgb_to_gray.realize(output_img);

    return output_img;
}

// Function to blur image
Halide::Buffer<uint8_t> blurImage(Halide::Buffer<uint8_t> input_img) {
    // Check if the input image has 3 channels (RGB)
    if (input_img.channels() != 3) {
        throw std::runtime_error("Input image must have 3 channels (RGB)");
    }

    // Define the Halide variables and functions
    Halide::Var x, y, c, xi, yi;
    Halide::Func blur_x, blur_y, clamped;

    clamped = Halide::BoundaryConditions::repeat_edge(input_img);

    // Define the horizontal blur function
    blur_x(x, y, c) = (clamped(x - 1, y, c) + clamped(x, y, c) + clamped(x + 1, y, c)) / 3;

    // Define the vertical blur function
    blur_y(x, y, c) = (blur_x(x, y - 1, c) + blur_x(x, y, c) + blur_x(x, y + 1, c)) / 3;

    Halide::Func blur("box_blur");
    blur(x, y, c) = Halide::cast<uint8_t>(blur_y(x, y, c));

    // Define the schedule
    blur.tile(x, y, xi, yi, 32, 32).vectorize(xi, 8).parallel(yi);

    // Realize the function and generate the output image
    Halide::Buffer<uint8_t> output_img(input_img.width(), input_img.height(), input_img.channels());
    blur.realize(output_img);

    return output_img;
}

// Function to binarize an image
Halide::Buffer<uint8_t> binarize(Halide::Buffer<uint8_t> input_img, int threshold = 127) {
    // Check if the input image has more than 1 channel (not grayscale)
    if (input_img.channels() > 1) {
        // Convert the Halide buffer to an OpenCV Mat
        cv::Mat input_mat(input_img.height(), input_img.width(), CV_8UC3, input_img.data());

        // Convert the input image to grayscale
        cv::cvtColor(input_mat, input_mat, cv::COLOR_BGR2GRAY);

        // Convert the grayscale OpenCV Mat back to a Halide buffer
        input_img = Halide::Buffer<uint8_t>(input_mat.data, input_mat.cols, input_mat.rows);
    }

    // Define the Halide variables and functions
    Halide::Var x, y, c;
    Halide::Func binarize_func;

    // Define the binarization function
    binarize_func(x, y, c) = Halide::select(input_img(x, y) < threshold, 0, 255);

    // Realize the function and generate the output image
    Halide::Buffer<uint8_t> output_img(input_img.width(), input_img.height(), 1);
    binarize_func.realize(output_img);

    return output_img;
}

// function to flip an image (horizontally if flag = 0 and vertically if flip=1)
Halide::Buffer<uint8_t> flip(Halide::Buffer<uint8_t> input_img, int flag = 0) {
    // Check if the input image has 3 channels (RGB)
    if (input_img.channels() != 3) {
        throw std::runtime_error("Input image must have 3 channels (RGB)");
    }

    // Define the Halide variables and functions
    Halide::Var x, y, c;
    Halide::Func flip_func;

    // Define the flip function based on the flag value
    if (flag == 0) {
        // Flip horizontally
        flip_func(x, y, c) = input_img(input_img.width() - x - 1, y, c);
    }
    else {
        // Flip vertically
        flip_func(x, y, c) = input_img(x, input_img.height() - y - 1, c);
    }

    // Realize the function and generate the output image
    Halide::Buffer<uint8_t> output_img(input_img.width(), input_img.height(), input_img.channels());
    flip_func.realize(output_img);

    return output_img;
}
