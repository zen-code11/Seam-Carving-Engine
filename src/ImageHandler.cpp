#include "ImageHandler.hpp"
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <stdexcept>

namespace ImageHandler
{

    ImageGrid loadImage(const std::string &filePath)
    {
        // Load the image in full color (OpenCV loads this as BGR format by default)
        cv::Mat mat = cv::imread(filePath, cv::IMREAD_COLOR);

        if (mat.empty())
        {
            throw std::runtime_error("Error: Could not open or find the image at " + filePath);
        }

        int H = mat.rows;
        int W = mat.cols;

        // Instantiate our native ImageGrid with matching dimensions
        ImageGrid grid(H, std::vector<Pixel>(W));

        // Marshal bytes from cv::Mat into our standard vectors
        for (int i = 0; i < H; ++i)
        {
            for (int j = 0; j < W; ++j)
            {
                cv::Vec3b bgrPixel = mat.at<cv::Vec3b>(i, j);

                // Map BGR channels correctly to our RGB Pixel layout
                grid[i][j].r = bgrPixel[2];
                grid[i][j].g = bgrPixel[1];
                grid[i][j].b = bgrPixel[0];
            }
        }

        return grid;
    }

    void saveImage(const ImageGrid &grid, const std::string &filePath)
    {
        if (grid.empty() || grid[0].empty())
        {
            throw std::invalid_argument("Error: Cannot save an empty image grid.");
        }

        int H = grid.size();
        int W = grid[0].size();

        // Create an empty OpenCV matrix structure with 8-bit unsigned 3-channel layout
        cv::Mat mat(H, W, CV_8UC3);

        // Repopulate the cv::Mat structure row by row
        for (int i = 0; i < H; ++i)
        {
            for (int j = 0; j < W; ++j)
            {
                cv::Vec3b &bgrPixel = mat.at<cv::Vec3b>(i, j);

                // Convert back from RGB to OpenCV's expected BGR order
                bgrPixel[2] = grid[i][j].r;
                bgrPixel[1] = grid[i][j].g;
                bgrPixel[0] = grid[i][j].b;
            }
        }

        // Serialize the matrix back into a compressed disk file (PNG/JPG)
        if (!cv::imwrite(filePath, mat))
        {
            throw std::runtime_error("Error: Failed to write image asset to " + filePath);
        }
    }
}