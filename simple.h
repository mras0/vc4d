/* [0x00000000] */ 0x15827d80, 0x10021027, //     mov         iXSteps,unif
/* [0x00000008] */ 0x15827d80, 0x10020027, //     mov         iYSteps,unif
/* [0x00000010] */ 0x15827d80, 0x10021067, //     mov         iScreenAddr,unif
/* [0x00000018] */ 0x15827d80, 0x10020067, //     mov         iScreenDelta,unif
// :yloop
/* [0x00000020] */ 0x159c0fc0, 0x100200a7, //     mov         iXCnt,iXSteps
/* [0x00000028] */ 0x159c1fc0, 0x100200e7, //     mov         iScreenPtr,iScreenAddr
// :xloop
/* [0x00000030] */ 0xffffffff, 0xe0020527, //     mov         iDestColor,-1
/* [0x00000038] */ 0x009e7000, 0x100009e7, //     nop
/* [0x00000040] */ 0x159f2fc0, 0x100009e7, //     mov -,vw_wait
/* [0x00000048] */ 0x159e6fc0, 0x10020827, //     mov r0,qpu_num
/* [0x00000050] */ 0x119c21c0, 0xd0020827, //     shl r0,r0,iVPMShift
/* [0x00000058] */ 0x00001a00, 0xe0020867, //     ldi r1,vpm_setup(0, 1, h32(0))
/* [0x00000060] */ 0x0c9e7040, 0x10021c67, //     add vw_setup,r0,r1
/* [0x00000068] */ 0x15527d80, 0x10020c27, //     mov vpm,value
/* [0x00000070] */ 0x159e6fc0, 0x10020827, //     mov r0,qpu_num
/* [0x00000078] */ 0x119c91c0, 0xd0020827, //     shl r0,r0,iVPMShift+7
/* [0x00000080] */ 0x80904000, 0xe0020867, //     ldi r1,vdw_setup_0(1,16,dma_h32(0,0))
/* [0x00000088] */ 0x0c9e7040, 0x10021c67, //     add vw_setup,r0,r1
/* [0x00000090] */ 0x150e7d80, 0x10021ca7, //     mov vw_addr,addr
/* [0x00000098] */ 0x00000010, 0xe0020827, //     mov         r0,iXIncr
/* [0x000000a0] */ 0x119c21c0, 0xd0020827, //     shl         r0,r0,2
/* [0x000000a8] */ 0x0c0e7c00, 0x100200e7, //     add         iScreenPtr,iScreenPtr,r0
/* [0x000000b0] */ 0x0d081dc0, 0xd00220a7, //     sub.setf    iXCnt,iXCnt,1
/* [0x000000b8] */ 0xffffff58, 0xf03809e7, //     brr.anynz   -,:xloop
/* [0x000000c0] */ 0x009e7000, 0x100009e7, //     nop
/* [0x000000c8] */ 0x009e7000, 0x100009e7, //     nop
/* [0x000000d0] */ 0x009e7000, 0x100009e7, //     nop
/* [0x000000d8] */ 0x0c041f80, 0x10021067, //     add         iScreenAddr,iScreenAddr,iScreenDelta
/* [0x000000e0] */ 0x0d001dc0, 0xd0022027, //     sub.setf    iYSteps,iYSteps,1
/* [0x000000e8] */ 0xffffff18, 0xf03809e7, //     brr.anynz   -,:yloop
/* [0x000000f0] */ 0x009e7000, 0x100009e7, //     nop
/* [0x000000f8] */ 0x009e7000, 0x100009e7, //     nop
/* [0x00000100] */ 0x009e7000, 0x100009e7, //     nop
/* [0x00000108] */ 0x159f2fc0, 0x100009e7, //     mov         -,vw_wait
/* [0x00000110] */ 0x00000001, 0xe00209a7, //     mov interrupt, 1    # End when running via MBox interface
/* [0x00000118] */ 0x009e7000, 0x300009e7, //     nop; thrend         # Or running directly
/* [0x00000120] */ 0x009e7000, 0x100009e7, //     nop
/* [0x00000128] */ 0x009e7000, 0x100009e7  //     nop
