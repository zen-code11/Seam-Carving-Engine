#ifndef IMAGE_HANDLER_HPP
#define IMAGE_HANDLER_HPP

#include "Common.hpp"
#include <string>

// ─────────────────────────────────────────────────────────────────────────────
//  ImageHandler  —  OpenCV I/O Isolation Layer
// ─────────────────────────────────────────────────────────────────────────────
//
//  All direct OpenCV calls are confined here so the rest of the engine is
//  decoupled from the CV library. Internally uses raw row pointers (mat.ptr)
//  rather than mat.at<> to eliminate per-pixel bounds-checking overhead.
//
namespace ImageHandler {

    // Reads an image from disk and converts it into a native ImageGrid.
    // OpenCV loads in BGR order; this function performs the BGR→RGB swap.
    // Throws std::runtime_error if the file cannot be opened or decoded.
    ImageGrid loadImage(const std::string& filePath);

    // Serializes an ImageGrid to disk as a compressed image (JPEG/PNG/BMP,
    // determined by the file extension in filePath).
    // Throws std::invalid_argument on empty grid, std::runtime_error on write failure.
    void saveImage(const ImageGrid& grid, const std::string& filePath);

    // Returns the file size in bytes, or -1 if the path does not exist.
    long long getFileSize(const std::string& filePath);

} // namespace ImageHandler

#endif // IMAGE_HANDLER_HPP