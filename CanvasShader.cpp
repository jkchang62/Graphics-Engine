#include <GShader.h>
#include <GMatrix.h>
#include <GBitmap.h>

class CanvasShader : public GShader {
    public:

    // Constructor.
    CanvasShader(const GBitmap& device, const GMatrix& matrix, GShader::TileMode mode = GShader::kClamp) : bit_map(device), local_matrix(matrix), tile_mode(mode) {
        inverse_width  = (float) 1 / bit_map.width();
        inverse_height = (float) 1 / bit_map.height();
    };

    // Return true iff all of the GPixels that may be returned by this shader will be opaque.
    bool isOpaque() override {
        return bit_map.isOpaque();
    }

    // The draw calls in GCanvas must call this with the CTM before any calls to shadeRow().
    // Returns true if the matrix invertible and false if it's not.
    bool setContext(const GMatrix& new_ctm) {
        // Finding and checking the determinant.
        float det = new_ctm[0] * new_ctm[4] - new_ctm[1] * new_ctm[3];
        if (det == 0) return false;

        // Setting the new ctm.
        ctm = new_ctm;
        return true;
    }

    /**
     *  Given a row of pixels in device space [x, y] ... [x + count - 1, y], return the
     *  corresponding src pixels in row[0...count - 1]. The caller must ensure that row[]
     *  can hold at least [count] entries.
     */
    void shadeRow(int x, int y, int count, GPixel row[]) override {
        // Invariant fixing.
        GMatrix inverse_matrix = ctm * local_matrix;
        inverse_matrix.invert(&inverse_matrix);
        float fx = x + 0.5f;
        float fy = y + 0.5f;

        // Iterate through the row.
        for (int i = 0; i < count; i++) {
            // Finding the new point.
            GPoint P = inverse_matrix * GPoint::Make(fx, fy);
            
            // Repeating tiles.
            if (tile_mode == kRepeat) {
                P.set(repeat(P.x(), inverse_width, bit_map.width()), repeat(P.y(), inverse_height, bit_map.height()));

            // Mirroring tiles.
            } else if (tile_mode == kMirror) {
                P.set(mirror(P.x(), inverse_width, bit_map.width()), mirror(P.y(), inverse_height, bit_map.height()));
            }
            
            // Clamping 
            P.set(pin(P.x(), 0, bit_map.width() - 1), pin(P.y(), 0, bit_map.height() - 1));
            
            // Assign the new pixel to an index in row.
            row[i] = *bit_map.getAddr(GFloorToInt(P.x()), GFloorToInt(P.y()));

            // Incrementing.
            fx += 1;
        }
    }

    // Takes a value and pins it to the appropriate border.
    float pin(float value, int min, int max) {
        if (value > max) {
            return max;
        } else if (value < min) {
            return min;
        } else {
            return value;
        }
    }

    float repeat(float coord, float inverse_dimension, float dimension) {
        coord *= inverse_dimension;
        coord -= floorf(coord);
        coord -= floorf(coord);
        coord *= dimension;
        return coord;
    }

    float mirror(float coord, float inverse_dimension, float dimension) {
        coord *= inverse_dimension;
        if ((int)(coord) % 2) {
            coord -= floorf(coord);
            coord -= floorf(coord);
            coord = 1 - coord;
        } else {
            coord -= floorf(coord);
            coord -= floorf(coord);
        }
        return coord * dimension;
    }

    private:
        const GBitmap bit_map;
        float inverse_width;
        float inverse_height;
        GMatrix local_matrix;
        GShader::TileMode tile_mode;
        GMatrix ctm;
};

/**
 *  Return a subclass of GShader that draws the specified bitmap and the local matrix.
 *  Returns null if the either parameter is invalid.
 */
std::unique_ptr<GShader> GCreateBitmapShader(const GBitmap& bit_map, const GMatrix& localMatrix) {
    return std::unique_ptr<GShader>(new CanvasShader(bit_map, localMatrix)); 
}

/**
 *  Return a subclass of GShader that draws the specified bitmap and a local matrix.
 *  Returns null if the either parameter is invalid.
 */
std::unique_ptr<GShader> GCreateBitmapShader(const GBitmap& bit_map, const GMatrix& localMatrix, GShader::TileMode tile_mode) {
    return std::unique_ptr<GShader>(new CanvasShader(bit_map, localMatrix, tile_mode));
}