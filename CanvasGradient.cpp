#include <GShader.h>
#include <GMatrix.h>
#include <GBitmap.h>
#include <GColor.h>

float mirror(float x);
float repeat(float x);

class CanvasGradient : public GShader {
    public:

    // Constructor.
    CanvasGradient(GPoint p0, GPoint p1, const GColor _colors[], int _count, GShader::TileMode mode = GShader::kClamp) : n(_count), tile_mode(mode) {
        // Copying the given colors into the local color array.
        colors = new GColor[_count];
        for (int i = 0; i < n; i++) {
            colors[i] = _colors[i];
        }

        // Finding dx and dy.
        float dx = p1.fX - p0.fX;
        float dy = p1.fY - p0.fY;
        
        // Creating the local matrix: R*S*T.
        local_matrix = {
            dx, -dy, p0.fX,
            dy,  dx, p0.fY
        };
    }

    // Return true iff all of the GPixels that may be returned by this shader will be opaque.
    bool isOpaque() override {
        for (int i = 0; i < n; i++) {
            if (!(colors + i)->fA) {
                return false;
            }
        }
        return true;
    }

    // The draw calls in GCanvas must call this with the CTM before any calls to shadeRow().
    // Returns true if the matrix invertible and false if it's not.
    bool setContext(const GMatrix& new_ctm) {
        // Finding the determinant.
        float det = new_ctm[0] * new_ctm[4] - new_ctm[1] * new_ctm[3];

        // Setting a new ctm or just returning false.
        if (det != 0) {
            ctm = new_ctm;
            return true;
        } else {
            return false;
        }
    }

    /**
     *  Given a row of pixels in device space [x, y] ... [x + count - 1, y], return the
     *  corresponding colors in row[0...count - 1]. The caller must ensure that row[]
     *  can hold at least [count] entries.
     */
    void shadeRow(int x, int y, int count, GPixel row[]) override {
        // Invariant fixes.
        GMatrix inverse_matrix = ctm * local_matrix;
        inverse_matrix.invert(&inverse_matrix);

        // Iterating values.
        float fx = x + 0.5f;
        float fy = y + 0.5f;

        // Iterate through the row.
        for (int i = 0; i < count; i++) {
            // Finding the new point.
            GPoint P = inverse_matrix * GPoint::Make(fx, fy);

            // Mirror/repeat.
            if (tile_mode == kMirror) {
                P.set(mirror(P.fX), y);
            } else if (tile_mode == kRepeat) {
                P.set(repeat(P.fX), y);
            }

            // Clamping.
            if (P.fX >= 1) {
                P.set(0.9999999, P.fY);
            } else if (P.fX < 0) {
                P.set(0.0000000, P.fY);
            }

            // Calculating [t] and the new color.
            float t = P.fX * (n - 1);
            int index = int(t);
            t -= index;
            GColor color = mixColors(1 - t, t, colors[index], colors[index + 1]);
            
            // Transforming the color into a premul pixel and inserting it into [row].
            GPixel colored_pixel = color_to_pixel(color);
            row[i] = colored_pixel;

            // Incrementing [fX].
            fx += 1;
        }
    }

    // Returns a new color after multiplying its two colors' [A,R,G,B] by their respective factor.
    GColor mixColors(float factor1, float factor2, GColor color1, GColor color2) {
        return GColor::MakeARGB(
            color1.fA * factor1 + color2.fA * factor2,
            color1.fR * factor1 + color2.fR * factor2,
            color1.fG * factor1 + color2.fG * factor2,
            color1.fB * factor1 + color2.fB * factor2 
        );
    }

    // Takes a color and transforms it into a premul pixel.
    GPixel color_to_pixel(GColor color) {
        return GPixel_PackARGB(
            color.fA * 255 + 0.5,
            color.fA * color.fR * 255 + 0.5,
            color.fA * color.fG * 255 + 0.5,
            color.fA * color.fB * 255 + 0.5
        );
    }

    private:
        const int n;
        GColor* colors;
        GMatrix ctm;
        GMatrix local_matrix;
        GShader::TileMode tile_mode;
};

float mirror(float x) {
    x -= floorf(x);
    x -= floorf(x);
    if (GRoundToInt(x) % 2) {
        return 1 - x;
    } else {
        return x;
    }
}

float repeat(float x) {
    x -= floorf(x);
    x -= floorf(x);
    return x;
}

/**
 *  Return a subclass of GShader that draws the specified gradient of [count] colors between
 *  the two points. Color[0] corresponds to p0, and Color[count-1] corresponds to p1, and all
 *  intermediate colors are evenly spaced between.
 *
 *  The gradient colors are GColors, and therefore unpremul. The output colors (in shadeRow)
 *  are GPixel, and therefore premul. The gradient has to interpolate between pairs of GColors
 *  which it needs to do before "pre" multiplying.
 *
 *  If count == 1, the returned shader just draws a single color everywhere.
 *  If count < 1, this should return nullptr.
 */
std::unique_ptr<GShader> GCreateLinearGradient(GPoint p0, GPoint p1, const GColor colors[], int count) {
    // Quick check.
    if (count < 1) return nullptr;
    return std::unique_ptr<GShader>(new CanvasGradient(p0, p1, colors, count)); 
}

std::unique_ptr<GShader> GCreateLinearGradient(GPoint p0, GPoint p1, const GColor colors[], int count, GShader::TileMode tile_mode) {
    if (count < 1) return nullptr;
    return std::unique_ptr<GShader>(new CanvasGradient(p0, p1, colors, count, tile_mode));
}