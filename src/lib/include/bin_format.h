#pragma once
#include <iterator>

template <typename T>
concept ConstIterator = 
	std::same_as<const uint8_t*, typename T::pointer>;


template <typename T>
concept NonConstIterator = 
	std::same_as<uint8_t*, typename T::pointer>;

template <typename T>
concept OutputIterator = requires(T t) {
	{ typename std::iterator_traits<T>::iterator_category{} } -> std::output_iterator;
};

template<class It>
class bin_format
{
public:
	bin_format (It it) : it (it)
	{}

	
	auto operator+ (uint8_t& val) requires ConstIterator<It> {
		val = *it;
		++it;
		return *this;
	}

	
	auto operator+ (uint8_t val) requires NonConstIterator<It> {
		*it = val;
		++it;
		return *this;
	}

private:

	It	it;
};