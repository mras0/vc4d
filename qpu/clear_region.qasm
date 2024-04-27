.include "common.qinc"

    mov         iXSteps,unif
    sub         iYSteps,unif,1          # Height
    mov         iScreenAddr,unif
    mov         iScreenDelta,unif

    mov         r0,qpu_num
    shl         r0,r0,iVPMShift
    ldi         r1,vpm_setup(0, 1, h32(0))
    add         vw_setup,r0,r1
    mov         vpm,unif        # value

    # Adjust screen stuff to match number of QPUs
    mul24       r0,iScreenDelta,qpu_num
    add         iScreenAddr,iScreenAddr,r0
    mul24       iScreenDelta,iScreenDelta,iNQPU

    sub.setf    iYSteps,iYSteps,qpu_num
    brr.anyn    -,:out

:yloop
    mov         iXCnt,iXSteps
    mov         iScreenPtr,iScreenAddr
:xloop
    mov         -,vw_wait
    mov         r0,qpu_num
    shl         r0,r0,iVPMShift+7
    ldi         r1,vdw_setup_0(1,16,dma_h32(0,0))
    add         vw_setup,r0,r1
    mov         vw_addr,iScreenPtr

    sub.setf    iXCnt,iXCnt,1
    brr.anynz   -,:xloop
    mov         r0,iXIncr                   # delay slot 1
    shl         r0,r0,2                     # delay slot 2
    add         iScreenPtr,iScreenPtr,r0    # delay slot 3

    mov         r0,iScreenDelta
    add         iScreenAddr,iScreenAddr,r0

    sub.setf    iYSteps,iYSteps,iNQPU
    brr.anynn   -,:yloop



    nop         # delay slot 1
    nop         # delay slot 2
    nop         # delay slot 3

    mov         -,vw_wait
:out

# end
    mov interrupt, 1    # End when running via MBox interface
    nop; thrend         # Or running directly
    nop
    nop
