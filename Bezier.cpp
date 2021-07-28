#include <Bezier.h>
#include <GPoint.h>

/**
 * Calculates #segments for quadratic bezier curves.
 * T = 0.25. 
 */ 
int quadSegments(GPoint pts[]) {
    // Defining pts.
    GPoint A = pts[0];
    GPoint B = pts[1];
    GPoint C = pts[2];

    // E = (-A + 2B - C) / 4.
    GVector E = (-1*A + 2*B - C) * 0.25;

    // #segments = ceil(sqrt(E/T))
    return ceil(2*sqrt(E.length()));
}

/**
 * Calculates #segments for cubic bezier curves.
 * T = 0.25. 
 */
int cubicSegments(GPoint pts[]) {
    // Defining pts.
    GPoint A = pts[0];
    GPoint B = pts[1];
    GPoint C = pts[2];
    GPoint D = pts[3];

    // P = -A + 2B - C.
    GPoint P = -1*A + 2*B + C;
    
    // Q = -B + 2C - D.
    GPoint Q = -1*B + 2*C - D;

    /* 
     * E = { E.x, E.y }, where
     * - E.x = max(abs(P.x), abs(Q.x));
     * - E.y = max(abs(P.y), abs(Q.y));
     */
    GVector E;
    float x = std::max(abs(P.fX), abs(Q.fX));
    float y = std::max(abs(P.fY), abs(Q.fY));
    E.set(x, y);

    // #segments = ceil(sqrt(E/T)).
    return ceil(sqrt(3*E.length()));
}

/**
 * Calculates a point on a quadratic bezier curve.
 * 
 * R(t) = (1 - t)^2 * A + 2t(1 -t) * B + t^2 * C.
 */
GPoint calculateQuad(GPoint A, GPoint B, GPoint C, float t) {
    return pow(1 - t, 2)*A + 2*t*(1 - t)*B + pow(t, 2)*C;
}

/**
 * Calculates a point on a cubic bezier curve.
 * 
 * R(t) = (1 - t)^3 * A + 3t(1 - t)^2 * B + 3*t^2 * (1 - t) * C + t^3 * D.
 */
GPoint calculateCubic(GPoint A, GPoint B, GPoint C, GPoint D, float t) {
    return pow(1 - t, 3)*A + 3*t*pow(1 - t, 2)*B + 3*pow(t, 2)*(1 - t)*C + pow(t, 3)*D;
}

/**
 * Creates the points on a quadratic bezier and returns them.
 */
void createQuadPts(GPoint pts[], GPoint quadPts[], int count) {
    // Points.
    GPoint A = pts[0];
    GPoint B = pts[1];
    GPoint C = pts[2];

    // Updating [quadPts] for each point on the curve.
    quadPts[0] = A;
    float inverse_count = pow(count, -1);
    for (int i = 1; i < count; i++) {
        // Inserting point.
        float t = i * inverse_count;
        GPoint new_point = calculateQuad(A, B, C, t);
        quadPts[i] = new_point;
    }
    quadPts[count] = C;
}

/**
 *  Creates the points on a cubic bezier and returns them.
 */
void createCubicPts(GPoint pts[], GPoint cubicPts[], int count) {
    // Points.
    GPoint A = pts[0];
    GPoint B = pts[1];
    GPoint C = pts[2];
    GPoint D = pts[3];

    // Updating [cubicPts] for each point on the curve.
    cubicPts[0] = A;
    float inverse_count = pow(count, -1);
    for (int i = 1; i < count; i++) {
        // Inserting point.
        float t = i * inverse_count;
        GPoint new_point = calculateCubic(A, B, C, D, t);
        cubicPts[i] = new_point;
    }
    cubicPts[count] = D;
}