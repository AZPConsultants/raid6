#ifndef RS_H
#define RS_H

#include <vector>
#include <array>
#include "types.h"

namespace ds{
class RS
{
public:
    RS();
    bool init();
    ~RS();
    bool rs_encode(std::vector<buff_t> const & inputs, std::vector<buff_t> & parities);
    std::vector<uint8_t> rs_recoverDQ(std::vector<uint8_t> input, int nofd, uint8_t p);
    std::vector<uint8_t> rs_recoverDP(std::vector<uint8_t> input, int nofd, uint8_t q);
    std::vector<uint8_t> rs_recoverDD(std::vector<uint8_t> input, int nofd1, int nofd2, uint8_t p, uint8_t q);
    short *gflog, *gfilog;

private:
    unsigned int prim_poly_4;
    unsigned int prim_poly_8;
    unsigned int prim_poly_16;


    int setup_tables(int w);
    int gfmult(int a, int b);
    int gfdiv(int a, int b);
};
}
#endif // RS_H
