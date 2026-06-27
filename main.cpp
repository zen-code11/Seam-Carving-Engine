#include <iostream>
#include <string>
#include <cstdlib>
#include "ImageHandler.hpp"
#include "EnergyMap.hpp"
#include "SeamSolver.hpp"

int main(int argc, char* argv[]) {
    // Expecting: executable_name <input_path> <output_path> <pixels_to_remove>
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0] << " <input_image_path> <output_image_path> <pixels_to_remove>\n";
        std::cerr << "Example: " << argv[0] << " input.jpg output_resized.jpg 150\n";
        return 1;
    }

    std::string inputPath  = argv[1];
    std::string outputPath = argv[2];
    int pixelsToRemove     = std::atoi(argv[3]);

    if (pixelsToRemove < 0) {
        std::cerr << "Error: The number of pixels to remove must be a positive integer.\n";
        return 1;
    }

    try {
        std::cout << "[1/3] Loading image asset via Image Handler...\n";
        ImageGrid image = ImageHandler::loadImage(inputPath);
        
        int originalHeight = image.size();
        int originalWidth  = image[0].size();
        
        if (pixelsToRemove >= originalWidth) {
            std::cerr << "Error: Target truncation count (" << pixelsToRemove 
                      << ") is equal to or greater than total image width (" << originalWidth << ").\n";
            return 1;
        }

        std::cout << "      Original Dimensions: " << originalWidth << "x" << originalHeight << "\n";
        std::cout << "[2/3] Initiating Dynamic Programming Seam Carving Engine...\n";

        // Iteratively calculate energy grids and carve seams out single file slices
        for (int i = 0; i < pixelsToRemove; ++i) {
            // 1. Map the contrast profile of the mutated matrix
            EnergyGrid energy = EnergyMap::calculateEnergy(image);
            
            // 2. Compute the cumulative DP matrix and backtrack the lowest path
            std::vector<int> seam = SeamSolver::findVerticalSeam(energy);
            
            // 3. Mutate the memory grid to isolate and strip the column slice
            image = SeamSolver::removeVerticalSeam(image, seam);

            // Log milestone markers for visibility during heavy image crops
            if ((i + 1) % 50 == 0 || (i + 1) == pixelsToRemove) {
                std::cout << "      Carved " << (i + 1) << " / " << pixelsToRemove << " vertical seams.\n";
            }
        }

        std::cout << "[3/3] Serializing resized frame grid to disk...\n";
        std::cout << "      New Dimensions: " << image[0].size() << "x" << originalHeight << "\n";
        ImageHandler::saveImage(image, outputPath);

        std::cout << "🎉 Success! Content-aware image scaling completed cleanly.\n";

    } catch (const std::exception& ex) {
        std::cerr << "\nFATAL ENGINE EXCEPTION: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}