#include <GColor.h>
#include <GPaint.h>

class GColor;
class GPaint;
enum class GBlendMode;

typedef GPixel (*blend_mode)(GPixel& src, GPixel& dst);

/*
* Takes an integer from [0...255] that needs to be divided by 255 and rounded.
*
* x: integer that will be rounded.
* 
* return: the rounded integer.
*/
static inline int Div255(int x) {
    return ((x + 128) * 257) >> 16;
}

/*
* Turns a src paint into a src pixel.
*
* src: paint that will be transformed
*
*/
GPixel color_to_pixel(GPaint src) {
    return GPixel_PackARGB(
        src.getColor().fA * 255 + 0.5,
        src.getColor().fA * src.getColor().fR * 255 + 0.5,
        src.getColor().fA * src.getColor().fG * 255 + 0.5,
        src.getColor().fA * src.getColor().fB * 255 + 0.5
    );
}


// Returns true if the blend functions will ultimately return dst.
bool willReturnDst(GBlendMode blend_mode, float alpha) {
    if (blend_mode == GBlendMode::kDst ||
    (blend_mode == GBlendMode::kSrcOver && alpha == 0) ||
    (blend_mode == GBlendMode::kDstOver && alpha == 0) ||
    (blend_mode == GBlendMode::kDstOut && alpha == 0) ||
    (blend_mode == GBlendMode::kDstIn && alpha == 1)) {
        return true; 
    } else {
        return false;
    }
}

// All blendmode functions. ----------------------------------------------------------------------------------------------

// 0
GPixel kClear(GPixel&src, GPixel& dst) {
    return GPixel_PackARGB(0, 0, 0, 0);
}

// S.
// Also used to put src into a premul form.
GPixel kSrc(GPixel& src, GPixel& dst) {
    return src;
}

// D.
GPixel kDst(GPixel& src, GPixel& dst) {
    return dst;
}

// S + (1 - Sa)*D.
GPixel kSrcOver(GPixel& src, GPixel& dst) {
    // Finding alphas.
    float src_alpha_inverse = 255 - GPixel_GetA(src);

    return GPixel_PackARGB(
        GPixel_GetA(src) + Div255((src_alpha_inverse * GPixel_GetA(dst))),
        GPixel_GetR(src) + Div255((src_alpha_inverse * GPixel_GetR(dst))),
        GPixel_GetG(src) + Div255((src_alpha_inverse * GPixel_GetG(dst))),
        GPixel_GetB(src) + Div255((src_alpha_inverse * GPixel_GetB(dst)))
    );
}

// D + (1 - Da)*S.
GPixel kDstOver(GPixel& src, GPixel& dst) {
    // Finding alpha
    float dst_alpha_inverse = 255 - GPixel_GetA(dst);

    // Finishing kDstOver.
    return GPixel_PackARGB(
        GPixel_GetA(dst) + Div255((dst_alpha_inverse * GPixel_GetA(src))),
        GPixel_GetR(dst) + Div255((dst_alpha_inverse * GPixel_GetR(src))),
        GPixel_GetG(dst) + Div255((dst_alpha_inverse * GPixel_GetG(src))),
        GPixel_GetB(dst) + Div255((dst_alpha_inverse * GPixel_GetB(src)))
    );
}

// Da * S.
GPixel kSrcIn(GPixel& src, GPixel& dst) {
    // Finding alphas.
    int dst_A = GPixel_GetA(dst);

    // Finishing kSrcIn.
    return GPixel_PackARGB(
        Div255(dst_A * GPixel_GetA(src)),
        Div255(dst_A * GPixel_GetR(src)),
        Div255(dst_A * GPixel_GetG(src)),
        Div255(dst_A * GPixel_GetB(src))
    );
}

// Sa * D.
GPixel kDstIn(GPixel& src, GPixel& dst) {
    // Finding alphas.
    int src_A = GPixel_GetA(src);

    // Finishing kDstIn.
    return GPixel_PackARGB(
        Div255(src_A * GPixel_GetA(dst)),
        Div255(src_A * GPixel_GetR(dst)),
        Div255(src_A * GPixel_GetG(dst)),
        Div255(src_A * GPixel_GetB(dst))
    );
}

