.include "common.qinc"

# TODO: A number of registers aren't live at the same time in the innerloop

    mov         iNumTriangles,unif

:triloop

    # Fetch uniforms
    mov         iXSteps,unif
    mov         iYSteps,unif
    mov         iScreenAddr,unif
    mov         iScreenDelta,unif
    mov         iTexAddr,unif
    mov         iTexWidth,unif
    mov         iTexHeight,unif
    mov         fA01,unif
    mov         fA12,unif
    mov         fA20,unif
    mov         fB01,unif
    mov         fB12,unif
    mov         fB20,unif
    mov         fW0_row,unif
    mov         fW1_row,unif
    mov         fW2_row,unif
    mov         fvw0,unif
    mov         fvw1,unif
    mov         fvw2,unif
    mov         fvu0,unif
    mov         fvu1,unif
    mov         fvu2,unif
    mov         fvv0,unif
    mov         fvv1,unif
    mov         fvv2,unif
    mov         fvr0,unif
    mov         fvr1,unif
    mov         fvr2,unif
    mov         fvg0,unif
    mov         fvg1,unif
    mov         fvg2,unif
    mov         fvb0,unif
    mov         fvb1,unif
    mov         fvb2,unif
    mov         fva0,unif
    mov         fva1,unif
    mov         fva2,unif

    # Adjust w0/w1/w2 row elements (for SIMD)

    itof        r0,elem_num         # r0 = float(elem_num)
    fmul        r1,r0,fA12
    fadd        fW0_row,fW0_row,r1
    fmul        r1,r0,fA20
    fadd        fW1_row,fW1_row,r1
    fmul        r1,r0,fA01
    fadd        fW2_row,fW2_row,r1

    # Scale Axx
    mov         r0,fXIncr
    fmul        fA01,fA01,r0
    fmul        fA12,fA12,r0
    fmul        fA20,fA20,r0

    # Adjust w0/w1/w2 to match QPU row
    itof        r0,qpu_num
    fmul        r1,r0,fB12
    fadd        fW0_row,fW0_row,r1
    fmul        r1,r0,fB20
    fadd        fW1_row,fW1_row,r1
    fmul        r1,r0,fB01
    fadd        fW2_row,fW2_row,r1

    # Scale Bxx
    itof        r0,iNQPU
    fmul        fB01,fB01,r0
    fmul        fB12,fB12,r0
    fmul        fB20,fB20,r0

    # Adjust screen stuff to match number of QPUs
    mul24       r0,iScreenDelta,qpu_num
    add         iScreenAddr,iScreenAddr,r0
    mul24       iScreenDelta,iScreenDelta,iNQPU


    # Also replicated in delay slots for yloop
    mov         fW0,fW0_row
    mov         fW1,fW1_row
    mov         fW2,fW2_row
:yloop
    mov         iXCnt,iXSteps
    mov         iScreenPtr,iScreenAddr
:xloop
    mov         r0,fW0
    or          r0,r0,fW1
    or.setf     r0,r0,fW2
    brr.alln    -,:next
    mov         fMask,r0        # delay slot 1

    ## BODY GOES HERE ###
    .long -1 # Marker
    #####################


    mov.setf    -,fMask
    mov.ifnn    iDestColor,r0

    store16h    iScreenPtr,iDestColor

:next
    mov         r0,iXIncr
    shl         r0,r0,2
    add         iScreenPtr,iScreenPtr,r0
    sub.setf    iXCnt,iXCnt,1
    brr.anynz   -,:xloop
    fadd        fW0,fW0,fA12 # delay slot 1
    fadd        fW1,fW1,fA20 # delay slot 2
    fadd        fW2,fW2,fA01 # delay slot 3

    add         iScreenAddr,iScreenAddr,iScreenDelta
    fadd        fW0_row,fW0_row,fB12
    fadd        fW1_row,fW1_row,fB20
    fadd        fW2_row,fW2_row,fB01

    sub.setf    iYSteps,iYSteps,1
    brr.anynz   -,:yloop
    mov         fW0,fW0_row # delay slot 1
    mov         fW1,fW1_row # delay slot 2
    mov         fW2,fW2_row # delay slot 3

    nop
    nop

    sub.setf    iNumTriangles,iNumTriangles,1
    brr.anynz   -,:triloop

    nop         # delay slot 1
    nop         # delay slot 2
    nop         # delay slot 3

    mov         -,vw_wait

# end
    mov interrupt, 1    # End when running via MBox interface
    nop; thrend         # Or running directly
    nop
    nop
