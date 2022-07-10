#pragma once
#include <bsoncxx/document/element.hpp>
#include <bsoncxx/array/element.hpp>
#include <bsoncxx/types.hpp>

namespace raid6 {

inline int64_t get_int (bsoncxx::document::element const & ele_int_32_or_64, bool try_double=false)
{
    if (ele_int_32_or_64.type () == bsoncxx::type::k_int64)
        return (int64_t)ele_int_32_or_64.get_int64 ();
    else if (ele_int_32_or_64.type () == bsoncxx::type::k_int32)
        return (int64_t)ele_int_32_or_64.get_int32 ();
    else if (try_double && ele_int_32_or_64.type () == bsoncxx::type::k_double)
        return (int64_t)ele_int_32_or_64.get_double ();
    else
    {
        std::stringstream st;
        st << "expected k_int32/64, got type: " << (int)ele_int_32_or_64.type ();
        throw(std::runtime_error (st.str()));
    }
}

inline int64_t get_int_opt (bsoncxx::document::element const & ele_int_32_or_64, int64_t default_val = 0)
{
    if (ele_int_32_or_64)
    {
        return get_int (ele_int_32_or_64);
    }
    else
        return default_val;
}

inline int64_t get_int (bsoncxx::array::element const & ele_int_32_or_64)
{
    if (ele_int_32_or_64.type () == bsoncxx::type::k_int32)
        return ele_int_32_or_64.get_int32 ();
    return ele_int_32_or_64.get_int64 ();
}

inline int64_t get_int_opt (bsoncxx::array::element const & ele_int_32_or_64, int64_t default_val = 0)
{
    if (ele_int_32_or_64)
    {
        return get_int (ele_int_32_or_64);
    }
    else
        return default_val;
}

inline uint64_t get_uint (bsoncxx::document::element const & ele_int_32_or_64)
{
    if (ele_int_32_or_64.type () == bsoncxx::type::k_int32)
        return (uint64_t)ele_int_32_or_64.get_int32 ();
    return (uint64_t)ele_int_32_or_64.get_int64 ();
}

inline double get_num (bsoncxx::document::element const & ele_num)
{
    if (ele_num.type () == bsoncxx::type::k_int32)
        return (double)ele_num.get_int32 ();
    else if (ele_num.type () == bsoncxx::type::k_int64)
        return (double)ele_num.get_int64 ();
    return ele_num.get_double ();
}

inline double get_num_opt (bsoncxx::document::element const & ele_num, double default_val = 0.0)
{
    if (ele_num)
    {
        return get_num (ele_num);
    }
    else
        return default_val;
}

inline std::string get_str (bsoncxx::document::element const & ele_str)
{
    return ele_str.get_utf8 ().value.to_string ();
}

inline std::string get_str_opt (bsoncxx::document::element const & ele_str,
                                std::string const& default_val = std::string (),
                                bool replace_empty_with_default = true)
{
    if (ele_str)
    {
        std::string s = ele_str.get_utf8 ().value.to_string ();
        if (replace_empty_with_default && s.empty ())
            return default_val;
        else
            return s;
    }
    else
        return default_val;
}

inline std::string get_str (bsoncxx::array::element const & ele_str)
{
    return ele_str.get_utf8 ().value.to_string ();
}

inline std::string get_str_opt (bsoncxx::array::element const & ele_str,
                                std::string const& default_val = std::string ())
{
    if (ele_str)
        return ele_str.get_utf8 ().value.to_string ();
    else
        return default_val;
}

}
