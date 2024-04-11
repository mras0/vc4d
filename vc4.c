// TODO: Support multiple contexts (need to arbitrate access...)

#include "vc4.h"
#include <stdint.h>
#include <stddef.h>

#include <exec/types.h>
#include <proto/exec.h>
#include "devicetree.h"
#include "vc4d.h"
#include "v3d_regs.h"

#define BUS_TO_PHYS(x) ((x)&~0xC0000000)

#define MAX_COMMAND_LENGTH 32

static APTR DeviceTreeBase;
static ULONG MailBox;
static uint32_t MBReqStorage[MAX_COMMAND_LENGTH + 4];
static uint32_t* MBReq;
static uint32_t BytesAllocated;

#define PR_STAT(name) LOG_DEBUG("%-20s 0x%lx\n", #name, LE32(V3D_ ## name))

#define QPURQL_MASK     (0x1f)
#define QPURQERR_MASK   (1 << 7)
#define QPURQCM_MASK    (0xFF << 8)
#define QPURQCC_MASK    (0xFF << 16)
#define QPURQL_RESET    (1)
#define QPURQERR_RESET  (QPURQERR_MASK)
#define QPURQCM_RESET   (1 << 8)
#define QPURQCC_RESET   (1 << 16)

#define MBOX_READ   LE32(*((const volatile uint32_t*)(MailBox + 0x00)))
#define MBOX_STATUS LE32(*((const volatile uint32_t*)(MailBox + 0x18)))
#define MBOX_WRITE  *((volatile uint32_t*)(MailBox + 0x20))

#define MBOX_CHANNEL 8

#define MBOX_TX_FULL (1UL << 31)
#define MBOX_RX_EMPTY (1UL << 30)
#define MBOX_CHANMASK 0xF

static void ZeroMem(void* d, size_t sz)
{
    uint8_t* dst = d;
    while (sz--)
        *dst++ = 0;
}

static uint32_t mbox_recv(void)
{
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

static void mbox_send(uint32_t* req)
{
    while (MBOX_STATUS & MBOX_TX_FULL)
        asm volatile ("nop");
    asm volatile ("nop");
    MBOX_WRITE = LE32(((uint32_t)req & ~MBOX_CHANMASK) | MBOX_CHANNEL);
}

static int mbox_transaction(VC4D* vc4d, uint32_t* req)
{
    SYSBASE;
    ULONG len = LE32(*req) * 4;
    CachePreDMA(req, &len, 0);
    Forbid();
    mbox_send(req);
    mbox_recv();
    Permit();
    CachePostDMA(req, &len, 0);
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

static unsigned qpu_enable(VC4D* vc4d, unsigned enable)
{
    int i=0;
    uint32_t* p = MBReq;

    p[i++] = 0; // size
    p[i++] = 0; // process request

    p[i++] = LE32(0x30012); // (the tag id)
    p[i++] = LE32(4); // (size of the buffer)
    p[i++] = LE32(4); // (size of the data)
    p[i++] = LE32(enable);

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
                        LOG_DEBUG("VC4 initialzed Mailbox = 0x%lx\n", MailBox);
                        LOG_DEBUG("QPU ident: %08lx %08lx %08lx\n", LE32(V3D_IDENT0), LE32(V3D_IDENT1), LE32(V3D_IDENT2));
                        ret = qpu_enable(vc4d, 1);
                        LOG_DEBUG("QPU enable result: %ld\n", ret);
                        LOG_DEBUG("QPU ident: %08lx %08lx %08lx\n", LE32(V3D_IDENT0), LE32(V3D_IDENT1), LE32(V3D_IDENT2));
                        ret = 0; // XXX: Ignore enable result for now?
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
    if (BytesAllocated) {
        LOG_ERROR("Still %lu bytes allocated!\n", BytesAllocated);
    }
    int ret = qpu_enable(vc4d, 0);
    LOG_DEBUG("qpu_enable(0) %ld\n", ret);
}


void vc4_mem_free(VC4D* vc4d, vc4_mem* m)
{
	if (m->handle) {
        BytesAllocated -= m->size;
        LOG_DEBUG("Freeing %lu bytes, busaddr = %lx phys = %lx\n", m->size, m->busaddr, BUS_TO_PHYS(m->busaddr));
		if (m->busaddr)
			mem_unlock(vc4d, m->handle);
		mem_free(vc4d, m->handle);
	}
	ZeroMem(m, sizeof(*m));
}

int vc4_mem_alloc(VC4D* vc4d, vc4_mem* m, unsigned size)
{
	ZeroMem(m, sizeof(*m));
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
	LOG_DEBUG("Allocated %lu bytes, busaddr = %lx phys = %lx\n", size, m->busaddr, BUS_TO_PHYS(m->busaddr));
    BytesAllocated += m->size;
	return 0;
}

static unsigned wait_num;

int vc4_wait_qpu(struct VC4D* vc4d)
{
    (void)vc4d;
    if (wait_num) {
        while (((LE32(V3D_SRQCS)>>16) & 0xff) != wait_num)
            ;
        wait_num = 0;
    }
    return 0;
}

int vc4_run_qpu(struct VC4D* vc4d, uint32_t num_qpus, unsigned code_bus, unsigned uniform_bus)
{
    (void)vc4d;
    // This should never happen (uniform memory overwritten anyway)
    //int ret = vc4_wait_qpu(vc4d);
    //if (ret)
    //   return ret;

    V3D_DBCFG  = LE32(0);       // Disallow IRQ
    V3D_DBQITE = LE32(0);       // Disable IRQ
    V3D_DBQITC = LE32(-1);      // Resets IRQ flags

    V3D_L2CACTL = LE32(1<<2);   // Clear L2 cache
    V3D_SLCACTL = LE32(-1);     // Clear other caches

    V3D_SRQCS = LE32((1<<7) | (1<<8) | (1<<16)); // Reset error bit and counts

    for (uint32_t q=0; q<num_qpus; q++) {
        V3D_SRQUA = LE32(uniform_bus);
        V3D_SRQPC = LE32(code_bus);
    }

    wait_num = num_qpus;

    return 0;
}

