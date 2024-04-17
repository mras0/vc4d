.include "common.qinc"

    mov     r1,iDestColor

    .long -1 # Marker

    v8muld      r0,r0,r2
    v8muld      r1,r1,r3
    v8adds      r0,r0,r1
