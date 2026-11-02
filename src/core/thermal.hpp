#pragma once

namespace ns {
/// Approximate rack temperature in Celsius, with one day of thermal inertia per action.
/// This simple aggregate model is not a per-tile CFD simulation.
int nextTemperature(int current, int servers, int cooling, bool hosting);
/// Percentage of nominal hosting throughput after thermal throttling.
int thermalThroughput(int temperature);
} // namespace ns
