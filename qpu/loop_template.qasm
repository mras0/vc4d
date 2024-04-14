.include "common.qinc"

.const Z_BUFFER_DISABLED ,     0
.const W3D_Z_NEVER       ,     1       # discard incoming pixel
.const W3D_Z_LESS        ,     2       # draw, if value < Z(Z_Buffer)
.const W3D_Z_GEQUAL      ,     3       # draw, if value >= Z(Z_Buffer)
.const W3D_Z_LEQUAL      ,     4       # draw, if value <= Z(Z_Buffer)
.const W3D_Z_GREATER     ,     5       # draw, if value > Z(Z_Buffer)
.const W3D_Z_NOTEQUAL    ,     6       # draw, if value != Z(Z_Buffer)
.const W3D_Z_EQUAL       ,     7       # draw, if value == Z(Z_Buffer)
.const W3D_Z_ALWAYS      ,     8       # always draw


    # XXX TEMP TEMP TEMP (for W3D_BLEND)
    mov         cEnvColor,-1

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
    .if iNQPU == 12
    itof        r0,qpu_num
    .else
    mov         r0,1.0
    .endif
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

    .if iNQPU == 12
    # Adjust screen stuff to match number of QPUs
    mul24       r0,iScreenDelta,qpu_num
    add         iScreenAddr,iScreenAddr,r0
    .if Z_MODE != 0
    # Adjust Zbuffer ptr
    add         iZBufAddr,iZBufAddr,r0
    .endif
    mul24       iScreenDelta,iScreenDelta,iNQPU
    .else
    .assert iNQPU==1
    .endif


    # Also replicated in delay slots for yloop
    mov         fW0,fW0_row
    mov         fW1,fW1_row
    mov         fW2,fW2_row
:yloop
    mov         iXCnt,iXSteps
    mov         iScreenPtr,iScreenAddr
    .if Z_MODE != 0
    mov         iZBufPtr,iZBufAddr
    .endif
:xloop
    mov         r0,fW0
    or          r0,r0,fW1
    or.setf     r0,r0,fW2
    brr.alln    -,:next
    mov         fMask,r0        # delay slot 1

    .if Z_MODE != 0
    start_load16h iZBufPtr # 2 delays slots overlap
    fmul    r0,fW1,fvz1
    fmul    r1,fW2,fvz2
    fadd    r0,r0,r1
    fadd    r3,r0,fvz0      # r3 = calculated z value
    finish_load r2

    .assert Z_MODE == W3D_Z_LESS
    # Z_LESS => draw if value < Z(Z_Buffer)
    # => Need floating point value >= 0 in mask register
    # TODO: Below gives LE not less, but eh, probably just adjust value a bit?
    fsub        r0,r2,r3
    mov         fZVal,r2

    or.setf     fMask,fMask,r0
    brr.alln    -,:next
    mov.ifnn    fZVal,r3 # delay slot 1, Update zbuffer

    .endif

    ## BODY GOES HERE ###
    .long -1 # Marker
    #####################


    mov.setf    -,fMask
    mov.ifnn    iDestColor,r0

    .if Z_MODE != 0

    # Visualize depth
    #mov         r0,255.0
    #fmul        r0,r0,fZVal
    #ftoi        r0,r0
    #mov.ifnn    iDestColor,r0

    store16h    iZBufPtr,fZVal
    .endif


    store16h    iScreenPtr,iDestColor

:next
    mov         r0,iXIncr
    shl         r0,r0,2
    add         iScreenPtr,iScreenPtr,r0
    .if Z_MODE != 0
    add         iZBufPtr,iZBufPtr,r0
    .endif
    sub.setf    iXCnt,iXCnt,1
    brr.anynz   -,:xloop
    fadd        fW0,fW0,fA12 # delay slot 1
    fadd        fW1,fW1,fA20 # delay slot 2
    fadd        fW2,fW2,fA01 # delay slot 3

    mov         r0,iScreenDelta
    add         iScreenAddr,iScreenAddr,r0
    .if Z_MODE != 0
    add         iZBufAddr,iZBufAddr,r0
    .endif
    fadd        fW0_row,fW0_row,fB12
    fadd        fW1_row,fW1_row,fB20
    fadd        fW2_row,fW2_row,fB01

    sub.setf    iYSteps,iYSteps,1
    brr.anynz   -,:yloop
    mov         fW0,fW0_row # delay slot 1
    mov         fW1,fW1_row # delay slot 2
    mov         fW2,fW2_row # delay slot 3


# Triangle loop end
