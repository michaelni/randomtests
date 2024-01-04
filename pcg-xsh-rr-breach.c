//Copyrite 2024 Michael Niedermayer michael-random@niedermayer.cc
//GPL License

#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#if STATE_BITS == 128
#define ROTATE_BITS 6
#define MIX_BITS 35
typedef unsigned __int128 STATET;
typedef uint64_t OUTT;
static unsigned __int128 const multiplier = ((unsigned __int128)2549297995355413924ULL<<64) + 4865540595714422341ULL;
static unsigned __int128 const increment  = ((unsigned __int128)6364136223846793005ULL<<64) + 1442695040888963407ULL;

#else // 64 bit
#define ROTATE_BITS 5
#define MIX_BITS 18
typedef uint64_t STATET;
typedef uint32_t OUTT;
static uint64_t const multiplier = 6364136223846793005u;
static uint64_t const increment  = 1442695040888963407u;
#endif


#define ROTATE_MASK ((1<<ROTATE_BITS) - 1)
#define ROTATE_SRC (STATE_BITS - ROTATE_BITS)

#if 0
// PCG-XSL-RR TODO
#define EXIT_BITS 0
MIX_BITS are wrong
#else // PCG-XSH-RR
#define EXIT_BITS (STATE_BITS/2 - ROTATE_BITS)
#endif

static const STATET MSB = (STATET)(-1) << EXIT_BITS;

static OUTT rotr(OUTT x, unsigned r)
{
    return x >> (r & ROTATE_MASK) | x << (-r & ROTATE_MASK);
}

OUTT pcg(STATET *state)
{
    STATET x = *state;
    unsigned count = (unsigned)(x >> ROTATE_SRC);

    *state = x * multiplier + increment;
    x ^= x >> MIX_BITS;
    return rotr((OUTT)(x >> EXIT_BITS), count);
}

static STATET inv_multiplier = (STATET)0-1;

void pcg_init(STATET *state, STATET seed)
{
    *state = seed + increment;
    (void)pcg(state);
}

#define FFMIN(a,b) ((a) > (b) ? (b) : (a))
STATET egcd(STATET a, STATET b, STATET*f, STATET *e) {
    if (b) {
        *f -= *e * (a / b);
        return egcd(b, a % b, e, f);
    }else
        return a;
}

