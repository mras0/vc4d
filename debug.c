#include "vc4d.h"
#include <proto/exec.h>
#include <proto/dos.h>

#define SERDATR *((volatile uint16_t*)0xDFF018)
#define SERDAT  *((volatile uint16_t*)0xDFF030)
#define SERPER  *((volatile uint16_t*)0xDFF032)

static void ser_putch(UBYTE ch __asm("d0"), void* context __asm("a3"))
{
    (void)context;
    // Wait for TBE
    while (!(SERDATR & (1<<13)))
            ;
    SERDAT = 0x100 | ch;
}

void log_debug(VC4D* vc4d, const char* fmt, ...)
{
#if 0
    DOSBASE;
    BPTR out = Output();
    if (!out)
        return;
    VFPrintf(out, fmt, (LONG*)&fmt + 1);
#else
    SYSBASE;
    SERPER = 30;
    RawDoFmt(fmt, (LONG*)&fmt + 1, (APTR)&ser_putch, NULL);
#endif
}
