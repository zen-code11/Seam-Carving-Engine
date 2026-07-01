// ═════════════════════════════════════════════════════════════════════════════
//  tests/benchmark.cpp
//  Seam Carving Engine — Throughput Benchmark Suite
//
//  Measures real-world performance across a range of synthetic image sizes.
//  No external benchmark library required — uses std::chrono directly.
//
//  Build:  cmake --build . --config Release
//  Run:    seam_benchmark
//
//  Reported metrics:
//    • Energy map throughput (Megapixels / second)
//    • DP seam-find throughput (seams / second)
//    • In-place removal throughput (seams / second)
//    • Full pipeline throughput (seams / second)
//    • Average milliseconds per seam
// ═════════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <chrono>
#include <sstream>
#include <random>
#include <algorithm>
#include <numeric>

#include "Common.hpp"
#include "EnergyMap.hpp"
#include "SeamSolver.hpp"

// ─────────────────────────────────────────────────────────────────────────────
//  Timer
// ─────────────────────────────────────────────────────────────────────────────

using Clock = std::chrono::high_resolution_clock;

static double elapsedMs(std::chrono::time_point<Clock> start) {
    return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Synthetic Image Factory
//  Creates a pseudo-random colour image so the energy map is non-trivial
//  (a uniform image would produce degenerate seams and unrepresentative timings)
// ─────────────────────────────────────────────────────────────────────────────

static ImageGrid makeSyntheticImage(int width, int height, unsigned seed = 42) {
    ImageGrid img(height, std::vector<Pixel>(width));
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> dist(0, 255);

    for (int i = 0; i < height; ++i) {
        for (int j = 0; j < width; ++j) {
            img[i][j] = {
                static_cast<uint8_t>(dist(rng)),
                static_cast<uint8_t>(dist(rng)),
                static_cast<uint8_t>(dist(rng))
            };
        }
    }
    return img;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Single-Size Benchmark
// ─────────────────────────────────────────────────────────────────────────────

struct BenchResult {
    int    width, height;
    int    seamsRun;
    double energyMs;
    double findMs;
    double removeMs;
    double totalMs;
};

static BenchResult runBenchmark(int width, int height,
                                int seamsToCarve, EnergyFunction fn) {
    ImageGrid image = makeSyntheticImage(width, height);

    double energyTotal = 0.0;
    double findTotal   = 0.0;
    double removeTotal = 0.0;

    auto wallStart = Clock::now();

    for (int s = 0; s < seamsToCarve; ++s) {
        // Energy
        auto t0 = Clock::now();
        EnergyGrid energy = EnergyMap::calculateEnergy(image, fn);
        energyTotal += elapsedMs(t0);

        // Find
        auto t1 = Clock::now();
        std::vector<int> seam = SeamSolver::findVerticalSeam(energy);
        findTotal += elapsedMs(t1);

        // Remove (in-place)
        auto t2 = Clock::now();
        SeamSolver::removeVerticalSeam(image, seam);
        removeTotal += elapsedMs(t2);
    }

    double wallMs = elapsedMs(wallStart);

    return { width, height, seamsToCarve,
             energyTotal, findTotal, removeTotal, wallMs };
}

// ─────────────────────────────────────────────────────────────────────────────
//  Reporting
// ─────────────────────────────────────────────────────────────────────────────

static void printHeader() {
    std::cout
        << "\n"
        << "┌──────────────────────────────────────────────────────────────────────────────────┐\n"
        << "│               Seam Carving Engine  —  Throughput Benchmark                       │\n"
        << "│               (Release build, synthetic random images, vertical seams)            │\n"
        << "├──────────────┬────────┬────────────┬────────────┬────────────┬────────────────────┤\n"
        << "│  Resolution  │ Seams  │  Energy    │  DP Find   │  Removal   │  Pipeline          │\n"
        << "│              │        │  (MP/s)    │  (seams/s) │  (seams/s) │  (ms/seam avg)     │\n"
        << "├──────────────┼────────┼────────────┼────────────┼────────────┼────────────────────┤\n";
}

static void printRow(const BenchResult& r) {
    const double mpx = static_cast<double>(r.width) * r.height / 1.0e6;

    // Energy throughput: total megapixels processed / total energy time in seconds
    double energyMPxs = (r.energyMs > 0.0)
                        ? (mpx * r.seamsRun) / (r.energyMs / 1000.0)
                        : 0.0;

    double findSeamsS  = (r.findMs   > 0.0) ? r.seamsRun / (r.findMs   / 1000.0) : 0.0;
    double removeSeamsS= (r.removeMs > 0.0) ? r.seamsRun / (r.removeMs / 1000.0) : 0.0;
    double avgMsPerSeam= (r.seamsRun > 0)   ? r.totalMs  /  r.seamsRun            : 0.0;

    std::ostringstream res;
    res << r.width << "×" << r.height;

    std::cout
        << "│  " << std::left  << std::setw(12) << res.str()
        << "│  " << std::right << std::setw(5)  << r.seamsRun  << " "
        << "│  " << std::right << std::setw(8)  << std::fixed << std::setprecision(1) << energyMPxs  << "  "
        << "│  " << std::right << std::setw(8)  << std::fixed << std::setprecision(1) << findSeamsS  << "  "
        << "│  " << std::right << std::setw(8)  << std::fixed << std::setprecision(1) << removeSeamsS << "  "
        << "│  " << std::right << std::setw(8)  << std::fixed << std::setprecision(2) << avgMsPerSeam << " ms/seam   "
        << "│\n";
}

// ─────────────────────────────────────────────────────────────────────────────
//  Energy Function Comparison
// ─────────────────────────────────────────────────────────────────────────────

static void energyFunctionComparison() {
    constexpr int W = 1280, H = 720, SEAMS = 20;

    std::cout << "\n"
              << "┌────────────────────────────────────────────────────────┐\n"
              << "│         Energy Function Comparison  (1280×720)          │\n"
              << "├──────────────────────┬────────────────┬─────────────────┤\n"
              << "│  Function            │  Total (ms)    │  Throughput     │\n"
              << "├──────────────────────┼────────────────┼─────────────────┤\n";

    const double mpx = static_cast<double>(W) * H / 1.0e6;

    for (auto fn : { EnergyFunction::DUAL_GRADIENT, EnergyFunction::LAPLACIAN }) {
        ImageGrid img = makeSyntheticImage(W, H, 7);

        auto start = Clock::now();
        for (int s = 0; s < SEAMS; ++s) {
            EnergyGrid e = EnergyMap::calculateEnergy(img, fn);
            (void)e;   // timing only
        }
        double ms = elapsedMs(start);
        double mpxPerSec = (ms > 0.0) ? (mpx * SEAMS) / (ms / 1000.0) : 0.0;

        const char* name = (fn == EnergyFunction::DUAL_GRADIENT) ? "Dual Gradient" : "Laplacian";
        std::cout
            << "│  " << std::left  << std::setw(20) << name
            << "│  " << std::right << std::setw(10) << std::fixed << std::setprecision(2) << ms << " ms  "
            << "│  " << std::right << std::setw(8)  << std::fixed << std::setprecision(1) << mpxPerSec << " MP/s   │\n";
    }

    std::cout << "└──────────────────────┴────────────────┴─────────────────┘\n";
}

// ─────────────────────────────────────────────────────────────────────────────
//  Horizontal vs Vertical Seam Comparison
// ─────────────────────────────────────────────────────────────────────────────

static void directionComparison() {
    constexpr int W = 1280, H = 720, SEAMS = 20;

    std::cout << "\n"
              << "┌────────────────────────────────────────────────────────┐\n"
              << "│       Vertical vs Horizontal Seam Speed  (1280×720)     │\n"
              << "├──────────────────────┬────────────────┬─────────────────┤\n"
              << "│  Direction           │  Total (ms)    │  Throughput     │\n"
              << "├──────────────────────┼────────────────┼─────────────────┤\n";

    auto bench = [&](SeamDirection dir, const char* label) {
        ImageGrid img = makeSyntheticImage(W, H, 13);
        auto start = Clock::now();
        for (int s = 0; s < SEAMS; ++s) {
            EnergyGrid energy = EnergyMap::calculateEnergy(img, EnergyFunction::DUAL_GRADIENT);
            if (dir == SeamDirection::VERTICAL) {
                auto seam = SeamSolver::findVerticalSeam(energy);
                SeamSolver::removeVerticalSeam(img, seam);
            } else {
                auto seam = SeamSolver::findHorizontalSeam(energy);
                SeamSolver::removeHorizontalSeam(img, seam);
            }
        }
        double ms = elapsedMs(start);
        double throughput = (ms > 0.0) ? SEAMS / (ms / 1000.0) : 0.0;
        std::cout
            << "│  " << std::left  << std::setw(20) << label
            << "│  " << std::right << std::setw(10) << std::fixed << std::setprecision(2) << ms << " ms  "
            << "│  " << std::right << std::setw(8)  << std::fixed << std::setprecision(1) << throughput << " seams/s │\n";
    };

    bench(SeamDirection::VERTICAL,   "Vertical");
    bench(SeamDirection::HORIZONTAL, "Horizontal");
    std::cout << "└──────────────────────┴────────────────┴─────────────────┘\n";
}

// ─────────────────────────────────────────────────────────────────────────────
//  main
// ─────────────────────────────────────────────────────────────────────────────

int main() {
    std::cout << "\n  Building synthetic test images and warming up...\n";

    // Warm-up pass (avoids cold-cache bias in the first measurement)
    {
        ImageGrid warm = makeSyntheticImage(512, 512);
        for (int s = 0; s < 5; ++s) {
            auto energy = EnergyMap::calculateEnergy(warm, EnergyFunction::DUAL_GRADIENT);
            auto seam   = SeamSolver::findVerticalSeam(energy);
            SeamSolver::removeVerticalSeam(warm, seam);
        }
    }

    // ── Main throughput table ─────────────────────────────────────────────────
    printHeader();

    // Test cases: (width, height, seams_to_carve)
    const std::vector<std::tuple<int,int,int>> cases = {
        {  512,  512,  50 },
        { 1280,  720,  50 },
        { 1920, 1080,  30 },
        { 2560, 1440,  20 },
        { 3840, 2160,  10 },
    };

    for (auto& [w, h, seams] : cases) {
        BenchResult r = runBenchmark(w, h, seams, EnergyFunction::DUAL_GRADIENT);
        printRow(r);
    }

    std::cout
        << "└──────────────┴────────────┴────────────┴────────────┴────────────────────┘\n";

    // ── Energy function comparison ────────────────────────────────────────────
    energyFunctionComparison();

    // ── Direction comparison ──────────────────────────────────────────────────
    directionComparison();

    std::cout << "\n  Benchmark complete. Paste these numbers into your README.\n\n";
    return 0;
}
