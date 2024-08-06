#include "log.h"

static LogLevel LOG_LEVEL_FILTER = LOG_DEBUG;

LogLevel get_loglevel_filter(void) 
{ 
    return LOG_LEVEL_FILTER; 
}

void set_loglevel_filter(LogLevel level)
{ 
    LOG_LEVEL_FILTER = level;
}