#include <stdio.h>
#include <string.h>
#include <math.h>

int isAccel;

static inline int get() {
    if (isAccel) {
        int a = getchar(); getchar();
        int b = getchar(); getchar();
        int c = getchar(); getchar();
        int d = getchar(); getchar();
        if (d == EOF)
            return EOF;
        return (a&3) + 4*(b&3) + 16*(c&3) + 64*(d&3);
    } else {
        return getchar();
    }
}

int main(int argc, char **argv) {
    int v, c;
    long long i,p,k;
    int score[96] = {0};
    score[31] = 1;
    int cycle[96] = {0};
    int max = 1;
    isAccel = argc > 1 && !strcmp(argv[1], "-a");
    c=v=get();
    for(p=i=k=0; c!=EOF; i++) {
        int X = !((v^v>>6)&3);
        int x = i % 96;

        if (score[x] >= max) {
            k++;
            p += X;
        }
        if (!X && cycle[x] > 16)
            printf("Run %4d at phase %2d\n", cycle[x], x);
        cycle[x] = X*cycle[x] + X;
        int oldscore = score[x];
        score[x] = cycle[x] + 4*((x&31) == 31);
        if (score[x] >= max)
            max = score[x];
        else if (oldscore == max) {
            max = 1;
            for (int j = 0; j<96; j++)
                if (score[j] > max)
                    max = score[j];
        }

        v>>=2;
        if (!(i&3))
            v |= (c = get())<<6;
    }
    double d = fabs(4*p-k) / sqrt(3*k);
    printf("moolticycles: %8.2f, this or larger is expected %.2LG %% of the time in random data.\n",
           d, erfcl(d/sqrt(2))*100);

    return 0;
}
