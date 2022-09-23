/**
 * Copyright (c) 2022- Wyther Yang (https://github.com/wythers/iocoro)
 */

#pragma once

#include <random>
#include <unordered_map>
#include <vector>

using std::unordered_map;
using std::vector;

namespace ioCoro {

int
RandomInt(const int inFirst, const int inLast);

vector<int>
RandomRange(int size, int range);

} // namespace ioCoro
