#pragma once
#include <fmt/format.h>
#include <vector>
#include <algorithm>
#include <boost/iostreams/concepts.hpp> 
#include <boost/iostreams/device/mapped_file.hpp>
namespace io = boost::iostreams;

namespace raid6 {
namespace test {

struct seekable_source_tag : io::device_tag, io::input_seekable { };

class pattern_source 
{
    static const std::streamsize PATTERN_SIZE = 4096;
    static_assert(PATTERN_SIZE % 5 == 1);
public:
	typedef char							char_type;
	typedef seekable_source_tag             category;

	pattern_source ()
	{
		pattern_.reserve (PATTERN_SIZE);
        pattern_.resize (PATTERN_SIZE);
		for (size_t i = 0; i < PATTERN_SIZE-1; i+=5)
		{
			auto s = fmt::format ("{:04X}|", i / 5);
			std::copy (s.begin (), s.end (), pattern_.begin () + i);
		}
		pattern_[PATTERN_SIZE-1] = 'X';
	}

    io::stream_offset seek (io::stream_offset off, std::ios_base::seekdir way)
    {
        using namespace std;

        // Determine new value of pos_
        io::stream_offset next;
        if (way == ios_base::beg) {
            next = off;
        }
        else if (way == ios_base::cur) {
            next = pos_ + off;
        }
        /*else if (way == ios_base::end) {
            next = end_ - off - 1;
        }*/
        else {
            throw ios_base::failure ("bad seek direction");
        }

        pos_ = next;
        return pos_;
    }

    void next_eof (std::streamsize size)
    {
        next_eof_ = pos_ + size;
    }

    std::streamsize read (char_type* s, std::streamsize n)
    {
        if (pos_ == next_eof_)
            return -1; // EOF

        std::streamsize returned = 0;
        std::streamsize left = n;
        while (returned < n)
        {
            auto idx = pos_ % PATTERN_SIZE;
            auto eof = std::min (left, next_eof_ - pos_);
            auto l = std::min (eof, PATTERN_SIZE - idx);
            
            std::copy (pattern_.begin () + idx, pattern_.begin () + idx + l, s + returned);
            returned += l;
            left -= l;
            pos_ += l;

            if (eof <= left)
                break;
        }

        return returned;
    }
private:
	std::vector<char>	pattern_;
    std::streamsize     pos_{ 0 };
    std::streamsize     next_eof_{ std::numeric_limits<std::streamsize>::max () };
};
}
}