#pragma once

#include <limits>
#include <random>

static std::random_device gRandomDevice;
static std::mt19937 mt(gRandomDevice());

inline float
RandDeltaTime()
{
  static std::uniform_real_distribution<float> distribution(0.2f,
                                                            0.3f);
  return distribution(mt);
}

inline int
RandInt()
{
  std::uniform_int_distribution<int> distribution(
    0, std::numeric_limits<int>::max());
  return distribution(mt);
}

inline int
RandInt(const int inFrom, const int inTo)
{
  std::uniform_int_distribution<int> distribution(inFrom, inTo);
  return distribution(mt);
}

inline float
RandFloat()
{
  std::uniform_real_distribution<float> distribution(
    0, std::numeric_limits<float>::max());
  return distribution(mt);
}

inline float
RandFloat(const float inFrom, const float inTo)
{
  std::uniform_real_distribution<float> distribution(inFrom, inTo);
  return distribution(mt);
}
