#include <stdio.h>
#include <math.h>
#include <string.h>

double log_binomial(int N, int K) {
    return lgamma(N+1) - lgamma(K+1) - lgamma(N-K+1);
}

long double sum_binomial(int N, int K) {
    long double sum = 0;
    for(int k = 0; k <= K; k++)
        sum += expl(log_binomial(N, k) - N*log(2));

    return sum;
}

int main() {
    int v, c;
    int histogram[3][2] = {0};

    c=v=getchar();
    for(long long i=0; c!=EOF; i++) {
        int X = (v&1) ^ ((v>>1)&1);
        histogram[i%3][X]++;

        v>>=2;
        if (!(i&3))
            v |= (c = getchar())<<6;

        if (i%10000 == 0) {
            int clear = 0;
            for(int j=0; j<3; j++){
                int K = histogram[j][0];
                int N = histogram[j][0] + histogram[j][1];
                long double d = sum_binomial(N, N-K) * 100;
                if (d > 0.01)
                    continue;
                if (!clear)
                        printf("%9Ld ", i);
                printf("channel:%d %5.1f probability this or worse occures here: %8.2LG%% ", j, K * 200.0 / N, d);
                clear = 1;
            }
            if (clear)
                printf("\n");
            memset(histogram, 0, sizeof(histogram));
        }
    }

    return 0;
}
