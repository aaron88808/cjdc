#include <stdio.h>
#include <unistr.h>
#include "cjdc.h"

int main(int ac, char **av) {
    uint8_t *utf8_str = NULL;

    printf("sizeof(u1_t) = %2d\n", sizeof(u1_t));
    printf("sizeof(u1_2) = %2d\n", sizeof(u2_t));
    printf("sizeof(u1_4) = %2d\n", sizeof(u4_t));
    return 0;
}
