#include <stdio.h>
#include <math.h>

int main() {
    int v, c;
    long long i,p;
    c=v=getchar();
    for(p=i=0; c!=EOF; i++) {
        p += !((v^v>>6)&3);

        v>>=2;
        if (!(i&3))
            v |= (c = getchar())<<6;
    }
    double d = fabs(4*p-i) / sqrt(3*i);
    printf("mooltiness: %8.2f, this or larger is expected %.2LG %% of the time in random data.\n",
           d, erfcl(d/sqrt(2))*100);
    return 0;
}
