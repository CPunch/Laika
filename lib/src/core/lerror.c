#include "core/lerror.h"

jmp_buf eLaika_errStack[LAIKA_MAXERRORS];
int eLaika_errIndx = -1;