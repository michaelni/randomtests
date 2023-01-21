#include <stdio.h>
#include <math.h>

int main() {
    int v, c;
    long long i,p;
    for(v=i=p=0; 1; i++) {
        if (!(i&3)) {
            c = getchar();
            if (c == EOF)
                break;
            v |= c<<6;
        }
        if (i>2)
            p += (v&195)%65 ? -1 : 3;
        v>>=2;
    }
    printf("mooltiness: %8.2f expected: < 1 in 68%% cases, < 2 in 95%%, < 3 in 99.7%%\n",
           fabs(p) / sqrt(3*i-9));
    return 0;
}
