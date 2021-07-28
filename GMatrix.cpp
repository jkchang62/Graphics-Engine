#include <GMatrix.h>

// Initialize to identity matrix.
GMatrix::GMatrix() : 
fMat{
    1.0, 0.0, 0.0,
    0.0, 1.0, 0.0
} {};

/*
* Returns a matrix with the given translation.
*
* tx: float that translates the matrix's tx.
*
* ty: float that translates the matrix's ty.
*/
GMatrix GMatrix :: Translate(float tx, float ty) {
    return GMatrix(
        1, 0, tx,
        0, 1, ty
    );
}

/*
* Returns a matrix with the given scaling.
*
* sx: float that scales the matrix's sx.
*
* sy: float that scales the matrix's sy.
*/
GMatrix GMatrix :: Scale(float sx, float sy) {
    return GMatrix(
        sx, 0, 0,
        0, sy, 0
    );
}

/*
* Rotates a matrix by some radian.
*
* radians: float to rotate the matrix by.
*/
GMatrix GMatrix :: Rotate(float radians) {
    return GMatrix( 
        cos(radians), -sin(radians), 0,
        sin(radians),  cos(radians), 0
    );
}

/*
* Concatenates two matrices via matrix multiplication.
*/
GMatrix GMatrix :: Concat(const GMatrix& a, const GMatrix& b) {
    return GMatrix(
        a[0]*b[0] + a[1]*b[3], a[0]*b[1] + a[1]*b[4], a[0]*b[2] + a[1]*b[5] + a[2],
        a[3]*b[0] + a[4]*b[3], a[3]*b[1] + a[4]*b[4], a[3]*b[2] + a[4]*b[5] + a[5]
    );
}

/*
*  If this matrix is invertible, return true and (if not null) set the inverse parameter.
*  If this matrix is not invertible, return false and ignore the inverse parameter.
*
 *  [ SX KX TX ]             [ SX KY 0 ]               [  SX  -KY  0 ]
 *  [ KY SY TY ] ----------> [ KX SY 0 ] ------------> [ -KX   SY  0 ]
 *  [ 0  0  1  ]  adjugate   [ TX TY 1 ]  coefficient  [  TX  -TY  1 ]
*/
bool GMatrix :: invert(GMatrix* inverse) const {
    // Find determinate first before doing anymore work.
    double det = fMat[SX] * fMat[SY] - fMat[KX] * fMat[KY];
    if (det == 0) return false;

    // Inverse determinate to find the inverse matrix.
    double inverse_det = 1 / det;

    // Finding new elements for the inverse matrix.
    float a =  fMat[SY] * inverse_det;
    float b = -fMat[KX] * inverse_det;
    float c = (fMat[KX] * fMat[TY] - fMat[SY] * fMat[TX]) * inverse_det;
    float d = -fMat[KY] * inverse_det;
    float e =  fMat[SX] * inverse_det;
    float f = (fMat[KY] * fMat[TX] - fMat[SX] * fMat[TY]) * inverse_det;

    // Setting inverse and returning true.
    *inverse = GMatrix(a, b, c, d, e, f);
    return true;
}

/**
 *  Transform the set of points in src, storing the resulting points in dst, by applying this
 *  matrix. 
 * 
 *  *It is the caller's responsibility to allocate dst to be at least as large as src.
 *
 *  Note: It is legal for src and dst to point to the same memory (however, they may not
 *  partially overlap). Thus the following is supported.
 *
 *  GPoint pts[] = { ... };
 *  matrix.mapPoints(pts, pts, count);
 *
 *  [ SX KX TX ] * [ x ]   [SX*x + KX*y + TX]
 *  [ KY SY TY ]   [ y ] = [KY*x + SY*y + TY]
 *  [ 0  0  1  ]   [ 1 ]   [       1        ]
 */
void GMatrix :: mapPoints(GPoint dst[], const GPoint src[], int count) const {
    // Iterating and applying the appropriate transformations to each point.
    for (int i = 0; i < count; i++) {
        float x =  fMat[GMatrix::SX] * src[i].fX + fMat[GMatrix::KX] * src[i].fY + fMat[GMatrix::TX];
        float y =  fMat[GMatrix::KY] * src[i].fX + fMat[GMatrix::SY] * src[i].fY + fMat[GMatrix::TY];
        dst[i].set(x, y);
    }
}