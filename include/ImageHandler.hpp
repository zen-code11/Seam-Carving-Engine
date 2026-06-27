#ifndef IMAGE_HANDLER_HPP
#define IMAGE_HANDLER_HPP

#include "Common.hpp"
#include <string>

namespace ImageHandler
{
    // Loads an image file from disk and converts it into a native ImageGrid matrix
    ImageGrid loadImage(const std::string &filePath);

    // Converts a native ImageGrid matrix back into an image file and saves it to disk
    void saveImage(const ImageGrid &grid, const std::string &filePath);
}

#endif // IMAGE_HANDLER_HPP