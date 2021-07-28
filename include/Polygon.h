#include <GPoint.h>
#include <vector>
#include <GBitmap.h>

class GBitmap;
class GPoint;
float calculate_x(float slope, GPoint point);
float is_horizontal(float y1, float y2);

// Edge struct for polygons.
struct Edge {
    int   min_y;      // minimum height of the edge.
    int   max_y;      // maximum height of the edge.
    int   w;          // winding value.
    float m;          // Δx / Δy.
    float x;          // curr_x.

    // Constructor.
    Edge(float _min_y, float _max_y, float _m, float _x, int _w, const GBitmap& bit_map) {
        min_y = GRoundToInt(_min_y);
        max_y = GRoundToInt(_max_y);
        m = _m;
        x = _x;
        w = _w;
        assert(min_y < max_y);
        assert(0 <= min_y && max_y <= bit_map.height());
        assert(0 <= x && x <= (float) bit_map.width());
    }

    // Checks if [y] is in y-range of the edge.
    bool legal_y(int y) {
        return min_y <= y && y < max_y;
    }
};

/**
 * Takes an array of pts and creates Edges/places them into an Edge vector.
 * Clips any out-of-bound (OOB) points and creates their respective border edge.
 */
void find_edges(std::vector<Edge> &edges, const GPoint pts[], int count, const GBitmap &bit_map, bool connect_end = true) {
    // Iterating through the number of edges in the polygon and placing them into [edges].
    for (int i = 0; i < count; i++) {
        // Index of the next point.
        int next_i = ( i + 1 >= count && connect_end ) 
            ? 0
            : i + 1;

        // Checking if the edge is horizontal.
        if (is_horizontal(pts[i].fY, pts[next_i].fY)) continue;

        // Finding slope, b constant, and winding value.
        float m = (pts[next_i].fX - pts[i].fX) / (pts[next_i].fY - pts[i].fY);
        float b = pts[i].fX - m*pts[i].fY;
        int   w = pts[next_i].fY > pts[i].fY ? 1 : -1;

        // Organize pts from top to bottom.
        GPoint top_point    = pts[i].fY < pts[next_i].fY ? pts[i] : pts[next_i];
        GPoint bottom_point = pts[i].fY < pts[next_i].fY ? pts[next_i] : pts[i];

        // Skip edge if both points are above/under the bit_map.
        if ( (top_point.fY < 0 && bottom_point.fY < 0) ||
             (top_point.fY > bit_map.height() && bottom_point.fY > bit_map.height()) ) continue;

        // Bring the upper OOB point to the border.
        if (top_point.fY < 0) {
            top_point = GPoint::Make(b, 0);
        }

        // Bring the lower OOB point to the border.
        if (bottom_point.fY > bit_map.height()) {
            bottom_point = GPoint::Make(m * bit_map.height() + b, bit_map.height());
        }

        // Check for a horizontal edge.
        if (is_horizontal(top_point.fY, bottom_point.fY)) continue;

        // Case where both points are OOB.
        if ((top_point.fX < 0 && bottom_point.fX < 0) || (top_point.fX > bit_map.width() && bottom_point.fX > bit_map.width())) {
            int bounding_x = top_point.fX < 0
                ? 0
                : bit_map.width();
            
            Edge border_edge = Edge(top_point.fY, bottom_point.fY, 0, bounding_x, w, bit_map);
            edges.push_back(border_edge);
            continue;
        }

        // Sort pts from left to right.
        GPoint left_point  = top_point.fX > bottom_point.fX ? bottom_point : top_point;
        GPoint right_point = top_point.fX > bottom_point.fX ? top_point : bottom_point;

        // Creating a border edge if [left_point] is OOB.
        if (left_point.fX < 0) {
            // Finding the border edge's min/max y's.
            float left_intersection = -b / m;
            float edge_min_y = std::min(left_point.fY, left_intersection);
            float edge_max_y = std::max(left_point.fY, left_intersection);

            // Inserting an edge.
            if (!is_horizontal(edge_min_y, edge_max_y)) {
                Edge border_edge = Edge(edge_min_y, edge_max_y, 0, 0, w, bit_map);
                edges.push_back(border_edge);
            }

            // The point on the left border is the new left point.
            left_point = GPoint::Make(0, left_intersection);
        }

        // Creating a border edge if [right_point] is OOB.
        if (right_point.fX > bit_map.width()) {
            float right_intersection = (bit_map.width() - b) / m;
            // Finding the border edge's min/max y's.
            float edge_min_y = std::min(right_point.fY, right_intersection);
            float edge_max_y = std::max(right_point.fY, right_intersection);

            // Inserting an edge.
            if (!is_horizontal(edge_min_y, edge_max_y)) {
                Edge border_edge = Edge(edge_min_y, edge_max_y, 0, bit_map.width(), w, bit_map);
                edges.push_back(border_edge);
            }

            // The point on the border is now the new right point.
            right_point = GPoint::Make(bit_map.width(), right_intersection);
        }

        // Finding the final edge's min/max y's.
        float min_y = std::min(left_point.fY, right_point.fY);
        float max_y = std::max(left_point.fY, right_point.fY);

        // Checking if the edge is horizontal.
        if (is_horizontal(min_y, max_y)) continue;

        // Calculating Edge.x
        top_point = left_point.fY == min_y ? left_point : right_point;
        float x = calculate_x(m, top_point);

        // Creating and inserting the final edge.
        Edge edge = Edge(min_y, max_y, m, x, w, bit_map);
        edges.push_back(edge);
    }
}

