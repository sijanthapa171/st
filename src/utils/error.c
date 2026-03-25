#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "utils.h"

void die(const char *s) {
    if (write(STDOUT_FILENO, "\x1b[2J", 4) == -1) {}
    if (write(STDOUT_FILENO, "\x1b[H", 3) == -1) {}
    
    perror(s);
    exit(1);
}
