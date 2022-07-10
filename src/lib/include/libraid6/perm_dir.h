#pragma once
#include <assert.h>
#include <limits>
#include <filesystem>
namespace fs = std::filesystem;

namespace raid6 {
struct file_record
{
	fs::path path;
	// all values are absolute to the begining of the stream
	std::streamsize	 start{ std::numeric_limits<long long>::max () };
	std::streamsize	 end{ 0 };
	std::streamsize	 pad_end{ 0 };

	bool operator== (const file_record& rhs)
	{
		return std::tie (path, start, end, pad_end) == std::tie (rhs.path, rhs.start, rhs.end, rhs.pad_end);
	}

	bool operator!= (const file_record& rhs)
	{
		return std::tie (path, start, end, pad_end) != std::tie (rhs.path, rhs.start, rhs.end, rhs.pad_end);
	}

	bool empty() const { return path.empty(); }
};

inline std::streamsize round_up_pow2 (std::streamsize numToRound, std::streamsize multiple)
{
	assert (multiple && ((multiple & (multiple - 1)) == 0));
	return (numToRound + multiple - 1) & -multiple;
}

}