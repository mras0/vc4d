.include "common.qinc"

    # Calc z = 1/w
    # Val n = v##n[0] + w1 * v##n[1] + w2 * v##n[2];
    fmul        r0,fvw1,fW1     # delay slot 2
    fmul        r1,fvw2,fW2     # delay slot 3
    fadd        r0,r0,r1
    fadd        r0,r0,fvw0
    mov         recip,r0

    # Load destination colors from framebuffer
    start_load16h iScreenPtr # 2 recip delays slots overlap

    mov         r3,r4               # r3 = z
    # Texture look up

    calc_varying fU,fvu0,fvu1,fvu2
    calc_varying fV,fvv0,fvv1,fvv2

    ftoi        r0,fU
    sub         r1,iTexWidth,1
    and         r0,r0,r1
    ftoi        r1,fV
    sub         r2,iTexHeight,1
    and         r1,r1,r2
    mul24       r1,r1,iTexHeight
    add         r0,r0,r1
    shl         r0,r0,2
    add         t0s,r0,iTexAddr
    ldtmu0

    # r4 now contains color from texture

    calc_varying fB,fvb0,fvb1,fvb2
    calc_varying fG,fvg0,fvg1,fvg2
    calc_varying fR,fvr0,fvr1,fvr2
    calc_varying fA,fva0,fva1,fva2

    finish_load iDestColor # N.B. trashes r0/r1

    # W3D_MODULATE

    fmul        r0.8asf,r4.8af,fB
    fmul        r0.8bsf,r4.8bf,fG
    fmul        r0.8csf,r4.8cf,fR
    fmul        r0.8dsf,r4.8df,fA

    # Alpha blend
    mov         aTemp0,r0
    mov         r2,-1
    mov         r1,aTemp0.8dr       # r1 = a
    sub         r2,r2,r1  ; v8muld r0,r0,r1   # r2 = 255-a
    v8muld      r1,r2,iDestColor
    v8adds      r0,r0,r1

