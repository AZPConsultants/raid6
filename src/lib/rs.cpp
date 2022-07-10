#include "rs.h"
#include <stdlib.h>
#include "types.h"
#include <array>
#include <vector>

#define NW(w) (1 << w) /* NW equals 2 to the w-th power */
namespace ds{
RS::RS()
{
    prim_poly_4 = 023;
    prim_poly_8 = 0435;
    prim_poly_16 = 0210013;
}

bool RS::init() {
    setup_tables(8);
    return true;
}

RS::~RS()
{
}

bool RS::rs_encode(std::vector<buff_t> const& inputs, std::vector<buff_t>& parities)
{
    uint8_t p = 0;
    uint8_t q = 0;

    auto ND = inputs.size();
    auto NP = parities.size();

    for (size_t pos = 0; pos < BLOCK_SIZE; ++pos) {
        for (size_t n = 0; n < ND; ++n) {
            parities[0][pos] ^= inputs[n][pos];
            parities[1][pos] ^= gfmult(gfilog[n], inputs[n][pos]);
        }
    }
    return true;
    
}

std::vector<uint8_t> RS::rs_recoverDQ(std::vector<uint8_t> input, int nofd, uint8_t p)
{
    std::vector<uint8_t> res;
    uint8_t recD = 0;
    uint8_t recQ = 0;
    for(int i = 0; i < input.size(); i++){
        if(i != nofd)
            recD ^= input[i];
    }
    recD ^= p;

    input[nofd] = recD;

    for(int i = 0; i < input.size(); i++){
        recQ ^= (uint8_t)gfmult(gfilog[i], input[i]);
    }
    res.push_back(recD);
    res.push_back(recQ);
    return res;
}

std::vector<uint8_t> RS::rs_recoverDP(std::vector<uint8_t> input, int nofd, uint8_t q)
{
    std::vector<uint8_t> res;
    uint8_t recD = 0;
    uint8_t recP = 0;

    uint8_t qPrime = 0;
    for(int i = 0; i < input.size(); i++){
        if(i != nofd)
            qPrime ^= (uint8_t)gfmult(gfilog[i], input[i]);
        recP ^= input[i];
    }

    /*
    uint8_t gInverse = gfilog[nofd];                        //inverse of a is a^(p^n)−2 Wikipedia
    for(int i = 0; i < 6; i++){                             //2^3 -2 = 6
        gInverse = gfmult(gInverse, gfilog[nofd]);
    }
    */
    uint8_t gInverse = gfdiv(1, gfilog[nofd]);

    recD = gfmult(gInverse, (q ^ qPrime));
    res.push_back(recD);
    res.push_back(recP);
    return res;
}

std::vector<uint8_t> RS::rs_recoverDD(std::vector<uint8_t> input, int nofd1, int nofd2, uint8_t p, uint8_t q)
{
    std::vector<uint8_t> res;
    uint8_t recD1 = 0;
    uint8_t recD2 = 0;
    uint8_t pPrime = 0;
    uint8_t qPrime = 0;

    for(int i = 0; i < input.size(); i++){
        if(i == nofd1 || i == nofd2)
            continue;
        else{
            qPrime ^= (uint8_t)gfmult(gfilog[i], input[i]);
            pPrime ^= input[i];
        }
    }

    recD1 = gfmult(gfdiv(1, (gfilog[nofd1] ^ gfilog[nofd2])), (gfmult(gfilog[nofd2], (p ^ pPrime)) ^ q ^ qPrime));
    recD2 = recD1 ^ (p ^ pPrime);

    res.push_back(recD1);
    res.push_back(recD2);
    return res;
}

int RS::setup_tables(int w)
{
    unsigned int b, log, x_to_w, prim_poly;
    switch (w) {
    case 4: prim_poly = prim_poly_4; break;
    case 8: prim_poly = prim_poly_8; break;
    case 16: prim_poly = prim_poly_16; break;
    default: return -1;
    }
    x_to_w = 1 << w;
    gflog = (short *)malloc(sizeof(short) * x_to_w);
    gfilog = (short *)malloc(sizeof(short) * x_to_w);
    b = 1;
    for (log = 0; log < x_to_w - 1; log++) {
        gflog[b] = (short)log;
        gfilog[log] = (short)b;
        b = b << 1;
        if (b & x_to_w) b = b ^ prim_poly;
    }
    return 0;
}

int RS::gfmult(int a, int b) {
    int sum_log;
    if (a == 0 || b == 0) return 0;
    sum_log = gflog[a] + gflog[b];
    if (sum_log >= NW(8) - 1) sum_log -= NW(8) - 1;
    return gfilog[sum_log];
}

int RS::gfdiv(int a, int b)
{
    int diff_log;
    if (a == 0) return 0;
    if (b == 0) return -1; /* Can’t divide by 0 */
    diff_log = gflog[a] - gflog[b];
    if (diff_log < 0) diff_log += NW(8) - 1;
    return gfilog[diff_log];
}
}