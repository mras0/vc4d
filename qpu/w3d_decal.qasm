.include "common.qinc"
    #// Cp(1-As) + CsAs = Cp + As*(Cs-Cp)
    #return {
    #    cp.a,
    #    cp.r + cs.a * (cs.r - cp.r),
    #    cp.g + cs.a * (cs.g - cp.g),
    #    cp.b + cs.a * (cs.b - cp.b),
    #};
#   Cs -> texture source color = r4
#   Cp -> previous (texture stage) color = fXX
    # Cs = texture color, cp = interpolated color

    fsub    r1,1.0,r4.8df       # r1 = (1-As)
    fmul    r2.8asf,fB,r1
    fmul    r2.8bsf,fG,r1
    fmul    r2.8csf,fR,r1       # r2 = (1-As) * Cp

    mov     r1,r4.8dr

    v8muld  r1,r4,r1            # r1 = As*Cs

    v8adds  r0,r1,r2            # r0 = As*Cs + (1-As) * Cp

    mov     r0.8dsf,r4.8df      # output alpha = Cp.A
