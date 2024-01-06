// Copyright 2024 Michael Niedermayer <michael-random@niedermayer.cc>
// GPL 2+

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

static uint64_t egcd(uint64_t a, uint64_t b, uint64_t *e, uint64_t *f) {
    if (b) {
        *e -= *f * (a / b);
        return egcd(b, a % b, f, e);
    }else
        return a;
}

static uint64_t inverse(uint64_t a, uint64_t mask) {
    uint64_t e = 1, f = -1;
    uint64_t gcd = egcd(a, mask + 1 - a, &e, &f);
    if (gcd > 1)
        fprintf(stderr, "Bad, non invertible multiplier, this is not a sane LCG\n");
    e -= f;
    return a*e*e;
}

int main(int argc, char **argv)
{
    if (argc < 5) {
        fprintf(stderr, "lcg.breach <bits> <value0> <value1> <value2> [<value3>]\n");
        return 1;
    }
    int bits    = strtoull(argv[1], NULL, 0);
    uint64_t o0 = strtoull(argv[2], NULL, 0);
    uint64_t o1 = strtoull(argv[3], NULL, 0);
    uint64_t o2 = strtoull(argv[4], NULL, 0);

    if (bits < 0 || bits > 64) {
        fprintf(stderr, "bad bits\n");
        return 2;
    }
    uint64_t mask       = (bits == 64 ? 0 : (1ull << bits)) - 1;
    if (o0 > mask || o1 > mask || o2 >mask) {
        fprintf(stderr, "values exceed specified bits\n");
        return 2;
    }

    uint64_t     multiplier = ((o1 - o2) * inverse(o0 - o1, mask)) & mask;
    uint64_t inv_multiplier = inverse(multiplier, mask) & mask;
    uint64_t increment      = (o1 - o0*multiplier) & mask;
    uint64_t seed           = ((o0 - increment)*inv_multiplier) & mask;

    if (argc > 5) {
        uint64_t o3 = strtoull(argv[5], NULL, 0);
        if (((o2 * multiplier + increment) & mask) == o3) {
            printf("Verified Parameters!\n");
        } else {
            fprintf(stderr, "Not a LCG\n");
            return 2;
        }
    }

    printf("Seed:       %19"PRIu64" 0x%016"PRIX64"\n", seed, seed);
    printf("Multiplier: %19"PRIu64" 0x%016"PRIX64"\n", multiplier, multiplier);
    printf("Increment:  %19"PRIu64" 0x%016"PRIX64"\n", increment, increment);
}
