#include <cmath>
#include <tuple>

namespace hades::detail
{
    // NOTE: this becomes constexpr in cpp26 due to std::sin/cos
    constexpr std::tuple<float, float> random_gradient(const int ix, const int iy) noexcept {
        // No precomputed gradients mean this works for any number of grid coordinates
        const unsigned w = 8 * sizeof(unsigned);
        const unsigned s = w / 2; // rotation width
        unsigned a = ix, b = iy;
        a *= 3284157443; b ^= a << s | a >> (w - s);
        b *= 1911520717; a ^= b << s | b >> (w - s);
        a *= 2048419325;
        const auto random = static_cast<float>(a * (3.14159265 / ~(~0u >> 1))); // in [0, 2*Pi]
        return { std::cos(random), std::sin(random) };
    }

    // Computes the dot product of the distance and gradient vectors.
    inline float dot_grid_gradient(const int ix, const int iy, const float x, const float y) noexcept {
        // Get gradient from integer coordinates
        const auto [g_x, g_y] = random_gradient(ix, iy);

        // Compute the distance vector
        const auto dx = x - static_cast<float>(ix);
        const auto dy = y - static_cast<float>(iy);

        // Compute the dot-product
        return (dx * g_x + dy * g_y);
    }
}

namespace hades
{
    // this is the wikipedia implementation of perlin (including above functions)
    // origional wikipedia comments are preserved
    [[nodiscard]]
    inline float perlin(const float x, const float y) noexcept
    {
        // Determine grid cell coordinates
        const auto x0 = static_cast<int>(std::floor(x));
        const auto x1 = x0 + 1;
        const auto y0 = static_cast<int>(std::floor(y));
        const auto y1 = y0 + 1;

        // Determine interpolation weights
        // Could also use higher order polynomial/s-curve here
        const auto sx = x - static_cast<float>(x0);
        const auto sy = y - static_cast<float>(y0);

        // Interpolate between grid point gradients
        auto n0 = detail::dot_grid_gradient(x0, y0, x, y);
        auto n1 = detail::dot_grid_gradient(x1, y0, x, y);
        const auto ix0 = std::lerp(n0, n1, sx);

        n0 = detail::dot_grid_gradient(x0, y1, x, y);
        n1 = detail::dot_grid_gradient(x1, y1, x, y);
        const auto ix1 = std::lerp(n0, n1, sx);

        return std::lerp(ix0, ix1, sy); // Will return in range -1 to 1. To make it in range 0 to 1, multiply by 0.5 and add 0.5
    }
}