static int make_a_table(STATET *table, int bits)
{
    memset(table, -1, sizeof(*table) * 300);
    int j;
    uint64_t i = 0;
    STATET v = 0;

    if (bits == 58) { // avoid 5 minutes of table precalculation
        static const uint64_t table128[152] = {
 0x0000000000000001,0x0000000000000008,0x000000000000000F,0x0000000000000016,0x000000000000001D,0x0000000000000098,0x0000000000000113,0x000000000000018E,
 0x0000000000000209,0x000000000000048D,0x0000000000000B9E,0x000000000000B553,0x0000000000015F08,0x00000000000208BD,0x000000000002B272,0x0000000000035C27,
 0x00000000000405DC,0x000000000004AF91,0x0000000000055946,0x00000000000602FB,0x000000000006ACB0,0x0000000000075665,0x000000000008001A,0x000000000008A9CF,
 0x0000000000095384,0x000000000009FD39,0x00000000000AA6EE,0x00000000000B50A3,0x00000000000BFA58,0x00000000000CA40D,0x00000000000D4DC2,0x00000000000DF777,
 0x00000000000EA12C,0x00000000000F4AE1,0x00000000000FF496,0x0000000000109E4B,0x0000000000114800,0x000000000011F1B5,0x0000000000129B6A,0x000000000013451F,
 0x000000000013EED4,0x0000000000149889,0x000000000015423E,0x000000000015EBF3,0x00000000001695A8,0x0000000000173F5D,0x000000000017E912,0x00000000001892C7,
 0x0000000000193C7C,0x000000000019E631,0x00000000001A8FE6,0x00000000001B399B,0x000000000A3643D5,0x0000000014514E0F,0x000000001E6C5849,0x0000000028876283,
 0x0000000032A26CBD,0x00000000D4A4BD2E,0x0000000176A70D9F,0x0000000218A95E10,0x00000002BAABAE81,0x000000061759AD73,0x000000097407AC65,0x0000000CD0B5AB57,
 0x000000102D63AA49,0x000000138A11A93B,0x00000016E6BFA82D,0x0000005EF7AC9FA6,0x00000106004636C5,0x000001AD08DFCDE4,0x0000025411796503,0x000002FB1A12FC22,
 0x000003A222AC9341,0x000004492B462A60,0x000004F033DFC17F,0x000005973C79589E,0x0000063E4512EFBD,0x000006E54DAC86DC,0x0000078C56461DFB,0x00003D09BACA86F7,
 0x000072871F4EEFF3,0x0000A80483D358EF,0x0000DD81E857C1EB,0x000112FF4CDC2AE7,0x0001487CB16093E3,0x00017DFA15E4FCDF,0x0001B3777A6965DB,0x0001E8F4DEEDCED7,
 0x00021E72437237D3,0x000253EFA7F6A0CF,0x0002896D0C7B09CB,0x0002BEEA70FF72C7,0x0002F467D583DBC3,0x000329E53A0844BF,0x00035F629E8CADBB,0x000394E0031116B7,
 0x0003CA5D67957FB3,0x0003FFDACC19E8AF,0x00043558309E51AB,0x00046AD59522BAA7,0x0004A052F9A723A3,0x0004D5D05E2B8C9F,0x00050B4DC2AFF59B,0x000540CB27345E97,
 0x000576488BB8C793,0x0005ABC5F03D308F,0x0005E14354C1998B,0x000616C0B9460287,0x00064C3E1DCA6B83,0x000681BB824ED47F,0x0006B738E6D33D7B,0x0006ECB64B57A677,
 0x00072233AFDC0F73,0x000757B11460786F,0x00078D2E78E4E16B,0x0007C2ABDD694A67,0x0007F82941EDB363,0x00082DA6A6721C5F,0x000863240AF6855B,0x000898A16F7AEE57,
 0x0008CE1ED3FF5753,0x0009039C3883C04F,0x000939199D08294B,0x00096E97018C9247,0x0009A4146610FB43,0x0009D991CA95643F,0x000A0F0F2F19CD3B,0x000A448C939E3637,
 0x000A7A09F8229F33,0x000AAF875CA7082F,0x000AE504C12B712B,0x000B1A8225AFDA27,0x000B4FFF8A344323,0x000B857CEEB8AC1F,0x000BBAFA533D151B,0x000BF077B7C17E17,
 0x000C25F51C45E713,0x000C5B7280CA500F,0x000C90EFE54EB90B,0x000CC66D49D32207,0x000CFBEAAE578B03,0x000D316812DBF3FF,0x000D66E577605CFB,0x000D9C62DBE4C5F7,
 0x000DD1E040692EF3,0x000E075DA4ED97EF,0x000E3CDB097200EB,0x000E72586DF669E7,0x000EA7D5D27AD2E3,0x002C2CFEDBF4E1A5,0x0049B227E56EF067,0x00B0E978D457EF90
        };
        for(int i = 0; i<152; i++)
            table[i] = table128[i] * multiplier;
        return 152;
    }

    for(j = 0; j < 58; j++) {
        for(; i < (1LL<<bits); i++) {
            v += multiplier;
            if (j && table[j-1] <= v)
                continue;
            table[j] = v;
            printf("Table %3d 0x%016"PRIX64"%016"PRIX64" 0x%016"PRIX64"\n", j, (uint64_t)((unsigned __int128)table[j]>>64), (uint64_t)table[j], (uint64_t)table[j] * (uint64_t)inv_multiplier);
            break;
        }
        if (i == (1LL<<bits))
            break;
    }
#if STATE_BITS == 128 // we must use a bit more effort to compute the rest of the table as this is too slow otherwise
#define COMPUTE_BITS 29
#define MEM_BITS (56 - COMPUTE_BITS)
#define LUT_BITS 28 //with 29 malloc fails
#define LUT_SIZE (1<<LUT_BITS)
    uint32_t *lut = malloc(sizeof(*lut) * LUT_SIZE);
    memset(lut, 0, sizeof(*lut) * LUT_SIZE);
    printf("Making LUT %p\n", lut);
    v = 0;
    for(int i = 0; i < (1<<(56 - MEM_BITS)); i++) {
        lut[v >> (STATE_BITS - LUT_BITS)] = i;
        v += multiplier;
    }
    printf("Done\n");

    STATET multiplier2 = multiplier << MEM_BITS;
    v = 0;
    i = 0;
    for(; j < 300; j++) {
        for(; i < (1LL<<COMPUTE_BITS); i++) {
            v += multiplier2;
            int index = (-v) >> (STATE_BITS - LUT_BITS);
            STATET v2 = v + lut[index] * multiplier;

            if (table[j-1] <= v2)
                continue;
            table[j] = v2;
            printf("Table %3d 0x%016"PRIX64"%016"PRIX64" 0x%016"PRIX64"\n", j, (uint64_t)(table[j]>>64), (uint64_t)table[j], (uint64_t)table[j] * (uint64_t)inv_multiplier);
            break;
        }
        if (i == (1LL<<COMPUTE_BITS))
            break;
    }

    free(lut);
#endif

    return j;
}


