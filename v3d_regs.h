#ifndef V3D_REGS_H
#define V3D_REGS_H

#define V3D_BASE (0xF2000000 + 0xC00000)

#define V3D_IDENT0	*(volatile uint32_t*)(V3D_BASE+0x00000)	// V3D Identification 0 (V3D block identity)
#define V3D_IDENT1	*(volatile uint32_t*)(V3D_BASE+0x00004)	// V3D Identification 1 (V3D Configuration A)
#define V3D_IDENT2	*(volatile uint32_t*)(V3D_BASE+0x00008)	// V3D Identification 1 (V3D Configuration B)
#define V3D_SCRATCH	*(volatile uint32_t*)(V3D_BASE+0x00010)	// Scratch Register
#define V3D_L2CACTL	*(volatile uint32_t*)(V3D_BASE+0x00020)	// L2 Cache Control
#define V3D_SLCACTL	*(volatile uint32_t*)(V3D_BASE+0x00024)	// Slices Cache Control
#define V3D_INTCTL	*(volatile uint32_t*)(V3D_BASE+0x00030)	// Interrupt Control
#define V3D_INTENA	*(volatile uint32_t*)(V3D_BASE+0x00034)	// Interrupt Enables
#define V3D_INTDIS	*(volatile uint32_t*)(V3D_BASE+0x00038)	// Interrupt Disables
#define V3D_CT0CS	*(volatile uint32_t*)(V3D_BASE+0x00100)	// Control List Executor Thread 0 Control and Status.
#define V3D_CT1CS	*(volatile uint32_t*)(V3D_BASE+0x00104)	// Control List Executor Thread 1 Control and Status.
#define V3D_CT0EA	*(volatile uint32_t*)(V3D_BASE+0x00108)	// Control List Executor Thread 0 End Address.
#define V3D_CT1EA	*(volatile uint32_t*)(V3D_BASE+0x0010c)	// Control List Executor Thread 1 End Address.
#define V3D_CT0CA	*(volatile uint32_t*)(V3D_BASE+0x00110)	// Control List Executor Thread 0 Current Address.
#define V3D_CT1CA	*(volatile uint32_t*)(V3D_BASE+0x00114)	// Control List Executor Thread 1 Current Address.
#define V3D_CT00RA0	*(volatile uint32_t*)(V3D_BASE+0x00118)	// Control List Executor Thread 0 Return Address.
#define V3D_CT01RA0	*(volatile uint32_t*)(V3D_BASE+0x0011c)	// Control List Executor Thread 1 Return Address.
#define V3D_CT0LC	*(volatile uint32_t*)(V3D_BASE+0x00120)	// Control List Executor Thread 0 List Counter
#define V3D_CT1LC	*(volatile uint32_t*)(V3D_BASE+0x00124)	// Control List Executor Thread 1 List Counter
#define V3D_CT0PC	*(volatile uint32_t*)(V3D_BASE+0x00128)	// Control List Executor Thread 0 Primitive List Counter
#define V3D_CT1PC	*(volatile uint32_t*)(V3D_BASE+0x0012c)	// Control List Executor Thread 1 Primitive List Counter
#define V3D_PCS	    *(volatile uint32_t*)(V3D_BASE+0x00130)	// V3D Pipeline Control and Status
#define V3D_BFC	    *(volatile uint32_t*)(V3D_BASE+0x00134)	// Binning Mode Flush Count
#define V3D_RFC	    *(volatile uint32_t*)(V3D_BASE+0x00138)	// Rendering Mode Frame Count
#define V3D_BPCA	*(volatile uint32_t*)(V3D_BASE+0x00300)	// Current Address of Binning Memory Pool
#define V3D_BPCS	*(volatile uint32_t*)(V3D_BASE+0x00304)	// Remaining Size of Binning Memory Pool
#define V3D_BPOA	*(volatile uint32_t*)(V3D_BASE+0x00308)	// Address of Overspill Binning Memory Block
#define V3D_BPOS	*(volatile uint32_t*)(V3D_BASE+0x0030c)	// Size of Overspill Binning Memory Block
#define V3D_BXCF	*(volatile uint32_t*)(V3D_BASE+0x00310)	// Binner Debug
#define V3D_SQRSV0	*(volatile uint32_t*)(V3D_BASE+0x00410)	// Reserve QPUs 0-7
#define V3D_SQRSV1	*(volatile uint32_t*)(V3D_BASE+0x00414)	// Reserve QPUs 8-15
#define V3D_SQCNTL	*(volatile uint32_t*)(V3D_BASE+0x00418)	// QPU Scheduler Control
#define V3D_SRQPC	*(volatile uint32_t*)(V3D_BASE+0x00430)	// QPU User Program Request Program Address
#define V3D_SRQUA	*(volatile uint32_t*)(V3D_BASE+0x00434)	// QPU User Program Request Uniforms Address
#define V3D_SRQUL	*(volatile uint32_t*)(V3D_BASE+0x00438)	// QPU User Program Request Uniforms Length
#define V3D_SRQCS	*(volatile uint32_t*)(V3D_BASE+0x0043c)	// QPU User Program Request Control and Status
#define V3D_VPACNTL	*(volatile uint32_t*)(V3D_BASE+0x00500)	// VPM Allocator Control
#define V3D_VPMBASE	*(volatile uint32_t*)(V3D_BASE+0x00504)	// VPM base (user) memory reservation
#define V3D_PCTRC	*(volatile uint32_t*)(V3D_BASE+0x00670)	// Performance Counter Clear
#define V3D_PCTRE	*(volatile uint32_t*)(V3D_BASE+0x00674)	// Performance Counter Enables
#define V3D_PCTR0	*(volatile uint32_t*)(V3D_BASE+0x00680)	// Performance Counter Count 0
#define V3D_PCTRS0	*(volatile uint32_t*)(V3D_BASE+0x00684)	// Performance Counter Mapping 0
#define V3D_PCTR1	*(volatile uint32_t*)(V3D_BASE+0x00688)	// Performance Counter Count 1
#define V3D_PCTRS1	*(volatile uint32_t*)(V3D_BASE+0x0068c)	// Performance Counter Mapping 1
#define V3D_PCTR2	*(volatile uint32_t*)(V3D_BASE+0x00690)	// Performance Counter Count 2
#define V3D_PCTRS2	*(volatile uint32_t*)(V3D_BASE+0x00694)	// Performance Counter Mapping 2
#define V3D_PCTR3	*(volatile uint32_t*)(V3D_BASE+0x00698)	// Performance Counter Count 3
#define V3D_PCTRS3	*(volatile uint32_t*)(V3D_BASE+0x0069c)	// Performance Counter Mapping 3
#define V3D_PCTR4	*(volatile uint32_t*)(V3D_BASE+0x006a0)	// Performance Counter Count 4
#define V3D_PCTRS4	*(volatile uint32_t*)(V3D_BASE+0x006a4)	// Performance Counter Mapping 4
#define V3D_PCTR5	*(volatile uint32_t*)(V3D_BASE+0x006a8)	// Performance Counter Count 5
#define V3D_PCTRS5	*(volatile uint32_t*)(V3D_BASE+0x006ac)	// Performance Counter Mapping 5
#define V3D_PCTR6	*(volatile uint32_t*)(V3D_BASE+0x006b0)	// Performance Counter Count 6
#define V3D_PCTRS6	*(volatile uint32_t*)(V3D_BASE+0x006b4)	// Performance Counter Mapping 6
#define V3D_PCTR7	*(volatile uint32_t*)(V3D_BASE+0x006b8)	// Performance Counter Count 7
#define V3D_PCTRS7	*(volatile uint32_t*)(V3D_BASE+0x006bc)	// Performance Counter Mapping 7
#define V3D_PCTR8	*(volatile uint32_t*)(V3D_BASE+0x006c0)	// Performance Counter Count 8
#define V3D_PCTRS8	*(volatile uint32_t*)(V3D_BASE+0x006c4)	// Performance Counter Mapping 8
#define V3D_PCTR9	*(volatile uint32_t*)(V3D_BASE+0x006c8)	// Performance Counter Count 9
#define V3D_PCTRS9	*(volatile uint32_t*)(V3D_BASE+0x006cc)	// Performance Counter Mapping 9
#define V3D_PCTR10	*(volatile uint32_t*)(V3D_BASE+0x006d0)	// Performance Counter Count 10
#define V3D_PCTRS10	*(volatile uint32_t*)(V3D_BASE+0x006d4)	// Performance Counter Mapping 10
#define V3D_PCTR11	*(volatile uint32_t*)(V3D_BASE+0x006d8)	// Performance Counter Count 11
#define V3D_PCTRS11	*(volatile uint32_t*)(V3D_BASE+0x006dc)	// Performance Counter Mapping 11
#define V3D_PCTR12	*(volatile uint32_t*)(V3D_BASE+0x006e0)	// Performance Counter Count 12
#define V3D_PCTRS12	*(volatile uint32_t*)(V3D_BASE+0x006e4)	// Performance Counter Mapping 12
#define V3D_PCTR13	*(volatile uint32_t*)(V3D_BASE+0x006e8)	// Performance Counter Count 13
#define V3D_PCTRS13	*(volatile uint32_t*)(V3D_BASE+0x006ec)	// Performance Counter Mapping 13
#define V3D_PCTR14	*(volatile uint32_t*)(V3D_BASE+0x006f0)	// Performance Counter Count 14
#define V3D_PCTRS14	*(volatile uint32_t*)(V3D_BASE+0x006f4)	// Performance Counter Mapping 14
#define V3D_PCTR15	*(volatile uint32_t*)(V3D_BASE+0x006f8)	// Performance Counter Count 15
#define V3D_PCTRS15	*(volatile uint32_t*)(V3D_BASE+0x006fc)	// Performance Counter Mapping 15
#define V3D_DBCFG   *(volatile uint32_t*)(V3D_BASE+0x00e00)
#define V3D_DBQITE  *(volatile uint32_t*)(V3D_BASE+0x00e2c)
#define V3D_DBQITC  *(volatile uint32_t*)(V3D_BASE+0x00e30)
#define V3D_DBGE	*(volatile uint32_t*)(V3D_BASE+0x00f00)	// PSE Error Signals
#define V3D_FDBGO	*(volatile uint32_t*)(V3D_BASE+0x00f04)	// FEP Overrun Error Signals
#define V3D_FDBGB	*(volatile uint32_t*)(V3D_BASE+0x00f08)	// FEP Interface Ready and Stall Signals, FEP Busy Signals
#define V3D_FDBGR	*(volatile uint32_t*)(V3D_BASE+0x00f0c)	// FEP Internal Ready Signals
#define V3D_FDBGS	*(volatile uint32_t*)(V3D_BASE+0x00f10)	// FEP Internal Stall Input Signals
#define V3D_ERRSTAT	*(volatile uint32_t*)(V3D_BASE+0x00f20)	// Miscellaneous Error Signals (VPM, VDW, VCD, VCM, L2C)

#endif
