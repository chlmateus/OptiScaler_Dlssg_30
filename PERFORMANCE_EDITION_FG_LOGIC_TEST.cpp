#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>

static double sanitize(double delta, double& last, bool& reset)
{
    constexpr double fallback = 16.6666667;
    constexpr double minMs = 0.05;
    constexpr double maxMs = 100.0;
    constexpr double severeMs = 250.0;
    if (!std::isfinite(delta) || delta <= 0.0) {
        delta = last > 0.0 ? last : fallback;
        last = delta;
        reset = true;
        return delta;
    }
    if (delta >= severeMs || (last > minMs && delta > last * 4.0 && (delta - last) > 40.0))
        reset = true;
    delta = std::clamp(delta, minMs, maxMs);
    last = delta;
    return delta;
}

static uint64_t selectCandidate(uint64_t current, uint64_t submitted, uint64_t allowedAhead, bool currentHasInputs)
{
    if (submitted > current)
        submitted = current > 0 ? current - 1 : 0;
    if (current == submitted)
        return 0;
    uint64_t candidate = submitted == 0 ? current : submitted + 1;
    const uint64_t diff = current >= submitted ? current - submitted : 0;
    if ((diff > allowedAhead || submitted == 0) && currentHasInputs)
        candidate = current;
    return candidate;
}

static double baseTarget(double outputTarget, double refresh, uint32_t multiplier)
{
    const double output = refresh > 0.0 ? std::min(outputTarget, refresh) : outputTarget;
    return output / std::max(1u, multiplier);
}

int main()
{
    // Unsigned rollback must not wrap into a giant positive distance.
    assert(selectCandidate(100, 105, 2, true) == 100);
    assert(selectCandidate(105, 100, 2, true) == 105);
    assert(selectCandidate(101, 100, 2, false) == 101);

    double last = 0.0;
    bool reset = false;
    assert(std::abs(sanitize(0.0, last, reset) - 16.6666667) < 1e-6 && reset);
    reset = false;
    assert(std::abs(sanitize(12.5, last, reset) - 12.5) < 1e-6 && !reset);
    reset = false;
    const double severe = sanitize(300.0, last, reset);
    assert(reset && std::abs(severe - 100.0) < 1e-9);

    assert(std::abs(baseTarget(143.999, 143.999, 2) - 71.9995) < 1e-6);
    assert(std::abs(baseTarget(143.999, 143.999, 3) - (143.999 / 3.0)) < 1e-6);
    assert(std::abs(baseTarget(200.0, 143.999, 2) - 71.9995) < 1e-6);

    std::cout << "Performance Edition FG logic tests passed\n";
}
