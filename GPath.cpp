#include <GPath.h>
#include <GMatrix.h>
#include <Bezier.h>
class GMatrix;

/**
 *  Append a new contour, made up of the 4 points of the specified rect, in the specified
 *  direction. The contour will begin at the top-left corner of the rect.
 */
GPath& GPath::addRect(const GRect& rect, Direction direction) {
    // Save pts and verbs depending on direction.
    if (direction == Direction::kCW_Direction) {
        // Move to top left.
        moveTo(GPoint::Make(rect.left(), rect.top()));
        // Line to top right.
        lineTo(GPoint::Make(rect.right(), rect.top()));
        // Line to bottom right.
        lineTo(GPoint::Make(rect.right(), rect.bottom()));
        // Line to bottom left.
        lineTo(GPoint::Make(rect.left(), rect.bottom()));
    } else {
        // Move to top left.
        moveTo(GPoint::Make(rect.left(), rect.top()));
        // Line to top left.
        lineTo(GPoint::Make(rect.left(), rect.bottom()));
        // Line to bottom right.
        lineTo(GPoint::Make(rect.right(), rect.bottom()));
        // Line to top right.
        lineTo(GPoint::Make(rect.right(), rect.top()));
    }
    return *this;
}

/**
 *  Append a new contour with the specified polygon. Calling this is equivalent to calling
 *  moveTo(pts[0]), lineTo(pts[1..count-1]).
 */
GPath& GPath::addPolygon(const GPoint pts[], int count) {
    // Initial moveTo.
    moveTo(pts[0]);

    // Moving to the other points.
    for (int i = 1; i < count; i++) {
        lineTo(pts[i]);
    }
    return *this;
}

/**
 *  Return the bounds of all of the control-points in the path.
 *
 *  If there are no points, return {0, 0, 0, 0}
 */
GRect GPath::bounds() const {
    // Return an empty rectangle if no points exist.
    if (fPts.empty()) return GRect::MakeXYWH(0, 0, 0, 0);

    // Find the bounds of the rectangle and return it.
    float min_x = 0;
    float max_x = 0;
    float min_y = 0;
    float max_y = 0;

    for (int i = 0; i < fPts.size(); i++) {
        if (min_x > fPts[i].fX) min_x = fPts[i].fX;
        if (max_x < fPts[i].fX) max_x = fPts[i].fX;
        if (min_y > fPts[i].fY) min_y = fPts[i].fY;
        if (max_y < fPts[i].fY) max_x = fPts[i].fY;
    }
    return GRect::MakeLTRB(min_x, max_y, max_x, min_y);
}

/**
 *  Transform the path in-place by the specified matrix.
 */
void GPath::transform(const GMatrix& matrix) {
    for (int i = 0; i < fPts.size(); i++) {
        fPts[i] = matrix * fPts[i];
    }
}

/**
 *  Given 0 < t < 1, subdivide the src[] quadratic bezier at t into two new quadratics in dst[]
 *  such that
 *  0...t is stored in dst[0..2]
 *  t...1 is stored in dst[2..4]
 */
void GPath::ChopQuadAt(const GPoint src[3], GPoint dst[5], float t) {
    // Initializing points.
    GPoint A = src[0];
    GPoint B = src[1];
    GPoint C = src[2];

    // Defining additional points.
    GPoint AB = (1 - t)*A + B*t;
    GPoint BC = (1 - t)*B + C*t;

    // 0... t - 1.
    dst[0] = A;
    dst[1] = AB;
    // t is the shared point.
    dst[2] = calculateQuad(A, B, C, t);
    // t + 1...1.
    dst[3] = BC;
    dst[4] = C;
}

/**
 *  Given 0 < t < 1, subdivide the src[] cubic bezier at t into two new cubics in dst[]
 *  such that
 *  0...t is stored in dst[0..3]
 *  t...1 is stored in dst[3..6]
 */
