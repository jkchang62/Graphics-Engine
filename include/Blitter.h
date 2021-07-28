#include <GMatrix.h>
#include <GShader.h>

typedef GPixel (*blend_mode)(GPixel& src, GPixel& dst);
class GMatrix;
class GShader;

class Blitter {
    public:
    bool local_dest_set = false;

    // Constructor.
    Blitter(const GPaint _src, const GBitmap& _bit_map, GMatrix _ctm) {

        local_src = _src;
        local_src_pixel = color_to_pixel(_src);
        local_dst = GPixel_PackARGB(0, 0, 0, 0);
        blend_ptr = nullptr;
        bit_map = _bit_map;
        ctm = _ctm;
    }

    /*
    * Colors a row of pixels. 
    *
    * y: integer of the y-value of the row on the bit_map.
    *
    * start_x: integer of the start of the row.
    * 
    * end_x: integer of the end of the row.
    */
    void blit(int y, int start_x, int end_x) {
        // Shading + blitting.
        if (local_src.getShader() && local_src.getShader()->setContext(ctm)) {
            // Set a new array for the pixels.
            int count = end_x - start_x;
            assert(count >= 0);
            GPixel new_pixels[count];

            // Retrieving the pixels from the shader's location.
            local_src.getShader()->shadeRow(start_x, y, count, new_pixels);

            // Blitting using the shader's pixels.
            for (int x = start_x; x < end_x; x++) {
                // Pixels from shadeRow.
                GPixel new_src = new_pixels[x - start_x];
                GPixel dst = *bit_map.getAddr(x, y);
                *bit_map.getAddr(x, y) = get_blendmode_pixel(new_src, dst, local_src.getBlendMode());
            }

        // Normal blitting.
        } else {
            for (int x = start_x; x < end_x; x++) {
                GPixel dst = *bit_map.getAddr(x, y);
                set_blend_mode(dst);
                *bit_map.getAddr(x, y) = blend_ptr(local_src_pixel, dst);
            }
        }
    }

    /*
    * Set a new blend mode if the dst value has changed. If the dst value has changed, set local_dst to it.
    *
    * dst: GPixel that will be compared to the local dst given.
    * 
    * returns true if the local_dst and blend ptr has been changed and false otherwise.
    */
    bool set_blend_mode(GPixel dst) {
        if (!local_dest_set || GPixel_GetA(local_dst) != GPixel_GetA(dst)) {
            local_dest_set = true;
            local_dst = dst;
            blend_ptr = get_blendmode(local_src, dst);
            return true;
        } else {
            return false;
        }
    }

    private:
        GBitmap bit_map; 
        GPixel (*blend_ptr)(GPixel& src, GPixel& dst);
        GPixel local_dst;
        GPaint local_src;
        GPixel local_src_pixel;
        GMatrix ctm;
};