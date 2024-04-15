.include "common.qinc"

# TODO: A number of registers aren't live at the same time in the innerloop

    mov         iNumTriangles,unif
    mov         iTexAddr,unif
    mov         iTexWidth,unif
    mov         iTexHeight,unif

    # Ensure color registers are loaded with sane values
    mov         fA,1.0
    mov         fR,1.0
    mov         fG,1.0
    mov         fB,1.0

:triloop

    # Fetch uniforms
    mov         iXSteps,unif
    mov         iYSteps,unif
    mov         iScreenAddr,unif
    mov         iScreenDelta,unif
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

    ## BODY GOES HERE ###
    .long -1 # Marker
    #####################

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