STATET breach(OUTT v[3], int pos)
{
    STATET w[2];
    STATET c, d;
    STATET table[300];

    int table_size = make_a_table(table, EXIT_BITS);

    printf("\nBreaching spin:");
    for(int r = 0; r<(1<<(ROTATE_BITS*2)); r++){
        printf("."); fflush(stdout);
        w[0] = (STATET)(r &ROTATE_MASK) << ROTATE_SRC;
        w[1] = (STATET)(r>>ROTATE_BITS) << ROTATE_SRC;
        for (int j = 0; j<2; j++) {
            w[j] += (STATET)rotr(v[j], -(w[j] >> ROTATE_SRC)) << EXIT_BITS;
            w[j] ^= w[j] >> MIX_BITS;
            w[j] ^= w[j] >> (2*MIX_BITS);
            w[j] &= MSB;
        }
        c = w[0] * multiplier + increment;
        d = w[1] + ((STATET)1<<EXIT_BITS) - 1;

        STATET c_sum = 0;
        STATET d_sum = 0;
        while (1) {
            int i;
            for(i=0; i<table_size; i++) {
                STATET c_add = table[i] * inv_multiplier;
                STATET d_add = table[i];
                if (c_sum + c_add < ((STATET)1<<EXIT_BITS) && d - c >= d_sum + d_add && d_sum + d_add > d_sum) {
                    c_sum += c_add;
                    d_sum += d_add;
//                     printf("Remaining: %"PRIX64"\n", d - c - d_sum);
                    break;
                }
            }
            if (i == table_size) {
                c += d_sum;
                if ((c & MSB) == w[1]) {
                    printf("\nCandidate  0x%"PRIX64" for %"PRIX64" %"PRIX64" %"PRIX64"", (uint64_t)c, (uint64_t)v[0], (uint64_t)v[1], (uint64_t)v[2]);
                    d = (w[1] - w[0] * multiplier - increment) * inv_multiplier;
                    pcg(&c);
                    d = pcg(&c);
                    if (d == v[2]) {
                        STATET seed = w[0] + c_sum - increment;
                        do {
                            seed = seed * inv_multiplier - increment;
                        } while(pos--);
                        printf("\nfound Seed = 0x%"PRIX64"\n", (uint64_t)seed);
                        exit(0);
                    }
                    break;
                } else {
                    //FAIL
                    break;
                }
            }
        }

    }
    return -1;
}


int main(int argc, char **argv) {
    STATET state;
    OUTT v[10];

    STATET d = 1, c;
    c = egcd(multiplier, -multiplier, &inv_multiplier, &d);
    inv_multiplier -= d;

    if(inv_multiplier * multiplier != 1) //ugly TODO
        inv_multiplier *= -1;
    printf("INV 0x%"PRIX64" 0x%"PRIX64" gcd=0x%"PRIX64"\n", (uint64_t)inv_multiplier, (uint64_t)inv_multiplier * (uint64_t)multiplier, (uint64_t)c);


    STATET seed = argc > 1 ? atoll(argv[1]) : 0x123456789ABC;

    printf("Seed = 0x%"PRIX64"\n", (uint64_t)seed);
    pcg_init(&state, seed);
    for(int i = 0; i<10; i++) {
        v[i] = pcg(&state);
        printf("PCG output %d = 0x%"PRIX64"\n", i, (uint64_t)v[i]);
    }

    for (int retry = 0;  retry < 9; retry++)
        breach(v+retry, retry);

    return 0;
}
