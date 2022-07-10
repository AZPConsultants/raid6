#pragma once
#include <array>

namespace raid6 {
	const size_t BLOCK_SIZE = 4096;
	using buff_t = std::array<char, BLOCK_SIZE>;
}