# High-Performance Content-Aware Image Resizing Engine (Seam Carving)

A production-grade C++ toolchain that dynamically alters image aspect ratios without compressing or distorting critical visual subjects. By leveraging digital image processing gradients and **Dynamic Programming**, the engine maps the visual significance of an image, identifies continuous low-impact pixel pathways (seams), and carves them out in optimal linear time.

---

## 🖼️ Visual Showcase & Contrast Comparison

Use this section to demonstrate the power of content-aware scaling versus traditional scaling. Traditional scaling squishes or stretches objects, while this engine removes insignificant background data.

| Original Image (`input.jpg`) | Resized Image (`output.jpg`) |
| :---: | :---: |
| ![Original Image](./assets/input.jpg) | ![Resized Image](./assets/output.jpg) |
| **Dimensions:** e.g., 3920px Width | **Dimensions:** e.g., 3820px Width (-100 Seams) |

> 💡 **Notice the Contrast:** When you upload your images here, you will observe that high-contrast subjects (like people, buildings, or distinct foreground items) retain their exact structural dimensions, while flat, repetitive background pixels (like skies, empty fields, or blurred walls) are dynamically swallowed up.

---

## 🛠️ Tech Stack & Requirements

* **Language:** C++17 (Utilizing native STL vectors, memory management primitives, and strict encapsulation).
* **Build Configuration Tool:** CMake (Minimum 3.16 required).
* **External Framework Dependency:** OpenCV 4.x (`core` for custom optimized matrix containers and `imgcodecs` for decoding/encoding physical files).
* **Target Operating Platform:** Windows 10/11 x64.
* **Compiler Target:** Microsoft Visual C++ (MSVC via Visual Studio Community Build Tools) running native AMD64 architectures.

---

## 🧠 Core Algorithmic Architecture

The system operates like a mathematical assembly line, collapsing an exponential pathfinding search space into a deterministic linear operation using five core computer vision and data structure concepts:

### 1. Dual-Gradient Finite Difference Engine
Before resizing can occur, the raw color channels must be mapped into an absolute mathematical energy landscape. The algorithm measures localized spatial variance ($\Delta_x$ and $\Delta_y$) across all three independent RGB color channels simultaneously using central differences. 

For a pixel at index $(i, j)$:

$$\Delta_x^2 = (R_{i, j+1} - R_{i, j-1})^2 + (G_{i, j+1} - G_{i, j-1})^2 + (B_{i, j+1} - B_{i, j-1})^2$$

$$\Delta_y^2 = (R_{i+1, j} - R_{i-1, j})^2 + (G_{i+1, j} - G_{i-1, j})^2 + (B_{i+1, j} - B_{i-1, j})^2$$

$$\text{Energy}(i, j) = \sqrt{\Delta_x^2 + \Delta_y^2}$$

This function maps high energy scores to sharp edge frequencies (text, people, objects) while dropping flat spans (sky, ocean, bare floors) to near zero energy, targeting them for destruction.

### 2. Dynamic Programming (DP) — Cumulative Cost Mapping
Finding the absolute lowest-energy pathway from the top row to the bottom row is an intensive optimization challenge. A brute-force tree search would explore every branching option from top to bottom, exploding into an exponential time complexity of $O(3^H)$, completely freezing system performance.

By applying **Dynamic Programming**, the engine breaks the global problem down into overlapping subproblems. It solves each path segment exactly once and caches the local state inside a 2D `CostGrid`, instantly collapsing time complexity to a flat **$O(W \cdot H)$**.

* **State Space:** Let $M(i, j)$ be the minimum total energy required to reach pixel coordinate $(i, j)$ from anywhere on the absolute top boundary row ($i = 0$).
* **Base Case:** The top boundary row requires no accumulation; its path cost is simply its intrinsic gradient energy:
  $$M(0, j) = \text{Energy}(0, j) \quad \text{for all } j \text{ columns}$$
* **Recurrence Relation:** For all subsequent rows $i > 0$, the minimal path energy to reach cell $(i, j)$ is determined by assessing its three adjacent parent cells from the row directly above. The algorithm extracts the absolute minimum from those choices and sums it with the local pixel's energy:
  $$M(i, j) = \text{Energy}(i, j) + \min\big(M(i-1, j-1), M(i-1, j), M(i-1, j+1)\big)$$

### 3. Backtracking — Seam Reconstruction
While the forward DP loop completes our cost table down to the very last row ($H-1$), it does not natively save the coordinates of the route taken. The backtracking phase traces the optimal path in reverse:

1. **The Anchor Point:** The algorithm evaluates the bottom row of the `CostGrid` using a simple **Linear Search** to locate the column index $j^*$ with the smallest cumulative value. This coordinate is the guaranteed base of the minimum energy pathway.
2. **Reverse Traversal:** Moving upward from row $i = H-2$ down to row $0$, the engine examines the three parent paths directly above the current tracking pointer. It updates its pointer greedily to whichever parent cell was responsible for contributing that minimal cost value during the forward DP pass.
3. **Coordinate Log:** The column addresses are logged into a compact 1D vector (`std::vector<int> seam`) of size $H$, where the array index maps directly to the image row.

### 4. Toroidal Array Wrap-Around (Boundary Resolution)
A common issue in matrix convolution is the lack of adjacent elements at outer grid boundaries (row 0 or column 0). Instead of using artificial black padding (which creates synthetic hard edge paths that trap the seam finder), this engine uses a **Toroidal Wrap-Around algorithm**. Using modular conditional logic, it treats the image plane as a geometric torus where the left edge wraps directly to touch the right edge, ensuring seamless, natural gradient profiles across boundaries.

### 5. Linear-Time Matrix Reflow (Memory Optimization)
Calling standard array deletion functions like `std::vector::erase()` inside nested loops forces the operating system to continuously shift sequential memory blocks to fill index gaps. This scales processing overhead to an intensive $O(W^2 \cdot H)$. 
This engine bypasses this limitation by using a custom **Matrix Reflow algorithm**: for each row, it pre-allocates a clean new vector of size $W-1$, linearly copies over the surviving pixels while skipping the seam coordinate entirely, and swaps the internal pointers. This preserves linear, cache-friendly memory performance tracking at **$O(W \cdot H)$**.

---

## 📂 Project Directory Structure

```text
SeamCarvingEngine/
├── CMakeLists.txt                 # Master build configuration file
├── README.md                      # Project documentation and visual logs
├── main.cpp                       # Application entry point & pipeline orchestration
│
├── include/                       # Blueprint Declarations (Header Files)
│   ├── Common.hpp                 # Universal type overrides (Pixel layout, aliases)
│   ├── ImageHandler.hpp           # OpenCV isolation and file I/O layer
│   ├── EnergyMap.hpp              # Discrete pixel derivative boundaries
│   └── SeamSolver.hpp             # DP accumulators & pointer mutation logic
│
└── src/                           # Execution Engine (Source Files)
    ├── ImageHandler.cpp           # Type conversions (cv::Mat to standard vector)
    ├── EnergyMap.cpp              # Dual-gradient matrix mapping
    └── SeamSolver.cpp             # Forward DP loops and pointer swaps