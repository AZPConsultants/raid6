#pragma once
#include <string>

namespace raid6
{
std::string thousand_separated (double x, int precision = 2);

std::string thousand_separated_prop (double x);

std::string thousand_separated (unsigned long long x);

std::string thousand_separated (long long x);
}
