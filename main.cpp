// ═════════════════════════════════════════════════════════════════════════════
//  Seam Carving Engine  —  main.cpp
//  Content-Aware Image Resizing via Dynamic Programming
//
//  Pipeline:
//    1. Load image  →  2. (Optional) Visualize first seam
//    3. Carve N vertical seams  →  4. Carve M horizontal seams
//    5. Save output  →  6. (Optional) Print benchmark report
//
//  CLI:
//    seam_carver <input> <output> --vcrop N [--hcrop M]
//                [--energy dual_gradient|laplacian]
//                [--visualize <path>] [--benchmark] [--help]
// ═════════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <chrono>
#include <stdexcept>
#include <cstdlib>

#include "Common.hpp"
#include "ImageHandler.hpp"
#include "EnergyMap.hpp"
#include "SeamSolver.hpp"

// ─────────────────────────────────────────────────────────────────────────────
//  Timer Utilities
// ─────────────────────────────────────────────────────────────────────────────

using Clock     = std::chrono::high_resolution_clock;
using TimePoint = std::chrono::time_point<Clock>;

static double elapsedMs(const TimePoint& start) {
    return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
}

static std::string fmtMs(double ms) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << ms << " ms";
    return oss.str();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Help Text
// ─────────────────────────────────────────────────────────────────────────────

static void printHelp(const char* exe) {
    std::cout
        << "\n"
        << "╔══════════════════════════════════════════════════════════════╗\n"
        << "║       Seam Carving Engine  —  Content-Aware Image Resize     ║\n"
        << "╚══════════════════════════════════════════════════════════════╝\n\n"
        << "Usage:\n"
        << "  " << exe << " <input_image> <output_image> [options]\n\n"
        << "Core Options:\n"
        << "  --vcrop  <N>         Remove N vertical seams (shrinks width by N)\n"
        << "  --hcrop  <M>         Remove M horizontal seams (shrinks height by M)\n\n"
        << "Advanced Options:\n"
        << "  --energy <type>      Energy function:\n"
        << "                         dual_gradient  (default) — dual-axis Sobel gradient\n"
        << "                         laplacian               — 5-point Laplacian of luminance\n"
        << "  --visualize <path>   Save a seam-overlay image to <path>\n"
        << "  --benchmark          Print detailed per-phase timing and throughput\n"
        << "  --help               Show this help message\n\n"
        << "Examples:\n"
        << "  " << exe << " photo.jpg out.jpg --vcrop 200\n"
        << "  " << exe << " photo.jpg out.jpg --vcrop 100 --hcrop 50 --benchmark\n"
        << "  " << exe << " photo.jpg out.jpg --vcrop 150 --energy laplacian --visualize seams.jpg\n\n";
}

// ─────────────────────────────────────────────────────────────────────────────
//  Progress Bar
// ─────────────────────────────────────────────────────────────────────────────

