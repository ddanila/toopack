#include "../../src/dos.h"

int main(void) {
    unsigned int seg = dos_alloc(4);
    if (seg == 0) {
        dos_puts("alloc fail\r\n$");
        return 1;
    }
    dos_puts("alloc ok\r\n$");
    return 0;
}
