// Copyright 2024 Michael Niedermayer <michael-random@niedermayer.cc>
// GPL 2+

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    if (argc < 6) {
        fprintf(stderr, "lcg <bits> <multiplier> <increment> <seed> <count>\n");
        return 1;
    }
    int bits            = strtoull(argv[1], NULL, 0);
    uint64_t multiplier = strtoull(argv[2], NULL, 0);
    uint64_t increment  = strtoull(argv[3], NULL, 0);
    uint64_t seed       = strtoull(argv[4], NULL, 0);
    uint64_t count      = strtoull(argv[5], NULL, 0);

    if (bits < 0 || bits > 64) {
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
        printf("%"PRIu64"\n", seed & mask);
    }
    return 0;
}
