#include <GPoint.h>
#include <GColor.h>
#include <GMatrix.h>
#include <GShader.h>

/**
 * This shader's job is to call the realShader, but with a modified CTM so
 * that the shader sees the effect of our P matrix and the requested S
 * coordinates.
 */
class ProxyShader: public GShader {
    public:

    // Constructor.
    ProxyShader(GShader* _realShader, const GPoint _pts[3], const GPoint _coords[3]) :
        realShader(_realShader), pts(_pts), coords(_coords) {}

    // Return true iff all of the GPixels that may be returned by this shader will be opaque.
    bool isOpaque() {
        return realShader->isOpaque();
    }

    // The draw calls in GCanvas must call this with the CTM before any calls to shadeSpan().
    // Psuedo matrix needs to be inserted into the realShader.
    bool setContext(const GMatrix& new_ctm) {
        // Finding the determinant.
        float det = new_ctm[0] * new_ctm[4] - new_ctm[1] * new_ctm[3];

        // Rejecting ctm.
        if (det == 0) return false;

        // Defining points.
        GPoint p0 = pts[0];
        GPoint p1 = pts[1];
        GPoint p2 = pts[2];

        GPoint s0 = coords[0];
        GPoint s1 = coords[1];
        GPoint s2 = coords[2];

        // Creating the P matrix.
        GMatrix P = {
            (p1-p0).fX, (p2-p0).fX, p0.fX,
            (p1-p0).fY, (p2-p0).fY, p0.fY
        };

        // Creating the S^-1 matrix.
        GMatrix S = {
            (s1-s0).fX, (s2-s0).fX, s0.fX,
            (s1-s0).fY, (s2-s0).fY, s0.fY
        };
        S.invert(&S);

        // Setting realShader's ctm to the psuedo ctm.
        return realShader->setContext(new_ctm * P * S);
    }

    /**
     *  Given a row of pixels in device space [x, y] ... [x + count - 1, y], return the
     *  corresponding src pixels in row[0...count - 1]. The caller must ensure that row[]
     *  can hold at least [count] entries.
     */
    void shadeRow(int x, int y, int count, GPixel row[]) {
        realShader->shadeRow(x, y, count, row);
    }

    private:
        GShader* realShader;
        const GPoint* pts;
        const GPoint* coords;
};

/**
 *  Return a subclass of GShader that proxies a real shader.
 */
std::unique_ptr<GShader> GCreateProxyShader(GShader* _realShader, const GPoint _pts[3], const GPoint _coords[3]) {
    return std::unique_ptr<GShader>(new ProxyShader(_realShader, _pts, _coords)); 
}