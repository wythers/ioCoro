#pragma once

#include <random>

static inline std::random_device gRandomDevice;
static inline std::mt19937 mt(gRandomDevice());

inline int
RandomInt(int left, int right)
{
  return std::uniform_int_distribution<int>{ left, right }(mt);
}
