#include "random.hpp"


static std::random_device gRandomDevice{};
static std::mt19937 mt(gRandomDevice());

int
RandomInt(const int inFirst, const int inLast)
{
  std::uniform_int_distribution<int> distribution(inFirst, inLast);
  return distribution(mt);
}

vector<int>
RandomRange(int size, int range)
{
  unordered_map<int, int> hash;
	vector<int> selected;

  for (int i = 0; i < range; ++i) {
    int r = RandomInt(0, size - i);
    int n = hash.count(r) ? hash[r] : r;

    if (r != size - i) {
      if (hash.count(size - i))
        hash[r] = hash[size - i];
      else
        hash[r] = size - i;
    }

    selected.push_back(n);
  }

  return selected;
}