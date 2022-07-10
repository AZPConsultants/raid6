#pragma once
#include "types.h"
#include <vector>

namespace raid6 {

class encoder {
public:
	encoder(size_t packetsize = sizeof(size_t));
	void calculate(std::vector<buff_t>& data, std::vector<buff_t>& coding);
private:
	int packetsize_;
	int* bitmatrix_ {nullptr};
	int** schedule_ {nullptr};
};

class decoder {
public:
	decoder(size_t packetsize = sizeof(size_t));
	void calculate(std::vector<buff_t>& data, std::vector<buff_t>& coding, std::vector<int>& erasures);
private:
	int packetsize_;
	int* bitmatrix_{ nullptr };
};

}