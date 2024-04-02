#include "vc4.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <exec/types.h>
#include <proto/exec.h>
#include "devicetree.h"
#include "vc4d.h"

#define BUS_TO_PHYS(x) ((x)&~0xC0000000)

#define MAX_COMMAND_LENGTH 32

static APTR DeviceTreeBase;
static ULONG MailBox;
static uint32_t MBReqStorage[MAX_COMMAND_LENGTH + 4];
static uint32_t* MBReq;

#define LE32(x) (__builtin_bswap32(x))

#define MBOX_READ   LE32(*((const volatile uint32_t*)(MailBox + 0x00)))
#define MBOX_STATUS LE32(*((const volatile uint32_t*)(MailBox + 0x18)))
#define MBOX_WRITE  *((volatile uint32_t*)(MailBox + 0x20))

#define MBOX_CHANNEL 8

#define MBOX_TX_FULL (1UL << 31)
#define MBOX_RX_EMPTY (1UL << 30)
#define MBOX_CHANMASK 0xF

// TODO: We need "mailbox.resource" or similar

static uint32_t mbox_recv(VC4D* vc4d)
{
    (void)vc4d;
    uint32_t rsp;
    do {
        while (MBOX_STATUS & MBOX_RX_EMPTY)
            asm volatile("nop");

        asm volatile("nop");
        rsp = MBOX_READ;
        asm volatile("nop");
    } while ((rsp & MBOX_CHANMASK) != MBOX_CHANNEL);
    asm volatile ("nop" ::: "memory");
    return rsp & ~MBOX_CHANMASK;
}

static void mbox_send(VC4D* vc4d, uint32_t* req)
{
    SYSBASE;
    CacheClearE(req, __builtin_bswap32(*req), CACRF_ClearD);
    while (MBOX_STATUS & MBOX_TX_FULL)
        asm volatile ("nop");
    asm volatile ("nop");
    MBOX_WRITE = LE32(((uint32_t)req & ~MBOX_CHANMASK) | MBOX_CHANNEL);
}

static int mbox_transaction(VC4D* vc4d, uint32_t* req)
{
    mbox_send(vc4d, req);
    mbox_recv(vc4d);
    if (LE32(req[1]) == 0x80000000)
        return 0;
    //fprintf(stderr, "Mailbox transaction failed for command %08x: %08x\n", LE32(req[2]), LE32(req[1]));
    return LE32(req[1]);
}

static unsigned mem_alloc(VC4D* vc4d, unsigned size, unsigned align, unsigned flags)
{
   int i=0;
   uint32_t* p = MBReq;
   p[i++] = 0; // size
   p[i++] = 0; // process request

   p[i++] = LE32(0x3000c); // (the tag id)
   p[i++] = LE32(12); // (size of the buffer)
   p[i++] = LE32(12); // (size of the data)
   p[i++] = LE32(size); // (num bytes? or pages?)
   p[i++] = LE32(align); // (alignment)
   p[i++] = LE32(flags); // (MEM_FLAG_L1_NONALLOCATING)

   p[i++] = 0; // end tag
   p[0] = LE32(i*sizeof *p); // actual size

   if (mbox_transaction(vc4d, p))
       return 0;
   return LE32(p[5]);
}

static unsigned mem_free(VC4D* vc4d, unsigned handle)
{
   int i=0;
   uint32_t* p = MBReq;
   p[i++] = 0; // size
   p[i++] = 0; // process request

   p[i++] = LE32(0x3000f); // (the tag id)
   p[i++] = LE32(4); // (size of the buffer)
   p[i++] = LE32(4); // (size of the data)
   p[i++] = LE32(handle);

   p[i++] = 0; // end tag
   p[0] = LE32(i*sizeof *p); // actual size

   if (mbox_transaction(vc4d, p))
       return -1;
   return LE32(p[5]);
}

static unsigned mem_lock(VC4D* vc4d, unsigned handle)
{
   int i=0;
   uint32_t* p = MBReq;
   p[i++] = 0; // size
   p[i++] = 0; // process request
   p[i++] = LE32(0x3000d); // (the tag id)
   p[i++] = LE32(4); // (size of the buffer)
   p[i++] = LE32(4); // (size of the data)
   p[i++] = LE32(handle);

   p[i++] = 0; // end tag
   p[0] = LE32(i*sizeof *p); // actual size

   if (mbox_transaction(vc4d, p))
       return 0;
   return LE32(p[5]);
}

static unsigned mem_unlock(VC4D* vc4d, unsigned handle)
{
   int i=0;
   uint32_t* p = MBReq;
   p[i++] = 0; // size
   p[i++] = 0; // process request

   p[i++] = LE32(0x3000e); // (the tag id)
   p[i++] = LE32(4); // (size of the buffer)
   p[i++] = LE32(4); // (size of the data)
   p[i++] = LE32(handle);

   p[i++] = 0; // end tag
   p[0] = LE32(i*sizeof *p); // actual size

   if (mbox_transaction(vc4d, p))
       return -1;
   return LE32(p[5]);
}

