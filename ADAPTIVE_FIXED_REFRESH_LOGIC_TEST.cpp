#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>

struct CadenceResult
{
    double base = 0.0;
    uint32_t divisor = 0;
};

static CadenceResult FindCadence(double sustainableBaseFps, double baseCeilingFps, double refreshHz,
                                 uint32_t fgMultiplier, double maxBaseFps)
{
    CadenceResult result {};
    if (!std::isfinite(refreshHz) || refreshHz < 20.0 || fgMultiplier == 0)
        return result;

    const double ceiling = std::min(baseCeilingFps, maxBaseFps);
    double slowest = 0.0;
    uint32_t slowestDivisor = 0;
    for (uint32_t divisor = 1; divisor <= 16; ++divisor)
    {
        const double base = refreshHz / (static_cast<double>(divisor) * static_cast<double>(fgMultiplier));
        if (!std::isfinite(base) || base < 10.0 || base > ceiling + 0.001)
            continue;
        slowest = base;
        slowestDivisor = divisor;
        if (base <= sustainableBaseFps + 0.001 && base > result.base)
        {
            result.base = base;
            result.divisor = divisor;
        }
    }
    if (result.base <= 0.0)
    {
        result.base = slowest;
        result.divisor = slowestDivisor;
    }
    return result;
}

static bool Near(double a, double b, double eps = 0.001) { return std::abs(a - b) <= eps; }

int main()
{
    constexpr double hz = 143.999;

    auto nativeFull = FindCadence(200.0, 200.0, hz, 1, 200.0);
    assert(nativeFull.divisor == 1 && Near(nativeFull.base, 143.999));

    auto native68 = FindCadence(68.0, 143.999, hz, 1, 143.999);
    assert(native68.divisor == 3 && Near(native68.base, 47.9996667));

    auto fg2 = FindCadence(90.0, hz / 2.0, hz, 2, hz / 2.0);
    assert(fg2.divisor == 1 && Near(fg2.base, 71.9995));
    assert(Near(fg2.base * 2.0, hz));

    auto fg3 = FindCadence(90.0, hz / 3.0, hz, 3, hz / 3.0);
    assert(fg3.divisor == 1 && Near(fg3.base, 47.9996667));
    assert(Near(fg3.base * 3.0, hz));

    // Conservative 3x -> 2x topology rebase: preserve ~48 base as the ceiling and snap DOWN to a legal 2x cadence.
    auto fallback3to2 = FindCadence(fg3.base, hz / 2.0, hz, 2, hz / 2.0);
    assert(fallback3to2.divisor == 2 && Near(fallback3to2.base, 35.99975));

    const double sustainable68With8PctHeadroom = 68.0 / 1.08;
    assert(Near(sustainable68With8PctHeadroom, 62.962963, 0.0001));
    auto cadenceAtSafe62 = FindCadence(sustainable68With8PctHeadroom, hz, hz, 1, hz);
    assert(cadenceAtSafe62.divisor == 3 && Near(cadenceAtSafe62.base, 47.9996667));

    std::cout << std::fixed << std::setprecision(6);
    std::cout << "143.999 Hz divisors: " << hz << ", " << hz/2.0 << ", " << hz/3.0 << ", " << hz/4.0 << "\n";
    std::cout << "2x FG full-refresh base: " << fg2.base << " FPS\n";
    std::cout << "3x FG full-refresh base: " << fg3.base << " FPS\n";
    std::cout << "3x -> 2x conservative rebase: " << fallback3to2.base << " FPS (output "
              << fallback3to2.base * 2.0 << ")\n";
    std::cout << "68 FPS sustainable with 8% reserve: " << sustainable68With8PctHeadroom << " FPS\n";
    std::cout << "VSync cadence maps that to: " << cadenceAtSafe62.base << " FPS, divisor "
              << cadenceAtSafe62.divisor << "\n";
    std::cout << "Tearing-paced may retain arbitrary target: " << sustainable68With8PctHeadroom << " FPS\n";
    std::cout << "PASS\n";
}
