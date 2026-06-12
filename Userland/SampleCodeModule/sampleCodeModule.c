#include <stdint.h>
#include "usrlib.h"
#include "shell.h"

int main(void) {
    clearScreen();
    shell_init();
    return 0;
}