void GPath::ChopCubicAt(const GPoint src[4], GPoint dst[7], float t) {
    // Initializing points.
    GPoint A = src[0];
    GPoint B = src[1];
    GPoint C = src[2];
    GPoint D = src[3];

    // Defining additional points.
    GPoint AB = (1 - t)*A + B*t;
    GPoint BC = (1 - t)*B + C*t;
    GPoint CD = (1 - t)*C + D*t;

    // 0... t - 1.
    dst[0] = A;
    dst[1] = AB;
    dst[2] = (1 - t)*AB + BC*t;
    // t is the shared point.
    dst[3] = calculateCubic(A, B, C, D, t);
    // t + 1...1.
    dst[4] = (1 - t)*BC + CD*t;
    dst[5] = BC;
    dst[6] = D;
}

/**
 *  Append a new contour respecting the Direction. The contour should be an approximate
 *  circle (8 quadratic curves will suffice) with the specified center and radius.
 */
GPath& GPath::addCircle(GPoint center, float radius, Direction direction) {
    // Constant x/y values used throughout the calculations.
    const float h = tan(M_PI/8);
    const float s = sqrt(2)*0.5f;

    // Transformation matrix.
    GMatrix m = {
        radius,      0, center.fX,
        0,      radius, center.fY
    };

    /**
     * Point A is any combination of: (0, 0), (1, 0), (0, 1), (1, 1).
     * Point B is the 'pivot' point that lays outside the unit circle.
     * Point C is any combination of: (s, s), (s, -s), (-s, s), (-s, -s).
     */
    GPoint A = m*GPoint::Make(1, 0);
    GPoint B;
    GPoint C;
    moveTo(A);
    
    if (direction == Direction::kCW_Direction) {
        // Quadrant I    (+, +).
        C = m*GPoint::Make(s, s);
        B = m*GPoint::Make(1, h);
        quadTo(B, C);

        A = m*GPoint::Make(0, 1);
        B = m*GPoint::Make(h, 1);
        quadTo(B, A);

        // Quadrant II   (-, +).
        C = m*GPoint::Make(-s, s);
        B = m*GPoint::Make(-h, 1);
        quadTo(B, C);

        A = m*GPoint::Make(-1, 0);
        B = m*GPoint::Make(-1, h);
        quadTo(B, A);

        // Quadrant III. (-, -)
        C = m*GPoint::Make(-s, -s);
        B = m*GPoint::Make(-1, -h);
        quadTo(B, C);

        A = m*GPoint::Make(0, -1);
        B = m*GPoint::Make(-h, -1);
        quadTo(B, A);

        // Quadrant IV.  (+, -)
        C = m*GPoint::Make(s, -s);
        B = m*GPoint::Make(h, -1);
        quadTo(B, C);

        A = m*GPoint::Make(1, 0);
        B = m*GPoint::Make(1, -h);
        quadTo(B, A);
    } else {
        // Quadrant IV.  (+, -)
        C = m*GPoint::Make(s, -s);
        B = m*GPoint::Make(1, -h);
        quadTo(B, C);

        A = m*GPoint::Make(0, -1);
        B = m*GPoint::Make(h, -1);
        quadTo(B, A);

        // Quadrant III. (-, -)
        C = m*GPoint::Make(-s, -s);
        B = m*GPoint::Make(-h, -1);
        quadTo(B, C);

        A = m*GPoint::Make(-1,  0);
        B = m*GPoint::Make(-1, -h);
        quadTo(B, A);

        // Quadrant II   (-, +).
        C = m*GPoint::Make(-s, s);
        B = m*GPoint::Make(-1, h);
        quadTo(B, C);

        A = m*GPoint::Make( 0, 1);
        B = m*GPoint::Make(-h, 1);
        quadTo(B, A);

        // Quadrant I    (+, +).
        C = m*GPoint::Make(s, s);
        B = m*GPoint::Make(h, 1);
        quadTo(B, C);

        A = m*GPoint::Make(1, 0);
        B = m*GPoint::Make(1, h);
        quadTo(B, A);
    }
    return *this;
}