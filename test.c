#include <exec/execbase.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdio.h>
#include <proto/Warp3D.h>

struct DosBase* DosBase;
struct Library *Warp3DBase;

#define TRACE() printf("Trace %s:%d %s\n", __FILE__, __LINE__, __func__)

void PrintDriverInfo(void)
{
	ULONG res;

    TRACE();
	W3D_Driver **drivers = W3D_GetDrivers();
    TRACE();

	if (*drivers == NULL) {
		printf("Panic: No Drivers found\n");
		return;
	}
	printf("Available drivers:\n");
	while (*drivers) {
		printf("%s\n\tSupports format 0x%X\n\t",
			drivers[0]->name, drivers[0]->formats);
		if (drivers[0]->swdriver) printf("CPU Driver\n");
		else                      printf("Hardware Driver\n");

		printf("\tPrimitives supported:\n\t");
		res = W3D_QueryDriver(drivers[0], W3D_Q_DRAW_POINT, W3D_FMT_R5G5B5);
		if (res != W3D_NOT_SUPPORTED) printf("[POINT] ");

		res = W3D_QueryDriver(drivers[0], W3D_Q_DRAW_LINE, W3D_FMT_R5G5B5);
		if (res != W3D_NOT_SUPPORTED) printf("[LINE] ");

		res = W3D_QueryDriver(drivers[0], W3D_Q_DRAW_TRIANGLE, W3D_FMT_R5G5B5);
		if (res != W3D_NOT_SUPPORTED) printf("[TRIANGLE] ");

		printf("\n\tFiltering:\n\t");
		res = W3D_QueryDriver(drivers[0], W3D_Q_BILINEARFILTER, W3D_FMT_R5G5B5);
		if (res != W3D_NOT_SUPPORTED) printf("[BI-FILTER] ");

		res = W3D_QueryDriver(drivers[0], W3D_Q_MMFILTER, W3D_FMT_R5G5B5);
		if (res != W3D_NOT_SUPPORTED) printf("[MM-FILTER] ");
		printf("\n");

		drivers++;
	}
	printf("\n\n");
}

int main(void)
{
    DosBase = (struct DosBase*)OpenLibrary("dos.library", 0);
    if (!DosBase)
        return -1;
    printf("%s version %d.%d\n", (const char*)Warp3DBase->lib_IdString, Warp3DBase->lib_Version, Warp3DBase->lib_Revision);

    TRACE();
    printf("%04X %08X\n", *(UWORD*)((char*)Warp3DBase - 54), *(ULONG*)((char*)Warp3DBase - 54 + 2));
	ULONG flags = W3D_CheckDriver();
    TRACE();
	if (flags & W3D_DRIVER_3DHW) printf("Hardware driver available\n");
	if (flags & W3D_DRIVER_CPU)  printf("Software driver available\n");
	if (flags & W3D_DRIVER_UNAVAILABLE) {
		printf("PANIC: no driver available!!!\n");
		goto panic;
	}
    PrintDriverInfo();

panic:
    if (DosBase)
        CloseLibrary((struct Library*)DosBase);
    return 0;
}
