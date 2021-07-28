#include <GPoint.h>
#include <GColor.h>
#include <GMatrix.h>
#include <GShader.h>

static inline int Div255(int x) {
    return ((x + 128) * 257) >> 16;
}

/**
 * Takes two pixels and modulates them together by multiplying
 * their components.
 */
static GPixel modulate(GPixel pixel1, GPixel pixel2) {
    int A = Div255(GPixel_GetA(pixel1) * GPixel_GetA(pixel2));
    int R = Div255(GPixel_GetR(pixel1) * GPixel_GetR(pixel2));
    int G = Div255(GPixel_GetG(pixel1) * GPixel_GetG(pixel2));
    int B = Div255(GPixel_GetB(pixel1) * GPixel_GetB(pixel2));
    return GPixel_PackARGB(A, R, G, B);
}


/**
 * This shader's job is to work as proxy between two other shaders,
 * [CanvasShader] and a [CanvasGradient]. This shader's shadeRow will
 * call each of these subclasses' shadeRow and transform the returned pixels
 * into one pixel.
 */
class ComposeShader: public GShader {
    public:
    
    // Constructor.
    ComposeShader(GShader* _colorShader, GShader* _gradientShader) : colorShader(_colorShader), gradientShader(_gradientShader) {}

    // Return true iff all of the GPixels that may be returned by this shader will be opaque.
    bool isOpaque() {
        return colorShader->isOpaque() && gradientShader->isOpaque();
    }

    // The draw calls in GCanvas must call this with the CTM before any calls to shadeSpan().
    bool setContext(const GMatrix& new_ctm) {
        return colorShader->setContext(new_ctm) && gradientShader->setContext(new_ctm);
    }

    /**
     *  Given a row of pixels in device space [x, y] ... [x + count - 1, y], return the
     *  corresponding src pixels in row[0...count - 1]. The caller must ensure that row[]
     *  can hold at least [count] entries.
     */
    void shadeRow(int x, int y, int count, GPixel row[]) {
        // Creating two arrays of colors.
        GPixel tmp[count];
        colorShader->shadeRow(x, y, count, tmp);
        gradientShader->shadeRow(x, y, count, row);

        // Combining two pixels together to produce a single pixel to save to row.
        for (int i = 0; i < count; i++) {
            row[i] = modulate(tmp[i], row[i]);
        }
    }

    private:
        GShader* colorShader;
        GShader* gradientShader;
};

/**
 *  Return a subclass of GShader that takes two shaders and blends their pixels.
 */
std::unique_ptr<GShader> GCreateComposeShader(GShader* _colorShader, GShader* _gradientShader) {
    return std::unique_ptr<GShader>(new ComposeShader(_colorShader, _gradientShader)); 
}