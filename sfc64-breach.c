// Copyright 2024 Michael Niedermayer <michael-random@niedermayer.cc>
// GPL 2+

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sfc64.h"

#define MAX_VALUES 64

#define ABITS 19 // 17 works rarely, 18 works often, try other numbers if one doesnt work
static uint64_t b2c_lut[1<<ABITS][16];

static void build_b2c(void){
    unsigned mask = (8<<ABITS) - 1;

    for (int i = 0; i<(8<<ABITS); i++) {
        int i9 = (i*9) & mask;
        b2c_lut[i9>>3][i&15] = (uint64_t)i << (64 - 3 - ABITS);
    }
}

static int check(FFSFC64 *s, uint64_t prev_c, const uint64_t *o) {
    uint64_t c = ((prev_c << 24) | (prev_c >> 40)) + o[0];
    uint64_t b = 9* prev_c;

    s->a       = b ^ (b >> 11);
    s->b       = 9*c;
    s->c       = ((c << 24) | (c >> 40)) + o[1];

    s->counter = o[2] - s->a - s->b;

    for (int i= 1; i>=-1; i--)
        if (ff_sfc64_reverse_get(s) != o[i])
            return 0;
    while(s->counter > 1)
        ff_sfc64_reverse_get(s);
    return 1;
}

static int64_t step;

static void breach_step(uint64_t a, uint64_t b, uint64_t prev_c, const uint64_t *o, int depth) {
    uint64_t AMASK = -1ull >> (ABITS + 3);


    for (int ci = 0; ci < 16; ci++) {
        uint64_t c2 = (prev_c & AMASK) | b2c_lut[b>>(64 - ABITS)][ci]; // 3 bits more than in b
        uint64_t b2 = b, a2 = a;
        uint64_t c = c2;
        //TODO try adding a + 0.5

        if (!b2c_lut[b>>(64 - ABITS)][ci])
            continue;

        int cycle;
        if (depth == 2 || depth == 5) { // access o[0..-5]
            for (cycle=0; cycle>-5; cycle--) {
                b2  = a2 ^ (a2>>11); b2 ^= b2>>22; b2 ^= b2>>44;
                a2  = o[cycle] - b2;
                c2 -= o[cycle-1];
                c2  = c2 >> 24 | c2 << 40;
            }
        } else { //access o[0..4]
            for (cycle=0; cycle<3; cycle++) {
                c2 = (c2 << 24 | c2 >> 40) + o[cycle]; //end of first cycle
                a2 = b2 ^ (b2>>11);                        //begin of next cycle
                b2 = o[cycle+2] - a2;                     //look ahead to resolve b
            }
        }
        step ++;

        uint64_t score = llabs((int64_t)(b2 - c2 * 9));
        if (score >>56) //60 (bits=16) 56 (bits=17)
            continue;

        FFSFC64 sfc;
        if (depth>5 && check(&sfc,c,o)) {
            printf("Found (a=0x%016"PRIX64" b=0x%016"PRIX64" c=0x%016"PRIX64" counter=0x%016"PRIX64"\n", sfc.a,sfc.b,sfc.c, sfc.counter);
            printf("step %"PRId64"\n", step);
            exit(1);
        }

        if (depth < 7) {
            breach_step(a2, b2, c2, o + cycle, depth+1);
        }
    }
}

static int breach(FFSFC64 *sfc64, uint64_t *o, int num_values) {
    step = 0;
    for (int ai = 0; ai<(1<<ABITS); ai++) {
        uint64_t a = (uint64_t)ai << (64 - ABITS); //TODO as is its better to compute b first as a i sunused
        a += 1LL<<(63 - ABITS); // better rounding maybe
        uint64_t b = o[0] - a;

        a = b ^ (b>>11);

        b = o[1] - a; //from next iteration

        breach_step(a,b,0, o, 0);
    }
    return 0;
}

int main(int argc, char **argv)
{
    if (argc < 5) {
        fprintf(stderr, "sfc64-breach <value0> <value1> <value2> ...\n");
        return 1;
    }
    uint64_t o[MAX_VALUES];
    int num_values = argc - 1;
    if (num_values > MAX_VALUES)
        num_values = MAX_VALUES;

    FFSFC64 sfc64;

    for(int i = 0; i<num_values; i++)
        o[i] = strtoull(argv[1+i], NULL, 0);

    build_b2c();
    for (int i = 0; i<num_values - 10; i++) {
        printf("attempt %d\n", i);
        breach(&sfc64, o+i, num_values-i);
    }
    return 0;
}