// (1 - Da)*S.
GPixel kSrcOut(GPixel& src, GPixel& dst) {
    // Finding inverse alpha.
    int dst_A_inverse = 255 - GPixel_GetA(dst);
    
    // Finishing kSrcOut.
    return GPixel_PackARGB(
        Div255(dst_A_inverse * GPixel_GetA(src)),
        Div255(dst_A_inverse * GPixel_GetR(src)),
        Div255(dst_A_inverse * GPixel_GetG(src)),
        Div255(dst_A_inverse * GPixel_GetB(src))
    );
}

// (1 - Sa)*D.
GPixel kDstOut(GPixel& src, GPixel& dst) {
    // Finding inverse alpha.
    float src_A_inverse = 255 - GPixel_GetA(src);
    
    // Finishing kDstOut.
    return GPixel_PackARGB(
        Div255(src_A_inverse * GPixel_GetA(dst)),
        Div255(src_A_inverse * GPixel_GetR(dst)),
        Div255(src_A_inverse * GPixel_GetG(dst)),
        Div255(src_A_inverse * GPixel_GetB(dst))
    );
}

// Da*S + (1 - Sa)*D.
GPixel kSrcATop(GPixel& src, GPixel& dst) {
    // Finding alphas.
    int dst_A = GPixel_GetA(dst);
    float src_A_inverse = 255 - GPixel_GetA(src);
    
    // Finishing kSrcATop.
   return GPixel_PackARGB(
        Div255((dst_A * GPixel_GetA(src)) + (src_A_inverse * dst_A)),
        Div255((dst_A * GPixel_GetR(src)) + (src_A_inverse * GPixel_GetR(dst))),
        Div255((dst_A * GPixel_GetG(src)) + (src_A_inverse * GPixel_GetG(dst))),
        Div255((dst_A * GPixel_GetB(src)) + (src_A_inverse * GPixel_GetB(dst)))
    );
}

// Sa*D + (1 - Da)*S.
GPixel kDstATop(GPixel& src, GPixel& dst) {
    // Premul'ing source.
    GPixel src_pixel = kSrc(src, dst);

    // Finding alphas.
    float src_A = GPixel_GetA(src);
    int dst_A_inverse = 255 - GPixel_GetA(dst);
    
    // Finishing kDstATop.
    return GPixel_PackARGB(
        Div255((src_A * GPixel_GetA(dst)) + (dst_A_inverse * GPixel_GetA(src_pixel))),
        Div255((src_A * GPixel_GetR(dst)) + (dst_A_inverse * GPixel_GetR(src_pixel))),
        Div255((src_A * GPixel_GetG(dst)) + (dst_A_inverse * GPixel_GetG(src_pixel))),
        Div255((src_A * GPixel_GetB(dst)) + (dst_A_inverse * GPixel_GetB(src_pixel)))
    );
}

// (1 - Sa)*D + (1 - Da)*S.
GPixel kXor(GPixel& src, GPixel& dst) {
    // Finding alphas.
    int src_A_inverse = 255 - GPixel_GetA(src);
    int dst_A_inverse = 255 - GPixel_GetA(dst);
    
    // Finishing kXor.
    return GPixel_PackARGB(
        Div255((src_A_inverse * GPixel_GetA(dst) + (dst_A_inverse * GPixel_GetA(src)))),
        Div255((src_A_inverse * GPixel_GetR(dst) + (dst_A_inverse * GPixel_GetR(src)))),
        Div255((src_A_inverse * GPixel_GetG(dst) + (dst_A_inverse * GPixel_GetG(src)))),
        Div255((src_A_inverse * GPixel_GetB(dst) + (dst_A_inverse * GPixel_GetB(src))))
    );
}

