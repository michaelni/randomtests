#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>

#if STATE_BITS == 128
typedef unsigned __int128 STATET;
typedef uint64_t OUTT;
//const uint64_t multiplier = 15750249268501108917ULL;
#else
typedef uint64_t STATET;
typedef uint32_t OUTT;
//const uint64_t multiplier = 6364136223846793005ULL;
#endif

static OUTT pcg_dxsm(STATET *state, uint64_t multiplier, STATET inc) {
    *state = *state * multiplier + inc;

    OUTT h = *state >>   STATE_BITS / 2;
    h     ^= h      >>   STATE_BITS / 4;
    h     *= multiplier;
    h     ^= h      >> 3*STATE_BITS / 8;
    return h * (*state | 1);
}

static unsigned __int128 parse128(const char *s) {
    unsigned __int128 v = 0;
    int base = 10;

    if (s[0] == '0' && s[1] == 'x'){
        base = 16;
        s+=2;
    }

    while (*s) {
        int c = *s++;
        if (c > 0x60)
            c -= 0x20;
        if (c > 0x46) {
            goto error;
        } else if (c > 0x40) {
            c += 10 - 0x41;
        } else if (c > 0x39) {
            goto error;
        } else if (c > 0x2F) {
            c -= 0x30;
        } else
            goto error;
        v = base*v + c;
    }

    return v;

error:
    fprintf(stderr, "Failed to parse %s\n", s);
    exit(7);
}

int main(int argc, char **argv) {
    if (argc < 5) {
        fprintf(stderr, "%s <multiplier> <increment> <seed> <count> [<base>]\n", *argv);
        return 1;
    }
    unsigned __int128 multiplier128 = parse128(argv[1]);
    unsigned __int128 increment128  = parse128(argv[2]);
    unsigned __int128 seed128       = parse128(argv[3]);
    uint64_t count                  = strtoull(argv[4], NULL, 0);
    uint64_t base                   = argc  < 6 ? 10 : strtoull(argv[5], NULL, 0);
    uint64_t multiplier = multiplier128;
    STATET increment    = increment128;
    STATET seed         = seed128;

    if (multiplier128 != multiplier || !multiplier) {
        fprintf(stderr, "multiplier invalid\n");
        return 2;
    }
    if (increment128 != increment || (increment%2) == 0) {
        fprintf(stderr, "increment invalid\n");
        return 2;
    }
    if (seed128 != seed) {
        fprintf(stderr, "seed out of range\\n");
        return 2;
    }

    while(count--) {
        uint64_t out = pcg_dxsm(&seed, multiplier, increment);
        if (STATE_BITS == 128) {
            printf(base == 10 ? "%20"PRIu64"\n" : "0x%016"PRIX64"\n", out);
        } else
            printf(base == 10 ? "%10"PRIu64"\n" : "0x%08"PRIX64"\n", out);
    }
    return 0;

}