// Takes an endpoint of an edge and calculates its x value using the given slope.
float calculate_x(float slope, GPoint point) {
    float h = round(point.fY) - point.fY + 0.5;
    return point.fX + slope*h;
}

float is_horizontal(float y1, float y2) {
    return GRoundToInt(y1) == GRoundToInt(y2);
}

// Sorting function for edges.
bool sortEdges(const Edge& edge1, const Edge& edge2) {
    // Sort by y.
    if (edge1.min_y < edge2.min_y) {
        return true;

    } else if (edge2.min_y < edge1.min_y) {
        return false;
    }

    // Sort by x if y are equal.
    if (edge1.x < edge2.x) {
        return true;

    } else if (edge2.x < edge1.x) {
        return false;
    }

    // Determining via slope.
    return edge1.m < edge2.m;
}

// Sorting function for edges.
bool sortEdgesByX(const Edge& edge1, const Edge& edge2) {
    return edge1.x < edge2.x;
}

/**
 * Creates a point on a polygon.
 * 
 * P = (1 - u)*(1 - v)*A +
 *           u*(1 - v)*B +
 *                 u*v*C +
 *           v*(1 - u)*D
 */
GPoint pointAt(GPoint A, GPoint B, GPoint C, GPoint D, float u, float v) {
    return (1 - u) * (1 - v) * A +
                u  * (1 - v) * B +
                       u * v * C +
                v  * (1 - u) * D ;
}

GColor colorAt(GColor color1, GColor color2, GColor color3, GColor color4, float u, float v) {
    float coeff1 = (1 - u) * (1 - v);
    float coeff2 = u * (1 - v);
    float coeff3 = u * v;
    float coeff4 = v * (1 - u);

    return GColor::MakeARGB(
        coeff1 * color1.fA + coeff2 * color2.fA + coeff3 * color3.fA + coeff4 * color4.fA,
        coeff1 * color1.fR + coeff2 * color2.fR + coeff3 * color3.fR + coeff4 * color4.fR,
        coeff1 * color1.fG + coeff2 * color2.fG + coeff3 * color3.fG + coeff4 * color4.fG,
        coeff1 * color1.fB + coeff2 * color2.fB + coeff3 * color3.fB + coeff4 * color4.fB
    );
}

