.include "common.qinc"
# TODO
# Cp = Prev color (fA,fR,fG,fB), Cs = Source color (from texture, r4), Cc = Environment color (fEnvColor)
# Cout = Cp(1 - Cs) + Cc*Cs
# Aout = Ap * As

    # r0 = Cp
    mov     r0.8asf,fB
    mov     r0.8bsf,fG
    mov     r0.8csf,fR
    mov     r0.8dsf,fA

    # r1 = 1 - Cs
    v8subs  r1,-1,r4

    # r0 = Cp * (1-Cs)
    v8muld  r0,r0,r1

    # r1 = Cc * Cs
    v8muld  r1,cEnvColor,r4

    # r0 = Cp * (1-Cs) + Cc * Cs
    v8adds  r0,r0,r1

    # Aout = Ap*As
    fmul    r0.8dsf,r4.8df,fA
