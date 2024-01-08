// Copyright 2024 Michael Niedermayer <michael-random@niedermayer.cc>
// GPL 2+

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    if (argc < 6) {
        fprintf(stderr, "lcg <seed bits> <multiplier> <increment> <seed> <count> [<output bits> [<base>]]\n");
        fprintf(stderr, "When output bits is set to a value less than bits then the output will be a truncated LCG\n");
        return 1;
    }
    uint64_t bits       = strtoull(argv[1], NULL, 0);
    uint64_t multiplier = strtoull(argv[2], NULL, 0);
    uint64_t increment  = strtoull(argv[3], NULL, 0);
    uint64_t seed       = strtoull(argv[4], NULL, 0);
    uint64_t count      = strtoull(argv[5], NULL, 0);
    uint64_t output_bits= argc == 6 ? bits : strtoull(argv[6], NULL, 0);
    uint64_t base       = argc  < 8 ? 10   : strtoull(argv[7], NULL, 0);

    if (bits > 64 || output_bits > bits) {
        fprintf(stderr, "bad bits\n");
        return 2;
    }
    uint64_t mask       = (bits == 64 ? 0 : (1ull << bits)) - 1;
    if (multiplier > mask) {
        fprintf(stderr, "multiplier has more bits than specified bits\n");
        return 2;
    }
    if (increment > mask) {
        fprintf(stderr, "increment has more bits than  specified bits\n");
        return 2;
    }
    if (seed > mask) {
        fprintf(stderr, "seed has more bits than  specified bits\n");
        return 2;
    }

    while(count--) {
        seed *= multiplier;
        seed += increment;
        uint64_t out = (seed & mask) >> (bits - output_bits);
        printf(base == 10 ? "%20"PRIu64"\n" : "0x%016"PRIX64"\n", out);
    }
    return 0;
}
