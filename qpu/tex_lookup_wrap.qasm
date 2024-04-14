.include "common.qinc"

# TODO: repeat modes, and probably move iTexHeight-1 outside loop

    calc_varying fU,fvu0,fvu1,fvu2
    calc_varying fV,fvv0,fvv1,fvv2

    ftoi        r0,fU
    sub         r1,iTexWidth,1
    and         r0,r0,r1
    ftoi        r1,fV
    sub         r2,iTexHeight,1
    and         r1,r1,r2
    mul24       r1,r1,iTexWidth
    add         r0,r0,r1
    shl         r0,r0,2
    add         t0s,r0,iTexAddr
    ldtmu0


