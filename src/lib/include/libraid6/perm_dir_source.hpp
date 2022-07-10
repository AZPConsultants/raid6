#pragma once
#include "perm_dir.h"
#include <string>
#include <boost/iostreams/concepts.hpp> 
#include <boost/iostreams/device/mapped_file.hpp>
namespace io = boost::iostreams;

namespace raid6 {

struct seekable_source_tag : io::device_tag, io::input_seekable { };

template <class PermDirProvider, typename CharType=char>
class perm_dir_source
{
	static_assert(sizeof (CharType) == 1);
public:
	typedef typename CharType				char_type;
	typedef seekable_source_tag             category;

    perm_dir_source ()
    {}

	perm_dir_source (std::string_view db_name, std::string id, fs::path const& root)
	{
        open (db_name, id, root);
    }

    ~perm_dir_source ()
    {
        if (curr_file_.is_open ())
            curr_file_.close ();
    }

    void open (std::string_view db_name, std::string id, fs::path const& root)
    {
        assert (!open_);

        perm_dir_.open (db_name, id, root);
        end_ = perm_dir_.size ();
        pad_end_ = end_;
        open_ = true;
    }

    PermDirProvider const& provider () const
    {
        //assert (open_);
        
        return perm_dir_;
    }

    io::stream_offset seek (io::stream_offset off, std::ios_base::seekdir way)
    {
        using namespace std;

        assert (open_);

        // Determine new value of pos_
        io::stream_offset next;
        if (way == ios_base::beg) {
            next = off;
        }
        else if (way == ios_base::cur) {
            next = pos_ + off;
        }
        else if (way == ios_base::end) {
            next = pad_end_ - off - 1;
        }
        else {
            throw ios_base::failure ("bad seek direction");
        }

        // Check for errors
        if (next < 0 || next >= static_cast<io::stream_offset>(pad_end_))
            throw ios_base::failure ("bad seek offset");

        pos_ = next;
        return pos_;
    }

	std::streamsize read (char_type* s, std::streamsize n)
	{
        assert (open_);

        if (!curr_file_.is_open ())
            open_next ();
        std::streamsize returned = 0;
        std::streamsize left = n;
        while (returned < n)
        {
            if (pos_ < curr_file_rec_.end)
            {
                auto pos_in_curr = pos_ - curr_file_rec_.start;
                auto l = std::min(left, curr_file_rec_.end - pos_);
                std::memcpy (s + returned, curr_file_.data () + pos_in_curr, l);
                returned += l;
                left -= l;
                pos_ += l;
            }
            else if (pos_ >= curr_file_rec_.end && pos_ < curr_file_rec_.pad_end)
            {
                auto l = std::min (left, curr_file_rec_.pad_end - pos_);
                std::memset (s + returned, 0, l);
                returned += l;
                left -= l;
                pos_ += l;
            }
            else if (pos_ >= curr_file_rec_.pad_end)
            {
                curr_file_.close ();
                if (pos_ < end_)
                    open_next ();
                else
                    return -1;
            }
        }

		return returned;
	}

    void pad_end_until (std::streamsize max_offset)
    {
        // assert (open_);

        if (max_offset >= end_)
            pad_end_ = max_offset;
        else
            throw std::ios_base::failure ("bad pad end offset");
    }

    file_record const& curr_file () const
    {
        return curr_file_rec_;
    }
private:
    void open_next ()
    {
        curr_file_rec_ = perm_dir_.at (pos_);
        if (curr_file_rec_ != file_record ())
            curr_file_.open (curr_file_rec_.path.string ());
        if (curr_file_rec_.pad_end == end_)
            curr_file_rec_.pad_end = pad_end_;
    }
private:
    bool                    open_{ false };
    file_record             curr_file_rec_;
	io::mapped_file_source	curr_file_;
    std::streamsize			pos_{ 0 };
    std::streamsize         end_{ 0 };
    std::streamsize         pad_end_{ 0 };
	PermDirProvider			perm_dir_;
};

}