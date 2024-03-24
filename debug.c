#include "vc4d.h"
#include <proto/dos.h>

void log_debug(VC4D* vc4d, const char* fmt, ...)
{
    DOSBASE;
    BPTR out = Output();
    if (!out)
        return;
    VFPrintf(out, fmt, (LONG*)&fmt + 1);
}
