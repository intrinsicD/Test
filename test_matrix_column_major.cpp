#include "engine/math/include/engine/math/matrix.hpp"
#include "engine/math/include/engine/math/vector.hpp"
#include <iostream>
#include <iomanip>

int main() {
    using namespace engine::math;

    // Create a simple 3x3 matrix with known values
    // In row-major notation:
    // [1, 2, 3]
    // [4, 5, 6]
    // [7, 8, 9]
    Matrix<float, 3, 3> m{1, 2, 3, 4, 5, 6, 7, 8, 9};

    std::cout << "Matrix created with values 1-9 in row-major order:\n";
    std::cout << "Expected (if column-major is correct):\n";
    std::cout << "[1, 2, 3]\n[4, 5, 6]\n[7, 8, 9]\n\n";

    std::cout << "Actual values using [row][col] accessor:\n";
    for (size_t r = 0; r < 3; ++r) {
        std::cout << "[";
        for (size_t c = 0; c < 3; ++c) {
            std::cout << m[r][c];
            if (c < 2) std::cout << ", ";
        }
        std::cout << "]\n";
    }

    std::cout << "\nColumn storage (columns array):\n";
    for (size_t c = 0; c < 3; ++c) {
        std::cout << "Column " << c << ": [";
        for (size_t r = 0; r < 3; ++r) {
            std::cout << m.columns[c][r];
            if (r < 2) std::cout << ", ";
        }
        std::cout << "]\n";
    }

    std::cout << "\nMemory layout via data():\n";
    const float* data = m.data();
    std::cout << "[";
    for (size_t i = 0; i < 9; ++i) {
        std::cout << data[i];
        if (i < 8) std::cout << ", ";
    }
    std::cout << "]\n";

    // Test translation matrix
    std::cout << "\n\n=== Translation Matrix Test ===\n";
    vec3 offset{10.0f, 20.0f, 30.0f};
    Matrix<float, 4, 4> trans = translation(offset);

    std::cout << "Translation matrix with offset (10, 20, 30):\n";
    for (size_t r = 0; r < 4; ++r) {
        std::cout << "[";
        for (size_t c = 0; c < 4; ++c) {
            std::cout << std::setw(8) << trans[r][c];
            if (c < 3) std::cout << ", ";
        }
        std::cout << "]\n";
    }

    std::cout << "\nColumn 3 (should be translation):\n";
    std::cout << "[" << trans.columns[3][0] << ", "
              << trans.columns[3][1] << ", "
              << trans.columns[3][2] << ", "
              << trans.columns[3][3] << "]\n";

    // Test matrix-vector multiplication
    std::cout << "\n\n=== Matrix-Vector Multiplication Test ===\n";
    vec4 point{1.0f, 2.0f, 3.0f, 1.0f};
    vec4 result = trans * point;

    std::cout << "Point: [" << point[0] << ", " << point[1] << ", " << point[2] << ", " << point[3] << "]\n";
    std::cout << "Result: [" << result[0] << ", " << result[1] << ", " << result[2] << ", " << result[3] << "]\n";
    std::cout << "Expected: [11, 22, 33, 1] (original + translation)\n";

    bool correct = (result[0] == 11.0f && result[1] == 22.0f && result[2] == 33.0f && result[3] == 1.0f);
    std::cout << "\nTest " << (correct ? "PASSED" : "FAILED") << "\n";

    return correct ? 0 : 1;
}