/*
* Helper function to figure out how to calculate different blend modes.
*
* src: GPixel whose color will be mixed with the dst.
*
* dst: GPixel whose color will be mixed with the src depending on src's blendmode.
*
* returns: the new, blended GPixel.
*/
GPixel get_blendmode_pixel(GPixel& src, GPixel& dst, GBlendMode blend_mode) {
    // Checking whether src or dst alpha is 1 or 0.
    // Used for special cases in blend modes.
    int src_A_is_one = GPixel_GetA(src) == 255 ? 1 : 0;
    int src_A_is_zero = GPixel_GetA(src) == 0 ? 1 : 0;
    int dst_A_is_one = GPixel_GetA(dst) == 255 ? 1 : 0;
    int dst_A_is_zero = GPixel_GetA(dst) == 0 ? 1 : 0;

    switch(blend_mode) {
        // (1 - Sa)*D + (1 - Da)*S.
        case GBlendMode::kXor:
            // kXor --> kClear when src_A == 1 and dst_A == 1.
            if (src_A_is_one && dst_A_is_one) {
                return kClear(src, dst);
            // kXor --> kSrc when src_A == 1 and dst_A == 0
            } else if (src_A_is_one && dst_A_is_zero) {
                return kSrc(src, dst);
            // kXor --> kSrcOut when src_A == 1 and dst_A != 0.
            } else if (src_A_is_one) {
                return kSrcOut(src, dst);
            // kXor --> kDst when src_A != 0 and dst_ == 1
            } else if (dst_A_is_one) {
                return kDstOut(src, dst);
            } else {
                return kXor(src, dst);
            }

        // Sa*D + (1 - Da)*S.
        case GBlendMode::kDstATop:
            // kDstATop --> kDst when src_A == 1 and dst_A == 1.
            if (src_A_is_one && dst_A_is_one) {
                return kDst(src, dst);
            // kDstATop --> kSrc when src_A == 0 and dst_A == 0.
            } else if (src_A_is_zero && dst_A_is_zero) {
                return kSrc(src, dst);
            // kDstATop --> kDstIn when src_A == 0 and dst_A != 1.
            } else if (dst_A_is_one) {
                return kDstIn(src, dst);
            } else {
                return kDstATop(src, dst);
            }

        // Da*S + (1 - Sa)*D.
        case GBlendMode::kSrcATop:
            // kSrcATop --> kClear when src_A == 1 and dst_A == 1, 
            if (src_A_is_one && dst_A_is_one) {
                return kSrc(src, dst);
            // kSrcATop --> kSrcIn when src_A == 1 and dst_A != 0.
            } else if (src_A_is_one) {
                return kSrcIn(src, dst);
            // kSrcATop --> kDst when src_A == 0 and dst_A == 0.
            } else if (dst_A_is_zero) {
                return kDst(src, dst);
            } else {
                return kSrcATop(src, dst);
            }

        // D + (1 - Da)*S.
        case GBlendMode::kDstOver:
            // kDstOver --> kSrc when dst_A == 0.
            if (dst_A_is_zero) return kSrc(src, dst);
            // kDstOver --> kDst when dst_A == 1 or src_A == 0.
            if (dst_A_is_one) return kDst(src, dst);
            return kDstOver(src, dst);

        // S + (1 - Sa)*D.
        case GBlendMode::kSrcOver:
            // kSrcOver --> kSrc when src_A == 1.
            if (src_A_is_one) return kSrc(src, dst);
            return kSrcOver(src, dst);

        // (1 - Da)*S.
        case GBlendMode::kSrcOut:
            // kSrcOut --> kClear when dst_A == 1 or src_A == 0.
            if (dst_A_is_one || src_A_is_zero) return kClear(src, dst);
            // kSrcOut --> kSrc when dst_A == 0.
            if (dst_A_is_zero) return kSrc(src, dst);
            return kSrcOut(src, dst);

        // (1 - Sa)*D.
        case GBlendMode::kDstOut:
            // kDstOut --> kClear when src_A == 1.
            if (src_A_is_one) return kClear(src, dst);
            return kDstOut(src, dst);

        // Da * S.
        case GBlendMode::kSrcIn:
            // kSrcIn --> kClear when dst_A == 0 or src_A == 0.
            if (dst_A_is_zero || src_A_is_zero) return kClear(src, dst);
            // kSrcIn --> kSrc when dst_A == 1.
            if (dst_A_is_one)  return kSrc(src, dst);
            return kSrcIn(src, dst);

        // Sa * D.
        case GBlendMode::kDstIn:
            // kDstIn --> kClear when src_A == 0.
            if (src_A_is_zero) return kClear(src, dst);
            return kDstIn(src, dst);

        // 0.
        case GBlendMode::kClear: 
            return kClear(src, dst);

        // S.
        case GBlendMode::kSrc:
            return kSrc(src, dst);

        // D.
        case GBlendMode::kDst:
            return kDst(src, dst);
    }
    printf("Shouldn't be here.\n");
    // Return a dummy pixel as default.
    return GPixel_PackARGB(0, 0, 0,0);
}


// -----------------------------------------------------------------------------------------------------------------------------

