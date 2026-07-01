cmake_minimum_required(VERSION 3.16)
project(SeamCarvingEngine VERSION 2.0.0 LANGUAGES CXX)

# ─────────────────────────────────────────────────────────────────────────────
#  C++ Standard
# ─────────────────────────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)   # Enforce ISO C++17, not GNU extensions

# ─────────────────────────────────────────────────────────────────────────────
#  Compiler-Specific Optimizations
#  Applied only to Release builds so Debug stays debuggable.
# ─────────────────────────────────────────────────────────────────────────────
if(MSVC)
    # /O2  : Full speed optimization (subsumes /O1)
    # /Oi  : Enable compiler intrinsics (e.g. fast sqrt via SSE)
    # /GL  : Whole-Program Optimization (cross-TU inlining)
    # /LTCG: Link-Time Code Generation (pairs with /GL)
    add_compile_options($<$<CONFIG:Release>:/O2>)
    add_compile_options($<$<CONFIG:Release>:/Oi>)
    add_compile_options($<$<CONFIG:Release>:/GL>)
    add_link_options($<$<CONFIG:Release>:/LTCG>)
else()
    # GCC / Clang fallback
    add_compile_options($<$<CONFIG:Release>:-O3>)
    add_compile_options($<$<CONFIG:Release>:-march=native>)
endif()

# ─────────────────────────────────────────────────────────────────────────────
#  OpenCV
#  Requires: core (Mat), imgcodecs (imread/imwrite), imgproc (future use)
# ─────────────────────────────────────────────────────────────────────────────
set(OpenCV_DIR "C:/opencv/build")
find_package(OpenCV REQUIRED COMPONENTS core imgcodecs imgproc)
include_directories(${OpenCV_INCLUDE_DIRS})

# ─────────────────────────────────────────────────────────────────────────────
#  Include Paths
# ─────────────────────────────────────────────────────────────────────────────
include_directories(include)

# ─────────────────────────────────────────────────────────────────────────────
#  Engine Sources (explicit list — avoids file(GLOB) anti-pattern where new
#  .cpp files are silently ignored until CMake is re-run)
# ─────────────────────────────────────────────────────────────────────────────
set(ENGINE_SOURCES
    src/EnergyMap.cpp
    src/ImageHandler.cpp
    src/SeamSolver.cpp
)

# ─────────────────────────────────────────────────────────────────────────────
#  Primary Executable
# ─────────────────────────────────────────────────────────────────────────────
add_executable(seam_carver main.cpp ${ENGINE_SOURCES})
target_link_libraries(seam_carver PRIVATE ${OpenCV_LIBS})

# ─────────────────────────────────────────────────────────────────────────────
#  Benchmark Suite  (default ON)
# ─────────────────────────────────────────────────────────────────────────────
option(BUILD_BENCHMARKS "Build the throughput benchmark suite" ON)
if(BUILD_BENCHMARKS)
    add_executable(seam_benchmark tests/benchmark.cpp ${ENGINE_SOURCES})
    target_link_libraries(seam_benchmark PRIVATE ${OpenCV_LIBS})
    target_include_directories(seam_benchmark PRIVATE include)
endif()

# ─────────────────────────────────────────────────────────────────────────────
#  Correctness Test Suite  (default ON)
# ─────────────────────────────────────────────────────────────────────────────
option(BUILD_TESTS "Build the correctness test suite" ON)
if(BUILD_TESTS)
    add_executable(seam_tests tests/test_correctness.cpp ${ENGINE_SOURCES})
    target_link_libraries(seam_tests PRIVATE ${OpenCV_LIBS})
    target_include_directories(seam_tests PRIVATE include)
endif()