static void printProgress(const char* label, int done, int total, double ms) {
    constexpr int BAR = 28;
    const double  pct    = (total > 0) ? static_cast<double>(done) / total : 1.0;
    const int     filled = static_cast<int>(pct * BAR);

    std::cout << "  " << label << " [";
    for (int k = 0; k < BAR; ++k) std::cout << (k < filled ? '\xe2' : ' ');
    // Use ASCII block char on platforms that may not support UTF-8 box chars
    std::cout.flush();

    // Rewrite with simple ASCII bar for maximum compatibility
    std::cout.seekp(0, std::ios::cur);   // no-op, just keep flowing
    std::cout << "\r  " << label << " [";
    for (int k = 0; k < BAR; ++k) std::cout << (k < filled ? '#' : '.');
    std::cout << "] " << std::setw(4) << done << "/" << total
              << "  " << std::fixed << std::setprecision(1) << ms << " ms  \r"
              << std::flush;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Per-Phase Statistics
// ─────────────────────────────────────────────────────────────────────────────

struct PhaseStats {
    double energyMs  = 0.0;
    double findMs    = 0.0;
    double removeMs  = 0.0;
    double totalMs   = 0.0;
    int    seams     = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
//  Core Seam-Carving Loop
// ─────────────────────────────────────────────────────────────────────────────

static PhaseStats carveSeams(ImageGrid&     image,
                              int            count,
                              SeamDirection  dir,
                              EnergyFunction fn) {
    PhaseStats s;
    if (count <= 0) return s;

    const char* label = (dir == SeamDirection::VERTICAL) ? "V-Seams" : "H-Seams";
    const auto  loopStart = Clock::now();

    for (int i = 0; i < count; ++i) {

        // ── Phase A: Build energy map ────────────────────────────────────────
        auto t0 = Clock::now();
        EnergyGrid energy = EnergyMap::calculateEnergy(image, fn);
        s.energyMs += elapsedMs(t0);

        // ── Phase B: DP seam finding + backtracking ──────────────────────────
        auto t1 = Clock::now();
        std::vector<int> seam = (dir == SeamDirection::VERTICAL)
                                ? SeamSolver::findVerticalSeam(energy)
                                : SeamSolver::findHorizontalSeam(energy);
        s.findMs += elapsedMs(t1);

        // ── Phase C: In-place seam removal ───────────────────────────────────
        auto t2 = Clock::now();
        if (dir == SeamDirection::VERTICAL)
            SeamSolver::removeVerticalSeam(image, seam);
        else
            SeamSolver::removeHorizontalSeam(image, seam);
        s.removeMs += elapsedMs(t2);

        ++s.seams;

        // Update progress bar every 10 seams and at completion
        if ((i + 1) % 10 == 0 || (i + 1) == count) {
            printProgress(label, i + 1, count, elapsedMs(loopStart));
        }
    }
    std::cout << "\n";   // newline after the in-place progress bar

    s.totalMs = elapsedMs(loopStart);
    return s;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Benchmark Report
// ─────────────────────────────────────────────────────────────────────────────

static void printBenchmark(const PhaseStats& v, const PhaseStats& h,
                            int origW, int origH,
                            double loadMs, double saveMs) {

    const double totalMs   = loadMs + v.totalMs + h.totalMs + saveMs;
    const int    totalSeams = v.seams + h.seams;
    const double mpx        = static_cast<double>(origW) * origH / 1.0e6;

    // Helper lambda to print one table row
    auto row = [&](const std::string& name, double ms) {
        double pct = (totalMs > 0.0) ? ms / totalMs * 100.0 : 0.0;
        std::cout << "  " << std::left  << std::setw(28) << name
                  << std::right << std::setw(9)  << std::fixed << std::setprecision(2) << ms << " ms"
                  << "   " << std::setw(5) << std::fixed << std::setprecision(1) << pct << "%\n";
    };

    std::cout << "\n"
              << "┌───────────────────────────────────────────────────────────┐\n"
              << "│                    ⏱  Benchmark Report                    │\n"
              << "├───────────────────────────────────────────────────────────┤\n";

    row("  Image Load",               loadMs);
    if (v.seams > 0) {
        row("  V-Energy Maps ×" + std::to_string(v.seams),  v.energyMs);
        row("  V-DP Find     ×" + std::to_string(v.seams),  v.findMs);
        row("  V-Removal     ×" + std::to_string(v.seams),  v.removeMs);
    }
    if (h.seams > 0) {
        row("  H-Energy Maps ×" + std::to_string(h.seams),  h.energyMs);
        row("  H-DP Find     ×" + std::to_string(h.seams),  h.findMs);
        row("  H-Removal     ×" + std::to_string(h.seams),  h.removeMs);
    }
    row("  Image Save",               saveMs);

    std::cout << "├───────────────────────────────────────────────────────────┤\n";

    auto stat = [&](const std::string& name, const std::string& val) {
        std::cout << "│  " << std::left << std::setw(22) << name
                  << std::right << std::setw(34) << val << "  │\n";
    };

    stat("Total wall time",   fmtMs(totalMs));
    stat("Image dimensions",  std::to_string(origW) + " × " + std::to_string(origH) + " px  (" +
                              [&]() -> std::string {
                                  std::ostringstream o;
                                  o << std::fixed << std::setprecision(2) << mpx;
                                  return o.str();
                              }() + " MP)");
    stat("Seams carved",      std::to_string(totalSeams));

    if (totalSeams > 0 && (v.totalMs + h.totalMs) > 0) {
        stat("Avg time / seam", fmtMs((v.totalMs + h.totalMs) / totalSeams));
    }
    if ((v.energyMs + h.energyMs) > 0 && mpx > 0) {
        double mpxPerSec = mpx / ((v.energyMs + h.energyMs) / 1000.0);
        std::ostringstream o;
        o << std::fixed << std::setprecision(1) << mpxPerSec << " MP/s";
        stat("Energy throughput", o.str());
    }
    if (totalMs > 0 && totalSeams > 0) {
        std::ostringstream o;
        o << std::fixed << std::setprecision(1) << (totalSeams / (totalMs / 1000.0)) << " seams/s";
        stat("Pipeline throughput", o.str());
    }

    std::cout << "└───────────────────────────────────────────────────────────┘\n";
}

// ─────────────────────────────────────────────────────────────────────────────
//  main
// ─────────────────────────────────────────────────────────────────────────────

int main(int argc, char* argv[]) {

    // Handle bare --help or no-arg invocation
    if (argc < 3) {
        bool wantsHelp = (argc == 2 && std::string(argv[1]) == "--help");
        printHelp(argv[0]);
        return wantsHelp ? 0 : 1;
    }

    const std::string inputPath  = argv[1];
    const std::string outputPath = argv[2];

    // ── Argument Parsing ──────────────────────────────────────────────────────
    CarveConfig cfg;

    for (int i = 3; i < argc; ++i) {
        const std::string arg = argv[i];

        if (arg == "--help") {
            printHelp(argv[0]);
            return 0;
        } else if (arg == "--vcrop" && i + 1 < argc) {
            cfg.verticalSeams = std::atoi(argv[++i]);
        } else if (arg == "--hcrop" && i + 1 < argc) {
            cfg.horizontalSeams = std::atoi(argv[++i]);
        } else if (arg == "--energy" && i + 1 < argc) {
            const std::string fn = argv[++i];
            if      (fn == "dual_gradient") cfg.energyFn = EnergyFunction::DUAL_GRADIENT;
            else if (fn == "laplacian")     cfg.energyFn = EnergyFunction::LAPLACIAN;
            else {
                std::cerr << "Error: Unknown energy function \"" << fn
                          << "\". Valid options: dual_gradient, laplacian.\n";
                return 1;
            }
        } else if (arg == "--visualize" && i + 1 < argc) {
            cfg.visualize     = true;
            cfg.visualizePath = argv[++i];
        } else if (arg == "--benchmark") {
            cfg.benchmark = true;
        } else {
            std::cerr << "Warning: Unknown argument \"" << arg << "\". Ignoring.\n";
        }
    }

    // Validate seam counts
    if (cfg.verticalSeams < 0 || cfg.horizontalSeams < 0) {
        std::cerr << "Error: --vcrop and --hcrop values must be non-negative.\n";
        return 1;
    }
    if (cfg.verticalSeams == 0 && cfg.horizontalSeams == 0) {
        std::cerr << "Warning: No seams specified. Use --vcrop and/or --hcrop.\n"
                  << "         Output will be a copy of the input.\n";
    }

    // ── Pipeline ──────────────────────────────────────────────────────────────
    try {
        std::cout << "\n"
                  << "══════════════════════════════════════════════════════\n"
                  << "  Seam Carving Engine  |  Content-Aware Image Resize\n"
                  << "══════════════════════════════════════════════════════\n\n";

        // ── Step 1: Load ──────────────────────────────────────────────────────
        std::cout << "[1/4] Loading image...\n";
        auto   tLoad = Clock::now();
        ImageGrid image = ImageHandler::loadImage(inputPath);
        double   loadMs = elapsedMs(tLoad);

        const int origH = static_cast<int>(image.size());
        const int origW = static_cast<int>(image[0].size());

        // Validate seam counts against actual dimensions
        if (cfg.verticalSeams >= origW) {
            std::cerr << "Error: --vcrop " << cfg.verticalSeams
                      << " must be less than image width (" << origW << ").\n";
            return 1;
        }
        if (cfg.horizontalSeams >= origH) {
            std::cerr << "Error: --hcrop " << cfg.horizontalSeams
                      << " must be less than image height (" << origH << ").\n";
            return 1;
        }

        const long long srcBytes = ImageHandler::getFileSize(inputPath);
        std::cout << "  Input  : " << inputPath << "\n";
        std::cout << "  Size   : " << origW << " × " << origH << " px";
        if (srcBytes > 0)
            std::cout << "  |  " << srcBytes / 1024 << " KB on disk";
        std::cout << "\n";
        std::cout << "  Loaded : " << fmtMs(loadMs) << "\n\n";

        // ── Step 2: Seam Visualization (pre-carve) ────────────────────────────
        if (cfg.visualize) {
            std::cout << "[2/4] Generating seam visualization...\n";
            EnergyGrid visEnergy = EnergyMap::calculateEnergy(image, cfg.energyFn);

            ImageGrid overlaid;
            if (cfg.verticalSeams > 0) {
                std::vector<int> visSeam = SeamSolver::findVerticalSeam(visEnergy);
                overlaid = SeamSolver::overlayVerticalSeam(image, visSeam);
            } else {
                std::vector<int> visSeam = SeamSolver::findHorizontalSeam(visEnergy);
                overlaid = SeamSolver::overlayHorizontalSeam(image, visSeam);
            }
            ImageHandler::saveImage(overlaid, cfg.visualizePath);
            std::cout << "  Saved  → " << cfg.visualizePath << "\n\n";
        } else {
            std::cout << "[2/4] Visualization skipped  (use --visualize <path> to enable)\n\n";
        }

        // ── Step 3: Carve Seams ───────────────────────────────────────────────
        const std::string energyName =
            (cfg.energyFn == EnergyFunction::DUAL_GRADIENT) ? "Dual Gradient" : "Laplacian";

        std::cout << "[3/4] Carving seams  [energy: " << energyName << "]\n";
        if (cfg.verticalSeams > 0)
            std::cout << "  Vertical   : " << cfg.verticalSeams
                      << " seams  (width  " << origW << " → " << origW - cfg.verticalSeams << ")\n";
        if (cfg.horizontalSeams > 0)
            std::cout << "  Horizontal : " << cfg.horizontalSeams
                      << " seams  (height " << origH << " → " << origH - cfg.horizontalSeams << ")\n";
        std::cout << "\n";

        PhaseStats vStats = carveSeams(image, cfg.verticalSeams,
                                       SeamDirection::VERTICAL,   cfg.energyFn);
        PhaseStats hStats = carveSeams(image, cfg.horizontalSeams,
                                       SeamDirection::HORIZONTAL, cfg.energyFn);

        // ── Step 4: Save ──────────────────────────────────────────────────────
        std::cout << "\n[4/4] Saving output...\n";
        auto   tSave = Clock::now();
        ImageHandler::saveImage(image, outputPath);
        double saveMs = elapsedMs(tSave);

        const int finalH = static_cast<int>(image.size());
        const int finalW = static_cast<int>(image[0].size());
        const long long dstBytes = ImageHandler::getFileSize(outputPath);

        std::cout << "  Output : " << outputPath << "\n";
        std::cout << "  Size   : " << finalW << " × " << finalH << " px";
        if (dstBytes > 0)
            std::cout << "  |  " << dstBytes / 1024 << " KB on disk";
        std::cout << "\n";
        std::cout << "  Saved  : " << fmtMs(saveMs) << "\n";

        // ── Timing Summary ────────────────────────────────────────────────────
        if (cfg.benchmark) {
            printBenchmark(vStats, hStats, origW, origH, loadMs, saveMs);
        } else {
            const double total = loadMs + vStats.totalMs + hStats.totalMs + saveMs;
            const int    seams = vStats.seams + hStats.seams;
            std::cout << "\n  Completed " << seams << " seam"
                      << (seams != 1 ? "s" : "")
                      << " in " << fmtMs(total) << " total.\n";
            std::cout << "  (Re-run with --benchmark for a full timing breakdown)\n";
        }

        std::cout << "\n  Success! Content-aware resizing complete.\n\n";

    } catch (const std::exception& ex) {
        std::cerr << "\n[FATAL] " << ex.what() << "\n";
        return 1;
    }

    return 0;
}