// Returns the appropriate blendmode function.
blend_mode get_blendmode(GPaint& src, GPixel& dst) {
    // Retrieving the blend mode.
    GBlendMode blend_mode = src.getBlendMode();

    // Checking whether src or dst alpha is 1 or 0.
    // Used for special cases in blend modes.
    int src_A_is_one = src.getAlpha() == 1 ? 1 : 0;
    int src_A_is_zero = src.getAlpha() == 0 ? 1 : 0;
    int dst_A_is_one = GPixel_GetA(dst) == 255 ? 1 : 0;
    int dst_A_is_zero = GPixel_GetA(dst) == 0 ? 1 : 0;

    switch(blend_mode) {
        // (1 - Sa)*D + (1 - Da)*S.
        case GBlendMode::kXor:
            // kXor --> kClear when src_A == 1 and dst_A == 1.
            if (src_A_is_one && dst_A_is_one) {
                return kClear;
            // kXor --> kSrc when src_A == 1 and dst_A == 0
            } else if (src_A_is_one && dst_A_is_zero) {
                return kSrc;
            // kXor --> kSrcOut when src_A == 1 and dst_A != 0.
            } else if (src_A_is_one) {
                return kSrcOut;
            // kXor --> kDst when src_A != 0 and dst_ == 1
            } else if (dst_A_is_one) {
                return kDstOut;
            } else {
                return kXor;
            }

        // Sa*D + (1 - Da)*S.
        case GBlendMode::kDstATop:
            // kDstATop --> kDst when src_A == 1 and dst_A == 1.
            if (src_A_is_one && dst_A_is_one) {
                return kDst;
            // kDstATop --> kSrc when src_A == 0 and dst_A == 0.
            } else if (src_A_is_zero && dst_A_is_zero) {
                return kSrc;
            // kDstATop --> kDstIn when src_A == 0 and dst_A != 1.
            } else if (dst_A_is_one) {
                return kDstIn;
            } else {
                return kDstATop;
            }

        // Da*S + (1 - Sa)*D.
        case GBlendMode::kSrcATop:
            // kSrcATop --> kClear when src_A == 1 and dst_A == 1, 
            if (src_A_is_one && dst_A_is_one) {
                return kSrc;
            // kSrcATop --> kSrcIn when src_A == 1 and dst_A != 0.
            } else if (src_A_is_one) {
                return kSrcIn;
            // kSrcATop --> kDst when src_A == 0 and dst_A == 0.
            } else if (dst_A_is_zero) {
                return kDst;
            } else {
                return kSrcATop;
            }

        // D + (1 - Da)*S.
        case GBlendMode::kDstOver:
            // kDstOver --> kSrc when dst_A == 0.
            if (dst_A_is_zero) return kSrc;
            // kDstOver --> kDst when dst_A == 1 or src_A == 0.
            if (dst_A_is_one) return kDst;
            return kDstOver;

        // S + (1 - Sa)*D.
        case GBlendMode::kSrcOver:
            // kSrcOver --> kSrc when src_A == 1.
            if (src_A_is_one) return kSrc;
            return kSrcOver;

        // (1 - Da)*S.
        case GBlendMode::kSrcOut:
            // kSrcOut --> kClear when dst_A == 1 or src_A == 0.
            if (dst_A_is_one || src_A_is_zero) return kClear;
            // kSrcOut --> kSrc when dst_A == 0.
            if (dst_A_is_zero) return kSrc;
            return kSrcOut;

        // (1 - Sa)*D.
        case GBlendMode::kDstOut:
            // kDstOut --> kClear when src_A == 1.
            if (src_A_is_one) return kClear;
            return kDstOut;

        // Da * S.
        case GBlendMode::kSrcIn:
            // kSrcIn --> kClear when dst_A == 0 or src_A == 0.
            if (dst_A_is_zero || src_A_is_zero) return kClear;
            // kSrcIn --> kSrc when dst_A == 1.
            if (dst_A_is_one)  return kSrc;
            return kSrcIn;

        // Sa * D.
        case GBlendMode::kDstIn:
            // kDstIn --> kClear when src_A == 0.
            if (src_A_is_zero) return kClear;
            return kDstIn;

        // 0.
        case GBlendMode::kClear: 
            return kClear;

        // S.
        case GBlendMode::kSrc:
            return kSrc;

        // D.
        case GBlendMode::kDst:
            return kDst;
    }
    printf("Shouldn't be here.\n");
    // Return a dummy function as default.
    return kClear;
}