/*
    Some properties, like e.g. #size-cells, are not always available in a key, but in that case the properties
    should be searched for in the parent. The process repeats recursively until either root key is found
    or the property is found, whichever occurs first
*/
static CONST_APTR GetPropValueRecursive(APTR key, CONST_STRPTR property)
{
    do {
        /* Find the property first */
        APTR prop = DT_FindProperty(key, property);

        if (prop)
        {
            /* If property is found, get its value and exit */
            return DT_GetPropValue(prop);
        }
        
        /* Property was not found, go to the parent and repeat */
        key = DT_GetParent(key);
    } while (key);

    return NULL;
}


int vc4_init(VC4D* vc4d)
{
    int ret = -1;
    SYSBASE;
    MBReq = (uint32_t*)(((uintptr_t)MBReqStorage + 15) & ~15);
    if ((DeviceTreeBase = OpenResource("devicetree.resource")) != NULL) {
        APTR key = DT_OpenKey("/aliases");
        if (key) {
            const char* mbox_alias = DT_GetPropValue(DT_FindProperty(key, "mailbox"));
            DT_CloseKey(key);
            if (mbox_alias) {
                key = DT_OpenKey(mbox_alias);
                if (key) {
					//ULONG size_cells = 1;
					ULONG address_cells = 1;
					
					//CONST ULONG *siz = GetPropValueRecursive(key, "#size_cells");
					CONST ULONG *adr = GetPropValueRecursive(key, "#address-cells");
					CONST ULONG *reg = DT_GetPropValue(DT_FindProperty(key, "reg"));
					
					//if (siz != NULL) size_cells = *siz;
					if (adr != NULL) address_cells = *adr;
					MailBox = reg[(1 * address_cells) - 1];
                    DT_CloseKey(key);

                    /* Open /soc key and learn about VC4 to CPU mapping. Use it to adjust the addresses obtained above */
                    key = DT_OpenKey("/soc");
                    if (key) {
                        address_cells = 1;
                        ULONG cpu_address_cells = 1;

                        //const ULONG * siz = GetPropValueRecursive(key, "#size_cells");
                        const ULONG * addr = GetPropValueRecursive(key, "#address-cells");
                        const ULONG * cpu_addr = DT_GetPropValue(DT_FindProperty(DT_OpenKey("/"), "#address-cells"));

                        //if (siz != NULL) size_cells = *siz;
                        if (addr != NULL) address_cells = *addr;
                        if (cpu_addr != NULL) cpu_address_cells = *cpu_addr;

                        const ULONG *reg = DT_GetPropValue(DT_FindProperty(key, "ranges"));

                        ULONG phys_vc4 = reg[address_cells - 1];
                        ULONG phys_cpu = reg[address_cells + cpu_address_cells - 1];

                        MailBox = ((ULONG)MailBox - phys_vc4 + phys_cpu);

                        DT_CloseKey(key);
                        ret = 0;
                    } else {
                        LOG_ERROR("Could not open /soc\n");
                        MailBox = 0;
                        ret = -1;
                    }
                } else {
                    LOG_ERROR("Could not open mail box alias\n");
                    ret = -2;
                }
            } else {
                LOG_ERROR("Not mailbox alias\n");
                ret = -3;
            }
        } else {
            LOG_ERROR("Could not open aliases\n");
            ret = -4;
        }
    } else {
        LOG_ERROR("Could not open devicetree.resource\n");
        ret = -5;
    }
    return ret;
}

void vc4_free(VC4D* vc4d)
{
    LOG_DEBUG("vc4_free\n");
}


void vc4_mem_free(VC4D* vc4d, vc4_mem* m)
{
	LOG_DEBUG("Freeing %u bytes, busaddr = %x phys = %x\n", m->size, m->busaddr, BUS_TO_PHYS(m->busaddr));
	if (m->handle) {
		if (m->busaddr)
			mem_unlock(vc4d, m->handle);
		mem_free(vc4d, m->handle);
	}
	memset(m, 0, sizeof(*m));
}

int vc4_mem_alloc(VC4D* vc4d, vc4_mem* m, unsigned size)
{
	memset(m, 0, sizeof(*m));
    const uint32_t align = 16;
	m->size = (size + align - 1) & ~(align - 1);
	m->handle = mem_alloc(vc4d, m->size, 8, MEM_FLAG_DIRECT);
	if (!m->handle) {
		LOG_ERROR("Failed to alloc mem\n");
		return -1;
	}
	m->busaddr = mem_lock(vc4d, m->handle);
	if (!m->busaddr) {
		LOG_ERROR("Failed to lock memory\n");
		vc4_mem_free(vc4d, m);
		return -2;
	}
	m->hostptr = (void*)BUS_TO_PHYS(m->busaddr);
	if (!m->hostptr) {
		LOG_ERROR("Failed to map memory\n");
		vc4_mem_free(vc4d, m);
		return -3;
	}
	LOG_DEBUG("Allocated %u bytes, busaddr = %x phys = %x\n", size, m->busaddr, BUS_TO_PHYS(m->busaddr));
	return 0;
}
