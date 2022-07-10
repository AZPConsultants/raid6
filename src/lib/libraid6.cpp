#include <stdexcept>
#include <gf_rand.h>
#include "jerasure.h"
#include "reed_sol.h"
#include "cauchy.h"
#include "liberation.h"

#include "libraid6.h"

namespace raid6 {

encoder::encoder(size_t packetsize) : packetsize_(packetsize)
{} 

void encoder::calculate(std::vector<buff_t>& data, std::vector<buff_t>& coding)
{
	int k = data.size();
	int m = coding.size();
	const int w = 8;
	// init
	if (bitmatrix_ == nullptr)
		bitmatrix_ = liber8tion_coding_bitmatrix(k);
	if (schedule_ == nullptr)
		schedule_ = jerasure_smart_bitmatrix_to_schedule(k, m, w, bitmatrix_);

	char** cdata = (char**)malloc(sizeof(char*) * k);
	char** ccoding = (char**)malloc(sizeof(char*) * w);

	for (size_t i = 0; i < k; ++i) {
		cdata[i] = data[i].data();
	}
	for (size_t i = 0; i < m; ++i) ccoding[i] = coding[i].data();

	jerasure_schedule_encode(k, m, w, schedule_, cdata, ccoding, data[0].size(), packetsize_);

	free(cdata);
	free(ccoding);
}

decoder::decoder(size_t packetsize) : packetsize_(packetsize)
{}

void decoder::calculate(std::vector<buff_t>& data, std::vector<buff_t>& coding, std::vector<int>& erasures)
{
	int k = data.size();
	int m = coding.size();
	const int w = 8;
	// init
	if (bitmatrix_ == nullptr)
		bitmatrix_ = liber8tion_coding_bitmatrix(k);

	char** cdata = (char**)malloc(sizeof(char*) * k);
	char** ccoding = (char**)malloc(sizeof(char*) * w);

	for (size_t i = 0; i < k; ++i) {
		cdata[i] = data[i].data();
	}
	for (size_t i = 0; i < m; ++i) ccoding[i] = coding[i].data();

	int res = jerasure_schedule_decode_lazy(k, m, w, bitmatrix_, erasures.data(), cdata, ccoding, data[0].size(), packetsize_, 1);

	free(cdata);
	free(ccoding);

	if (res == -1) throw new std::runtime_error("decoding failed");
}
}