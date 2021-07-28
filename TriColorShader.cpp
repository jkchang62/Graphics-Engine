#include <GPoint.h>
#include <GColor.h>
#include <GMatrix.h>
#include <GShader.h>

GPixel color_to_pixel(GColor color);

/**
 * This shader's job is to take three points, each with a color payload.
 * The shader's shadeRow will fill a triangle with colors that are dependent
 * on the distance between each color payload.
 * 
 */
class TriColorShader: public GShader {
    public:

    // Constructor.
    TriColorShader(const GPoint _pts[3], const GColor _colors[3]) : pts(_pts), colors(_colors) {
        // Defining points.
        GPoint p0 = _pts[0];
        GPoint p1 = _pts[1];
        GPoint p2 = _pts[2];

        // Creating the local matrix (P).
        local_matrix = {
            (p1-p0).fX, (p2-p0).fX, p0.fX,
            (p1-p0).fY, (p2-p0).fY, p0.fY
        };
    }

    // Return true iff all of the GPixels that may be returned by this shader will be opaque.
    bool isOpaque() {
        for (int i = 0; i < 3; i++) {
            if (!(colors + i)->fA) {
                return false;
            }
        }
        return true;
    }

    // The draw calls in GCanvas must call this with the CTM before any calls to shadeSpan().
    bool setContext(const GMatrix& new_ctm) {
        // Finding the determinant.
        float det = new_ctm[0] * new_ctm[4] - new_ctm[1] * new_ctm[3];

        // Rejecting ctm.
        if (det == 0) return false;

        // Setting the inverse matrix.
        inverse_matrix = new_ctm * local_matrix;
        inverse_matrix.invert(&inverse_matrix);

        // Defining colors.
        GColor c0 = colors[0];
        GColor c1 = colors[1];
        GColor c2 = colors[2];

        // Defining coefficents.
        float a = inverse_matrix[0];
        float d = inverse_matrix[3];

        // Δcolor = (-a - d)*c0 + a*c1 + d*c2.
        float A = (-a - d) * c0.fA + a*c1.fA + d*c2.fA;
        float R = (-a - d) * c0.fR + a*c1.fR + d*c2.fR;
        float G = (-a - d) * c0.fG + a*c1.fG + d*c2.fG;
        float B = (-a - d) * c0.fB + a*c1.fB + d*c2.fB;
        delta_color = GColor::MakeARGB(A, R, G, B);

        // Setting new ctm and returning true.
        ctm = new_ctm;
        return true;
    }

    void shadeRow(int x, int y, int count, GPixel row[]) {
        // Defining colors.
        GColor c0 = colors[0];
        GColor c1 = colors[1];
        GColor c2 = colors[2];

        // Initializing x and y.
        float fx = x + 0.5f;
        float fy = y + 0.5f;

        // Creating the initial point.
        GPoint P = inverse_matrix * GPoint::Make(fx, fy);

        // Barycentric coordinates.
        // C_initial = (1 - Px - Py) * C0 + Py*C1 + Px*C2
        float A = (1 - P.fX - P.fY) * c0.fA + P.fX*c1.fA + P.fY*c2.fA;
        float R = (1 - P.fX - P.fY) * c0.fR + P.fX*c1.fR + P.fY*c2.fR;
        float G = (1 - P.fX - P.fY) * c0.fG + P.fX*c1.fG + P.fY*c2.fG;
        float B = (1 - P.fX - P.fY) * c0.fB + P.fX*c1.fB + P.fY*c2.fB;
        GColor c = GColor::MakeARGB(A, R, G, B);

        for (int i = 0; i < count; i++) {
            // Saving new pixel.
            row[i] = color_to_pixel(c);

            // Incrementing.
            c = GColor::MakeARGB(
                delta_color.fA + c.fA,
                delta_color.fR + c.fR,
                delta_color.fG + c.fG,
                delta_color.fB + c.fB
            );
        }
    }

    private:
        const GPoint* pts;
        const GColor* colors;
        GMatrix local_matrix;
        GMatrix inverse_matrix;
        GMatrix ctm;
        GColor delta_color;
};

// Takes a color and transforms it into a premul pixel.
GPixel color_to_pixel(GColor color) {
    return GPixel_PackARGB(
        color.fA * 255 + 0.5,
        color.fA * color.fR * 255 + 0.5,
        color.fA * color.fG * 255 + 0.5,
        color.fA * color.fB * 255 + 0.5
    );
}

/**
 *  Return a subclass of GShader that takes a triangle with color payload and draws them.
 */
std::unique_ptr<GShader> GCreateTriColorShader(const GPoint _pts[3], const GColor _colors[3]) {
    return std::unique_ptr<GShader>(new TriColorShader(_pts, _colors)); 
}
