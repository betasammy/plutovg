#include <stdio.h>

#if defined(_MSC_VER)
#include <intrin.h>
static inline int clz(unsigned int x) {
    unsigned long r = 0;
    if (_BitScanReverse(&r, x))
        return 31 - r;
    return 32;
}
#define PVG_FT_MSB(x)  (31 - clz(x))
#elif defined(__GNUC__)
#define PVG_FT_MSB(x)  (31 - __builtin_clz(x))
#else
static inline int clz(unsigned int x) {
    for (int i = 31; i >= 0; i--)
    {
        if (x >> i)
        {
            return 31 - i;
        }
    }

    return 32;
}
#define PVG_FT_MSB(x)  (31 - clz(x))
#endif

int main(void)
{
#if defined(_WIN32)
    printf("_WIN32\n");
#endif
#if defined(_MSC_VER)
    printf("_MSC_VER\n");
#endif
#if defined(__APPLE__)
    printf("__APPLE__\n");
#endif
#if defined(__clang__)
    printf("__clang__\n");
#endif
#if defined(__GNUC__)
    printf("__GNUC__\n");
#endif
    return 0;
}
