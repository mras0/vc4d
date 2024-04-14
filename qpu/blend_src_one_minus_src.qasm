.include "common.qinc"

    mov         aTemp0,r0
    mov         r2,-1
    mov         r1,aTemp0.8dr       # r1 = a
    sub         r2,r2,r1  ; v8muld r0,r0,r1   # r2 = 255-a
    v8muld      r1,r2,iDestColor
    v8adds      r0,r0,r1

