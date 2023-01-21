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
        p += i>2 && (v^v>>6)&3;

        v>>=2;
    }
    printf("mooltiness: %8.2f expected: < 1 in 68%% cases, < 2 in 95%%, < 3 in 99.7%%\n",
           fabs(4*p-3*i+9) / sqrt(3*i-9));
    return 0;
}
