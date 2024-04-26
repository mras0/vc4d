.include "common.qinc"

    mov         aTemp0,r0
    mov         r1,fAlphaRef
    fsub.setf   -,aTemp0.8df,r1
    mov.ifn     fMask,-1.0
    mov.ifz     fMask,-1.0
    nop # fMask used in following instruction
