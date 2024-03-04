#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <limits.h>

static unsigned __int128 egcd(unsigned __int128 a, unsigned __int128 b, unsigned __int128 *e, unsigned __int128 *f) {
    if (b) {
        *e -= *f * (a / b);
        return egcd(b, a % b, f, e);
    }else
        return a;
}

static unsigned __int128 inverse(unsigned __int128 a, unsigned __int128 mask) {
    unsigned __int128 e = 1, f = -1;
    unsigned __int128 gcd = egcd(a, mask + 1 - a, &e, &f);
    if (gcd > 1)
        fprintf(stderr, "Bad, non invertible\n");
    e -= f;
    return a*e*e;
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

static uint64_t pcg64_dxsm(unsigned __int128 *state, unsigned __int128 inc) {
    const uint64_t mul = 15750249268501108917ULL;
    *state = *state * mul + inc; //FIXME correct increment

    uint64_t l = *state | 1;
    uint64_t h = *state >> 64;
    h ^= h >> 32;
    h *= mul;
    h ^= h >> 48;
    return (h*l);
}

static unsigned __int128 pow128(unsigned __int128 a, int e) {
    unsigned __int128 v = 1;
    while(e--) {
        v *= a;
    }
    return v;
}

static unsigned __int128 powsum128(unsigned __int128 a, int e) {
    unsigned __int128 v = 1;
    unsigned __int128 s = 1;
    while(e--) {
        v *= a;
        s += v;
    }
    return s;
}

int main(int argc, const char **argv) {
    const uint64_t multiplier = 15750249268501108917ULL;
    unsigned __int128 multiplier128 = multiplier;
    uint64_t h;

    if (argc < 4) {
        fprintf(stderr, "%s <seed> <delta> <offset>\n", argv[0]);
        fprintf(stderr, "Produce 2increments for the given seed and delta with a offset of 1\n");
        exit(1);
    }

    unsigned __int128 seed = parse128(argv[1]);
    unsigned __int128 delta = parse128(argv[2]);
    int offset = atoi(argv[3]);

    unsigned __int128 inc0, inc1;


    if (offset == 1) {
        inc0 = delta            - seed*(multiplier - 1);
//         inc1 = delta*multiplier - seed*(multiplier - 1);
    } else if (offset == 2) {
        unsigned __int128 inv_multiplier_1 = inverse((multiplier + 1) / 2, -1);
        printf("I: 0x%016"PRIX64" %016"PRIX64"\n", (uint64_t)(inv_multiplier_1>>64), (uint64_t)inv_multiplier_1);
        inc0 = (seed * (1 - multiplier128 * multiplier) + delta) / 2 * inv_multiplier_1; //TODO this has a 2nd solution due to the /2
    }

    unsigned __int128 inv = powsum128(multiplier, offset-1);
    //NOTE powsum128 appears to have exactly as many factors of 2 as offset (tested upto 200000) i should think about this its likely very logic why


//         printf("I: 0x%016"PRIX64" %016"PRIX64"\n", (uint64_t)(inv>>64), (uint64_t)inv);
    int f2 = 1;
    while (inv % 2 == 0) {
        inv /= 2; //NOTE we have 2 solutions here as teh MSB is not known
        f2 *= 2;
    }
    inv = inverse(inv, -1);
printf("inv: 0x%016"PRIX64" %016"PRIX64" %d\n", (uint64_t)(inv>>64), (uint64_t)inv, f2);
    inc0 = seed + delta - seed * pow128(multiplier, offset);


printf("preinv: 0x%016"PRIX64" %016"PRIX64"\n", (uint64_t)(inc0>>64), (uint64_t)inc0);
if (inc0 % f2)
    printf("non zero reminder \n");

    inc0 = inc0 / f2 * inv;

    inc1 = seed;
    for(int i = 0; i<offset+1; i++)
        inc1 = inc1*multiplier + inc0;
    inc1 -= seed*multiplier + delta;

    if (inc0 % 2 == 0 || inc1 % 2 == 0) {
        fprintf(stderr, "failure due to even increment\n");
    }

    printf("increment0: 0x%016"PRIX64" %016"PRIX64"\n", (uint64_t)(inc0>>64), (uint64_t)inc0);
    printf("increment1: 0x%016"PRIX64" %016"PRIX64"\n", (uint64_t)(inc1>>64), (uint64_t)inc1);

    unsigned __int128 seed0 = seed;
    unsigned __int128 seed1 = seed;
    for(int i = 0; i<10; i++) {
        printf("v0: 0x%016"PRIX64" %016"PRIX64"   ", (uint64_t)(seed0>>64), (uint64_t)seed0);
        printf("v1: 0x%016"PRIX64" %016"PRIX64"\n", (uint64_t)(seed1>>64), (uint64_t)seed1);
        seed0 *= multiplier; seed0 += inc0;
        seed1 *= multiplier; seed1 += inc1;
    }

    return 0;
}
