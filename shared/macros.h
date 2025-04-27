#pragma once

#include <stdio.h>

#ifdef DEBUG_BUILD
    #define PRINTF(...) printf(__VA_ARGS__)
    #define FPRINTF(...) fprintf(__VA_ARGS__)
#else
    #define PRINTF(...)
    #define FPRINTF(...)
#endif