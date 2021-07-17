/* Modififed from  MUSL */
#if !defined(_STDBOOL_H) && !defined(bool)
#define _STDBOOL_H

#include <stdlib.h>

#define bool size_t
#define true 1
#define false (!true)

#endif
