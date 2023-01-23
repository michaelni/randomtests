#include <stdio.h>
#include <math.h>

int main() {
    int v, c;
    long long i,p,k;
    int cycle[96] = {0};
    c=v=getchar();
    for(p=i=k=0; c!=EOF; i++) {
        int X = !((v^v>>6)&3);
        int x = i % 96;

        int max = 2;
        for (int j = 0; j<96; j++)
            if (cycle[j] > max)
                max = cycle[j];

        if (cycle[x] >= max) {
            k++;
            p += X;
        }
        cycle[x] = cycle[x]*X + X;

        v>>=2;
        if (!(i&3))
            v |= (c = getchar())<<6;
    }
    double d = fabs(4*p-k) / sqrt(3*k);
    printf("moolticycles: %8.2f, this or larger is expected %.2LG %% of the time in random data.\n",
           d, erfcl(d/sqrt(2))*100);

    return 0;
}
