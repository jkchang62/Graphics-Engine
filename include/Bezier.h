#ifndef BEZIER_H
#define BEZIER_H
#include <GPoint.h>

/**
 * Calculates #segments for quadratic bezier curves.
 * T = 0.25. 
 */ 
int quadSegments(GPoint pts[]);

/**
 * Calculates #segments for cubic bezier curves.
 * T = 0.25. 
 */
int cubicSegments(GPoint pts[]);

/**
 * Calculates a point on a quadratic bezier curve.
 * 
 * R(t) = (1 - t)^2 * A + 2t(1 -t) * B + t^2 * C.
 */
GPoint calculateQuad(GPoint A, GPoint B, GPoint C, float t);

/**
 * Calculates a point on a cubic bezier curve.
 * 
 * R(t) = (1 - t)^3 * A + 3t(1 - t)^2 * B + 3*t^2 * (1 - t) * C + t^3 * D.
 */
GPoint calculateCubic(GPoint A, GPoint B, GPoint C, GPoint D, float t);

/**
 * Creates the points on a quadratic bezier and returns them.
 */
void createQuadPts(GPoint pts[], GPoint quadPts[], int count);

/**
 *  Creates the points on a cubic bezier and returns them.
 */
void createCubicPts(GPoint pts[], GPoint cubicPts[], int count);

#endif