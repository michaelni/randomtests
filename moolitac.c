#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define FFMIN(a, b) ((a) > (b) ? (b) : (a))
#define FFMAX(a, b) ((a) > (b) ? (a) : (b))

#define N 1024

static int check_length_factors(int best_length[N], int j){
    char factors[] = {2,3,5,7,11,13,17,19,23,29,31};
    int best = 0;

    for(int fi = 0; fi<sizeof(factors); fi++) {
        int f = factors[fi];
        if (j % f == 0)
            best = FFMAX(best, best_length[j / f]);
    }
    return best;
}

int main(int argc, char **argv) {
    int v, c, i;
    int p1 = atoi(argv[1]);
    c=v=getchar();
    v += 256*getchar();
    v = (short)v;

    short history[N];
    short history2[N];
    int histogram[N][N] = {0};
    int best_length[N] = {0};
    int rem[N] = {0};

    for(i=0; c!=EOF; i++) {
        c=v=getchar();
        v += 256*getchar();
        v = (short)v;
        int v2 = v ^ history[(i-p1)%N];

        for(int j = 1; j < N && j < i; j++) {
            if (v2 == history2[(i-j) & (N-1)]) {
                histogram[j][rem[j]]++;
            } else {
                if (histogram[j][rem[j]] > 4) {
                    if (histogram[j][rem[j]] > check_length_factors(best_length, j))
                        printf("Run of 0x%04X Terminated by 0x%04X (0x%04X/0x%04X), length %4d, period %4d, phase %4d at %9d\n",
                               history2[(i-j) & (N-1)] & 0xFFFF, v2 & 0xFFFF, v & 0xFFFF, (v^v2) & 0xFFFF,
                               histogram[j][rem[j]], j, rem[j], i
                               );
                    best_length[j] = FFMAX(best_length[j], histogram[j][rem[j]]);
                }
                histogram[j][rem[j]] = 0;
            }
        }
        for(int j = 1; j < N; j++) {
            rem[j] ++;
            if (rem[j] == j)
                rem[j] = 0;
        }
        history2[i%N] = v2;
        history [i%N] = v;
    }
    printf("\n");
    return 0;
}
