#pragma once
#include "perm_dir.h"
#include <string>
#include <boost/iostreams/concepts.hpp> 
#include <boost/iostreams/device/mapped_file.hpp>
namespace io = boost::iostreams;

namespace raid6 {

    template <class PermDirProvider, typename CharType = char>
    class block_dir_device
    {
        static_assert(sizeof (CharType) == 1);
    public:
        typedef typename CharType				char_type;
        typedef io::seekable_device_tag         category;

        block_dir_device ()
        {}

        block_dir_device (std::string_view db_name, std::string id, fs::path const& root, std::streamsize max_size = std::numeric_limits<std::streamsize>::max ())
        {
            open (db_name, id, root, max_size);
        }

        ~block_dir_device ()
        {
            if (curr_file_.is_open ())
                curr_file_.close ();
        }

        void open (std::string_view db_name, std::string id, fs::path const& root, std::streamsize max_size = std::numeric_limits<std::streamsize>::max ())
        {
            assert (!open_);

            perm_dir_.open (db_name, id, root);
            end_ = max_size;
            open_ = true;
        }

        PermDirProvider const& provider () const
        {
            assert (open_);

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
                next = end_ - off - 1;
            }
            else {
                throw ios_base::failure ("bad seek direction");
            }

            // Check for errors
            if (next < 0 || next >= static_cast<io::stream_offset>(end_))
                throw ios_base::failure ("bad seek offset");

            pos_ = next;
            return pos_;
        }

        std::streamsize read (char_type* s, std::streamsize n)
        {
            assert (open_);

            if (!curr_file_.is_open ())
                open_next_read ();
            std::streamsize returned = 0;
            std::streamsize left = n;
            while (returned < n)
            {
                if (pos_ < curr_file_rec_.end)
                {
                    auto pos_in_curr = pos_ - curr_file_rec_.start;
                    auto l = std::min (left, curr_file_rec_.end - pos_);
                    std::memcpy (s + returned, curr_file_.const_data () + pos_in_curr, l);
                    returned += l;
                    left -= l;
                    pos_ += l;
                }
                
                if (pos_ >= curr_file_rec_.end && returned < n)
                {
                    curr_file_.close ();
                    if (pos_ < end_)
                        open_next_read ();
                    else if (returned > 0)
                        return returned;
                    else
                        return -1;
                }
            }

            return returned;
        }

        std::streamsize write (const char_type* s, std::streamsize n)
        {
            assert (open_);

            if (!curr_file_.is_open ()) 
                open_next_write ();
            else if (curr_file_.flags () & io::mapped_file::mapmode::readonly)
            {
                curr_file_.close ();
                open_next_write ();
            }

            std::streamsize written = 0;
            std::streamsize left = n;
            while (written < n)
            {
                if (pos_ < curr_file_rec_.end)
                {
                    auto pos_in_curr = pos_ - curr_file_rec_.start;
                    auto l = std::min (left, curr_file_rec_.end - pos_);
                    std::memcpy (curr_file_.data () + pos_in_curr, s + written, l);
                    written += l;
                    left -= l;
                    pos_ += l;
                }

                if (pos_ < curr_file_rec_.pad_end && written < n)
                {
                    auto l = std::min(left, curr_file_rec_.pad_end - pos_);
#ifdef _DEBUG
                    for (size_t pd = 0; pd < l; ++pd)
                        assert(*(s + n - left + pd) == 0);
#endif
                    written += l;
                    left -= l;
                    pos_ += l;
                }

                if (pos_ >= curr_file_rec_.pad_end && written < n)
                {
                    curr_file_.close ();
                    if (pos_ < end_)
                        open_next_write ();
                    else if (written > 0)
                        return written;
                    else
                        return -1;
                }
            }

            return written;
        }

        file_record const& curr_file () const
        {
            return curr_file_rec_;
        }

    private:
        void open_next_read ()
        {
            curr_file_rec_ = perm_dir_.at (pos_);
            if (curr_file_rec_ != file_record ())
            {
                io::mapped_file_params params;
                params.flags = io::mapped_file::mapmode::readonly;
                params.path = curr_file_rec_.path.string ();
                curr_file_.open (params);
            }
        }

        void open_next_write ()
        {
            curr_file_rec_ = perm_dir_.at (pos_);
            if (curr_file_rec_ != file_record ())
            {
                io::mapped_file_params params;
                params.flags = io::mapped_file::mapmode::readwrite;
                params.path = curr_file_rec_.path.string ();
                if (!fs::exists (curr_file_rec_.path))
                {
                    fs::create_directories (curr_file_rec_.path.parent_path ());
                    params.new_file_size = curr_file_rec_.end - curr_file_rec_.start;
                }
                curr_file_.open (params);
            }
        }
    private:
        bool                    open_{ false };
        file_record             curr_file_rec_;
        io::mapped_file	        curr_file_;
        std::streamsize			pos_{ 0 };
        std::streamsize         end_{ 0 };
        PermDirProvider			perm_dir_;
    };

}