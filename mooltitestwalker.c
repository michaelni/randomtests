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
    printf("mooltiness: %8.2f expected: < 1 in 68%% cases, < 2 in 95%%, < 3 in 99.7%%\n",
           fabs(4*p-i) / sqrt(3*i));
    return 0;
}
