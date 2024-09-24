#include "time_mon.h"
#include <time.h>

uint32_t GetCurrentTimestamp(void)
{
    /* TODO: RTR - Implement a platform-specific method to obtain a timestamp */
    uint32_t ticks = clock();
    uint32_t seconds = ticks / CLOCKS_PER_SEC;

    return seconds;
}