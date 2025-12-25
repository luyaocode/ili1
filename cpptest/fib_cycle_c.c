#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

typedef unsigned long long UINT64;

UINT64 fibIter(UINT64 n) {
    if (n < 1||n>93) return 0;
    UINT64 a = 0, b = 1;
    while (--n) {
        UINT64 t = a + b;
        a = b;
        b = t;
    }
    return b;
}

const int MILLION_TIMES = 1000000;

int main(int argc, char* argv[]) {
    int t = 93;
    int u = 1 * MILLION_TIMES;
    UINT64 result = 0;
    volatile int flag = 0;
    if (argc == 2) {
        t = atoi(argv[1]);
    }
    if (argc == 3) {
    	t = atoi(argv[1]);
    	u = atoi(argv[2]) * MILLION_TIMES;
    }

    result = fibIter(t);

    volatile clock_t start = clock();
    __sync_synchronize();
    for (volatile int i = 0; i < u; ++i) {
        flag= fibIter((int)((((float)(i))/u)*40)+53)%10;
    }
    __sync_synchronize();
    volatile clock_t end = clock();

    double duration_s = (double)(end - start) / CLOCKS_PER_SEC;
    double duration_ns = duration_s * 1e9 / u;

    printf("number: %d\n", t);
    printf("times: %d\n", u);
    printf("result: %llu\n", result);
    printf("flag: %d\n", flag);
    printf("duration (s): %.6f\n", duration_s);
    printf("duration (ns): %.6f\n", duration_ns);

    return 0;
}
