#pragma once
#ifndef HALIDE_IMAGE_PROCESSING_H
#define HALIDE_IMAGE_PROCESSING_H

#include "Halide.h"
#include <opencv2/opencv.hpp>

Halide::Buffer<uint8_t> load_image(const std::string& filename);
Halide::Buffer<uint8_t> brighten(const Halide::Buffer<uint8_t>& input, float brightness);
Halide::Buffer<uint8_t> convolution(Halide::Buffer<uint8_t> input_img, int kernel_size, int stride);
Halide::Buffer<uint8_t> RGB2Gray(Halide::Buffer<uint8_t> input_img);
Halide::Buffer<uint8_t> blurImage(Halide::Buffer<uint8_t> input_img);
Halide::Buffer<uint8_t> binarize(Halide::Buffer<uint8_t> input_img, int threshold = 127);
Halide::Buffer<uint8_t> flip(Halide::Buffer<uint8_t> input_img, int flag = 0);

#endif // HALIDE_IMAGE_PROCESSING_H
