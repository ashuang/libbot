#include "globals.h"

static lcm_t *global_lcm = NULL;

lcm_t * 
globals_get_lcm (void)
{
    if(!global_lcm) 
        global_lcm = lcm_create (NULL);

    return global_lcm;
}
