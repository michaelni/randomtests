#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>

#if STATE_BITS == 128
typedef unsigned __int128 STATET;
typedef uint64_t OUTT;
//const uint64_t multiplier = 15750249268501108917ULL;
#define SHR1 64
#define SHR2 32
#define SHR3 48
#else
typedef uint64_t STATET;
typedef uint32_t OUTT;
//const uint64_t multiplier = 6364136223846793005ULL;
#define SHR1 32
#define SHR2 16
#define SHR3 24
#endif

static OUTT pcg_dxsm(STATET *state, uint64_t multiplier, STATET inc) {
    *state = *state * multiplier + inc;

    OUTT l1 = *state | 1;
    OUTT h1 = *state >> SHR1;
    OUTT h2 = h1 ^ (h1 >> SHR2);
    OUTT h3 = h2 * multiplier;
    OUTT h4 = h3 ^ (h3 >> SHR3);
    return h4 * l1;
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
    uint64_t multiplier = strtoull(argv[1], NULL, 0);
    STATET   increment  = parse128(argv[2]);
    STATET   seed       = parse128(argv[3]);
    uint64_t count      = strtoull(argv[4], NULL, 0);
    uint64_t base       = argc  < 6 ? 10 : strtoull(argv[5], NULL, 0);

    if ((STATET)increment != increment) {
        fprintf(stderr, "increment has more bits than  specified bits\n");
        return 2;
    }
    if ((STATET)seed != seed) {
        fprintf(stderr, "seed has more bits than  specified bits\n");
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