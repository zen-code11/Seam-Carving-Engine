cmake_minimum_required(VERSION 3.16)
project(SeamCarvingEngine CXX)

# Force modern C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable aggressive optimization for intensive matrix math
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

# Locate OpenCV library components
set(OpenCV_DIR "C:/opencv/build")
find_package(OpenCV REQUIRED COMPONENTS core imgcodecs)
include_directories(${OpenCV_INCLUDE_DIRS})

# Include our custom headers directory
include_directories(include)

# Gather all source files
file(GLOB SOURCES "src/*.cpp" "main.cpp")

# Define the final executable output
add_executable(seam_carver ${SOURCES})

# Link OpenCV binaries to our target executable
target_link_libraries(seam_carver ${OpenCV_LIBS})