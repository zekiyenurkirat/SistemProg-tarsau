#include <stdio.h>
int main(void)
{
    unsigned long m = 0;
    int r = sscanf("0777", "%lo", &m);
    printf("r=%d m=%lu\n", r, m);
    return 0;
}
