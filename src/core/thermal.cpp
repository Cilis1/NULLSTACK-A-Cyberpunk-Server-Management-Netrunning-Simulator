#include "core/thermal.hpp"
#include <algorithm>

namespace ns {
int nextTemperature(int current, int servers, int cooling, bool hosting) {
    constexpr int AMBIENT = 24;
    const int target = hosting ? std::max(AMBIENT, AMBIENT + servers * 9 - cooling * 6) : AMBIENT;
    return current + std::clamp(target - current, -5, 5);
}
int thermalThroughput(int temperature) {
    return std::clamp(100 - std::max(0, temperature - 45) * 5, 50, 100);
}
} // namespace ns
