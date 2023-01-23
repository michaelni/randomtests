#include <stdio.h>
#include <math.h>

int main() {
    int v, c;
    long long i,p,k;
    int score[96] = {0};
    int cycle[96] = {0};
    c=v=getchar();
    for(p=i=k=0; c!=EOF; i++) {
        int X = !((v^v>>6)&3);
        int x = i % 96;

        int max = 1;
        for (int j = 0; j<96; j++)
            if (score[j] > max)
                max = score[j];

        if (score[x] >= max) {
            k++;
            p += X;
        }
        if (!X && cycle[x] > 16)
            printf("Run %4d at phase %2d\n", cycle[x], x);
        cycle[x] = X*cycle[x] + X;
        score[x] = cycle[x] + 4*((x&31) == 31);

        v>>=2;
        if (!(i&3))
            v |= (c = getchar())<<6;
    }
    double d = fabs(4*p-k) / sqrt(3*k);
    printf("moolticycles: %8.2f, this or larger is expected %.2LG %% of the time in random data.\n",
           d, erfcl(d/sqrt(2))*100);

    return 0;
}
