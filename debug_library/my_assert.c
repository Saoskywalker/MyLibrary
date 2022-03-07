#include "my_assert.h"

void _assert_failed(char* file, unsigned int line)
{
    printf("local: %s, %d", file, line);
    while (1);
}

// int _debug_printf(const char* format, ...)
// {

// }
