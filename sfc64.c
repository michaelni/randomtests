#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sfc64.h"

int main(int argc, char **argv) {
    FFSFC64 sfc64;

    if (argc < 6) {
        fprintf(stderr, "sfc64 <seed0> <seed1> <seed2> <seedrounds> <count> [<base>]\n");
        return 1;
    }
    uint64_t count      = strtoull(argv[5], NULL, 0);
    uint64_t base       = argc  < 7 ? 10 : strtoull(argv[6], NULL, 0);
    ff_sfc64_init(&sfc64,
                  strtoul(argv[1], NULL, 0),
                  strtoul(argv[2], NULL, 0),
                  strtoul(argv[3], NULL, 0),
                  strtoul(argv[4], NULL, 0)
                 );

    while(count--) {
        // We also test ff_sfc64_reverse_get() here, this is not needed for computing sfc64 of course

        FFSFC64 sfc64_backup= sfc64;
        printf(base == 10 ? "%20"PRIu64" %20"PRIu64" %20"PRIu64"\n" : "0x%016"PRIX64" 0x%016"PRIX64" 0x%016"PRIX64"\n", sfc64.a, sfc64.b, sfc64.c);
        uint64_t v = ff_sfc64_get(&sfc64);
        printf(base == 10 ? "%20"PRIu64"\n" : "0x%016"PRIX64"\n", v);


        FFSFC64 sfc64_copy= sfc64;
        uint64_t w = ff_sfc64_reverse_get(&sfc64_copy);
        if (memcmp(&sfc64_copy, &sfc64_backup, sizeof(sfc64_copy)) || w != v) {
            fprintf(stderr, "reverse get test failed\n");
        }
    }

    return 0;
}

