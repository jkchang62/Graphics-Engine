#include <EmptyCanvas.h>
#include <GBitmap.h>
#include <GCanvas.h>
#include <GColor.h>
#include <GRect.h>
#include <GMath.h>
#include <GMatrix.h>
#include <GPaint.h>
#include <GPath.h>
#include <GPixel.h>
#include <Polygon.h>
#include <Blitter.h>
#include <Bezier.h>
#include <stack>
#include <vector> 
#include <GShader.h>
#include <MoreShaders.h>

struct Edge;
class GBitmap;
class GMatrix;
class GPaint;
class GPath;
class GRect;
class Blitter;
enum class GBlendMode;
bool sortEdges(const Edge& edge1, const Edge& edge2);
bool sortEdgesByX(const Edge& edge1, const Edge& edge2);

class EmptyCanvas: public GCanvas {
    public: 

    // Properties.
    bool has_called_save = false;

    // Constructor.
    EmptyCanvas(const GBitmap& device) : bit_map(device) {
        // Initializing the stack with an identity matrix.
        ctm.push(GMatrix());
    };

    /**
     *  Save off a copy of the canvas state (CTM), to be later used if the balancing call to
     *  restore() is made. Calls to save/restore can be nested.
     */
    void save() override {
        has_called_save = true;
        ctm.push(ctm.top());
    }

    /**
     *  Copy the canvas state (CTM) that was record in the correspnding call to save() back into
     *  the canvas. It is an error to call restore() if there has been no previous call to save().
     */
    void restore() override {
        if (has_called_save) {
            has_called_save = false;
            ctm.pop();
        }
    }

    /**
     *  Modifies the CTM by preconcatenating the specified matrix with the CTM. The canvas
     *  is constructed with an identity CTM.
     *
     *  CTM' = CTM * matrix
     */
    void concat(const GMatrix& matrix) override {
        ctm.top().preConcat(matrix);
    }

    // Sets the paint of the canvas with the given paint and blendmode.
    void drawPaint(const GPaint& src) override {
        // Cases where no work needs to be done (just kDst).
        if (!src.getShader() && willReturnDst(src.getBlendMode(), src.getAlpha())) return;

        // Creating blitter.
        Blitter blitter = Blitter(src, bit_map, ctm.top());
        
        // Looping through the entirety of [bit_map] and repainting each pixel.
        for (int y = 0; y < bit_map.height(); y++) {
            blitter.blit(y, 0, bit_map.width() - 1);
        }
    }

    // Fills a rectangular section of the canvas with the given rectangle dimensions
    // and fills the destination pixel with the src color.
    void drawRect(const GRect& rect, const GPaint& src) override {
        // Cases where no work needs to be done (just kDst).
        if (!src.getShader() && willReturnDst(src.getBlendMode(), src.getAlpha())) return;

        // Making a points array to use as an argument for mapPoints.
        const GPoint points[4] = {
            GPoint::Make(rect.fLeft, rect.fTop),      // top left point.
            GPoint::Make(rect.fRight, rect.fTop),     // top right point.
            GPoint::Make(rect.fRight, rect.fBottom),  // bottom right point.
            GPoint::Make(rect.fLeft, rect.fBottom)    // bottom left point.
        };

        // Delegate the task to drawConvexPolygon.
        drawConvexPolygon(points, 4, src);
    }
    
    /*
    * drawConvexPolygon takes a polygon and fills the insides of that polygon with the desired color.
    * 
    * org_points[]: array of points on the polygon's exterior.
    * 
    * count: integer that represents the number of edges the polygon has.
    * 
    * GPaint: GPaint object that the pixels of the polygon will be colored with.
    * 
    */
    void drawConvexPolygon(const GPoint org_points[], int count, const GPaint& src) override {
        // Cases where no work needs to be done (just kDst).
        if (willReturnDst(src.getBlendMode(), src.getAlpha()) && !src.getShader()) return;

        // Mapping each point post-ctm operations and placing them into [points].
        GPoint points[sizeof(GPoint) * count];
        ctm.top().mapPoints(points, org_points, count);

        // Creating edges and sorting them.
        std::vector <Edge> edges;
        find_edges(edges, points, count, bit_map);
        std::sort(edges.begin(), edges.end(), sortEdges);

        // Return if there are no edges to apply.
        if (edges.empty()) return;

        // Two comparison edges.
        Edge e0 = edges[0];
        Edge e1 = edges[1];
        int i = 1;

        // Blitter.
        Blitter blitter = Blitter(src, bit_map, ctm.top());

        // Global min_y and max_y.
        int global_top = GRoundToInt(edges[0].min_y);
        int global_bottom = edges[edges.size() - 1].max_y;

        // Iterating from the top of the polygon to the bottom.
        for (int y = global_top; y < global_bottom; y++) {
            // Asserting y values.
            assert(e0.legal_y(y));
            assert(e1.legal_y(y));

            // Rounding x values and blitting.
            int x0 = GRoundToInt(e0.x);
            int x1 = GRoundToInt(e1.x);
            assert(x0 <= x1);
            blitter.blit(y, x0, x1);

            // Check [e0].
            if (e0.legal_y(y + 1)) {
                e0.x += e0.m;
            } else {
                i++;
                e0 = edges[i];
            }

            // Check [e1].
            if (e1.legal_y(y + 1)) {
                e1.x += e1.m;
            } else {
                i++;
                e1 = edges[i];
            }
        }
    }
    
    /**
     *  Fill the path with the paint, interpreting the path using winding-fill (non-zero winding).
     */
    void drawPath(const GPath& path, const GPaint& src) {
        // Cases where no work needs to be done (just kDst).
        if (willReturnDst(src.getBlendMode(), src.getAlpha()) && !src.getShader()) return;

        // Transforming each point by the current ctm.
        GPath copy_path = path;
        copy_path.transform(ctm.top());

        // Finding edges.
        std::vector <Edge> edges;
        GPath::Edger iter(copy_path);
        for (;;) {
            GPoint pts[GPath::kMaxNextPoints];  // enough storage for each call to next()
            GPath::Verb v = iter.next(pts);
            if (v == GPath::kDone) {
                break;  // we're done with the loop
            }
            int numOfEdges;
            switch (v) {
                case GPath::kLine:
                    find_edges(edges, pts, 1, bit_map, false);
                    break;
                case GPath::kQuad:
                {
                    // Finding the number of edges and creates an array to their points.
                    numOfEdges = quadSegments(pts);
                    GPoint quadPts[numOfEdges + 1];

                    // Finding points on the quadratic bezier curve and creating edges.
                    createQuadPts(pts, quadPts, numOfEdges);
                    find_edges(edges, quadPts, numOfEdges, bit_map, false);
                }
                    break;

                case GPath::kCubic:
                {
                    // Finding the number of edges and creates an array to their points.
                    numOfEdges = cubicSegments(pts);
                    GPoint cubicPts[numOfEdges + 1];

                    // Finding points on the cubic bezier curve and creating edges.
                    createCubicPts(pts, cubicPts, numOfEdges);
                    find_edges(edges, cubicPts, numOfEdges, bit_map, false);
                }
                    break;

                default: break;
            }
        }

        // Sorting edges.
        std::sort(edges.begin(), edges.end(), sortEdges);

        // Return if there are no edges to apply.
        if (edges.empty()) return;

        // Blitter.
        Blitter blitter = Blitter(src, bit_map, ctm.top());
        int count = edges.size();

        // Shooting scan lines from the top to the bottom.
        for (int y = GRoundToInt(edges[0].min_y); count > 0; y++) {
            int i = 0;
            int w = 0;
            int start_x  = 0;

            // Looks for edges that intersect with the scan line.
            while (i < count && edges[i].min_y <= y) {
                // Renaming edge[i].
                Edge& currentEdge = edges[i];

                // Setting [start_x].
                if (w == 0) {
                    start_x = GRoundToInt(currentEdge.x);
                }

                // Incrementing w.
                w += currentEdge.w;

                // Setting [end_x] and blitting.
                if (w == 0) {
                    int end_x = GRoundToInt(currentEdge.x);
                    assert(start_x <= end_x);
                    blitter.blit(y, start_x, end_x);
                }

                // Erasing edge and reducing the count.
                if (y + 1 == currentEdge.max_y) {
                    edges.erase(edges.begin() + i);
                    count -= 1;
                    
                // Incrementing x.
                } else {
                    currentEdge.x += currentEdge.m;
                    i += 1;
                }
            }

            // Finding any future edges to add to be sorted by x.
            while (i < count && edges[i].min_y <= y + 1) {
                i += 1;
            }

            // Re-sort by x.
            std::sort(edges.begin(), edges.begin() + i, sortEdgesByX);
        }
    }

    /**
     *  Draw the quad, with optional color and/or texture coordinate at each corner. Tesselate
     *  the quad based on "level":
     *      level == 0 --> 1 quad  -->  2 triangles
     *      level == 1 --> 4 quads -->  8 triangles
     *      level == 2 --> 9 quads --> 18 triangles
     *      ...
     *  The 4 corners of the quad are specified in this order:
     *      top-left --> top-right --> bottom-right --> bottom-left
     *  Each quad is triangulated on the diagonal top-right --> bottom-left
     *      0---1
     *      |  /|
     *      | / |
     *      |/  |
     *      3---2
     *
     *  colors and/or texs can be null. The resulting triangles should be passed to drawMesh(...).
     */
    void drawQuad(const GPoint verts[4], const GColor colors[4], const GPoint texs[4], int level, const GPaint& paint) {
        // Nothing to draw.
        if (colors == nullptr && texs == nullptr) return;

        // Initializing num variables.
        int numOfVerts = pow(level + 2, 2);
        int numOfQuads = pow(level + 1, 2);
        int numOfTri   = numOfQuads * 2;
        int numOfPts   = numOfTri * 3;

        // Initializing arrays.
        GPoint newVerts[numOfVerts];
        GColor vertColors[numOfVerts];
        GPoint vertTexs[numOfVerts];
        int indices[numOfPts];

        // Iterating variables.
        int i, j, width, count;
        i = j = 0;
        width = level + 2;
        count = level + 1;
        float inverse_count = pow(count, -1);

        // Filling arrays.
        for (float v = 0; v <= count; v++) {
            float inverse_v = v * inverse_count;
            for (float u = 0; u <= count; u++) {
                float inverse_u = u * inverse_count;

                // Filling [indices] when not on the left/bottom border.
                if (u != 0 && v != count) {
                    // Triangle one.
                    indices[j++] = i;               // top-right.
                    indices[j++] = i + width - 1;   // bottom-left.
                    indices[j++] = i - 1;           // top-left.

                    // Triangle two.
                    indices[j++] = i;               // top-right.
                    indices[j++] = i + width;       // bottom-right.
                    indices[j++] = i + width - 1;   // bottom-left.
                }

                // Filling [vertColors].
                if (colors != nullptr) {
                    vertColors[i] = colorAt(colors[0], colors[1], colors[2], colors[3], inverse_u, inverse_v);
                }

                // Filling [vertTexs].
                if (texs != nullptr) {
                    vertTexs[i] = pointAt(texs[0], texs[1], texs[2], texs[3], inverse_u, inverse_v);
                }

                // Filling [newVert].
                newVerts[i] = pointAt(verts[0], verts[1], verts[2], verts[3], inverse_u, inverse_v);

                // Incrementing index.
                i++;
            }
        }

        // Delegating the rest to [drawMesh].
        drawMesh(newVerts, colors == nullptr ? nullptr : vertColors, texs == nullptr ? nullptr : vertTexs, numOfTri, indices, paint);
    }

    /**
     *  Draw a mesh of triangles, with optional colors and/or texture-coordinates at each vertex.
     *
     *  The triangles are specified by successive triples of indices.
     *      int n = 0;
     *      for (i = 0; i < count; ++i) {
     *          point0 = vertx[indices[n+0]]
     *          point1 = verts[indices[n+1]]
     *          point2 = verts[indices[n+2]]
     *          ...
     *          n += 3
     *      }
     *
     *  If colors is not null, then each vertex has an associated color, to be interpolated
     *  across the triangle. The colors are referenced in the same way as the verts.
     *          color0 = colors[indices[n+0]]
     *          color1 = colors[indices[n+1]]
     *          color2 = colors[indices[n+2]]
     *
     *  If texs is not null, then each vertex has an associated texture coordinate, to be used
     *  to specify a coordinate in the paint's shader's space. If there is no shader on the
     *  paint, then texs[] should be ignored. It is referenced in the same way as verts and colors.
     *          texs0 = texs[indices[n+0]]
     *          texs1 = texs[indices[n+1]]
     *          texs2 = texs[indices[n+2]]
     *
     *  If both colors and texs[] are specified, then at each pixel their values are multiplied
     *  together, component by component.
     */
    void drawMesh(const GPoint verts[], const GColor colors[], const GPoint texs[], int count, const int indices[], const GPaint& orig_paint) {
        // Nothing to draw.
        if (colors == nullptr && texs == nullptr) return;

        // Defining paint and shaders.
        GPaint paint = orig_paint;
        std::unique_ptr<GShader> tri_shader, proxy_shader, compose_shader;

        int n = 0;
        for (int i = 0; i < count; ++i) {
            // Defining pts.
            const GPoint pts[] = {
                verts[indices[n+0]],
                verts[indices[n+1]],
                verts[indices[n+2]]
            };

            // ComposeShader case.
            if (colors != nullptr && texs != nullptr) {
                const GColor triColors[] = {
                    colors[indices[n+0]],
                    colors[indices[n+1]],
                    colors[indices[n+2]]
                };

                const GPoint proxyCoords[] = {
                    texs[indices[n+0]],
                    texs[indices[n+1]],
                    texs[indices[n+2]]
                };

                // Setting the paint's shader.
                assert(paint.getShader() != nullptr);
                tri_shader     = GCreateTriColorShader(pts, triColors);
                proxy_shader   = GCreateProxyShader(orig_paint.getShader(), pts, proxyCoords);
                compose_shader = GCreateComposeShader(proxy_shader.get(), tri_shader.get());
                paint.setShader(compose_shader.get());
            }

            // TriColor case.
            else if (colors != nullptr) {
                GColor triColors[] = {
                    colors[indices[n+0]],
                    colors[indices[n+1]],
                    colors[indices[n+2]]
                };

                // Setting the paint's shader.
                tri_shader = GCreateTriColorShader(pts, triColors);
                paint.setShader(tri_shader.get());
            }

            // ProxyShader case.
            else {
                GPoint proxyCoords[] = {
                    texs[indices[n+0]],
                    texs[indices[n+1]],
                    texs[indices[n+2]]
                };

                // Setting the paint's shader.
                assert(paint.getShader() != nullptr);
                proxy_shader = GCreateProxyShader(orig_paint.getShader(), pts, proxyCoords);
                paint.setShader(proxy_shader.get());
            }

            // Delegate the drawing to drawConvexPolygon.
            drawConvexPolygon(pts, 3, paint);

            // Incrementing.
            n += 3;
        }
    }
    
    private:
        const GBitmap bit_map;
        std::stack <GMatrix> ctm;
};

// Returns a new canvas.
std::unique_ptr<GCanvas> GCreateCanvas(const GBitmap& device) {
    if (!device.pixels()) {
        return nullptr;
    }
    return std::unique_ptr<GCanvas>(new EmptyCanvas(device));
}

// Draws something.
std::string GDrawSomething(GCanvas* canvas, GISize dim) {
    // Creating points.
    GPoint backgroundPts[] = {
        GPoint::Make(0, 0), GPoint::Make(255, 0), GPoint::Make(255, 230), GPoint::Make(0, 230)
    };

    // Creating linear gradient for the sky.
    const GColor skyColors[] = {
        GColor::MakeARGB(1, 0.89, 0.75, 0.62), GColor::MakeARGB(1, 1, 0.4, 0.8), GColor::MakeARGB(1, 0.18, 0.10, 0.28), GColor::MakeARGB(1, 0, 0, 0)
    };
    auto skySh = GCreateLinearGradient(GPoint::Make(127, 230), GPoint::Make(127, 0), skyColors, 4);

    // Setting paint.
    GPaint paint = GPaint(GColor::MakeARGB(1, 1, 1, 1));
    paint.setShader(skySh.get());

    // Drawing background.
    canvas->drawConvexPolygon(backgroundPts, 4, paint);

    // Setting circle coords.
    int x  = 40;
    int y  = 100;
    int dx = 60;
    int dy = -35;

    // Setting paint.
    auto moonSh = GCreateLinearGradient(GPoint::Make(5, 80), GPoint::Make(240, 55), 
                    GColor::MakeARGB(1, 0.83, 0.83, 0.83), GColor::MakeARGB(1, 0, 0, 0));
    paint.setShader(moonSh.get());

    // Drawing moons.
    for (int i = 0; i < 4; i++) {
        GPath path;
        path.addCircle(GPoint::Make(x, y), 15);
        canvas->drawPath(path, paint);
        x += dx;
        y += dy;
        dy += 15;
    }

    // Creating mountain range.
    GPoint mountainPts[] = {
        GPoint::Make(0, 255), GPoint::Make(0, 200), GPoint::Make(75, 230), GPoint::Make(135, 180),
        GPoint::Make(155, 225), GPoint::Make(230, 170), GPoint::Make(255, 255)
    };

    // Setting paint.
    auto mountainSh = GCreateLinearGradient(GPoint::Make(127, 255), GPoint::Make(127, 200), 
                GColor::MakeARGB(1, 0, 0, 0), GColor::MakeARGB(1, 0.83, 0.83, 0.83));
    paint.setShader(mountainSh.get());

    // Drawing the mountain range.
    GPath path;
    path.addPolygon(mountainPts, 7);
    canvas->drawPath(path, paint);

    // Creating borders.
    GPoint border1[] = {
        GPoint::Make(0, 0), GPoint::Make(127, 0), GPoint::Make(0, 0), GPoint::Make(0, 127)
    };

    GPoint border2[] = {
        GPoint::Make(255, 0), GPoint::Make(127, 0), GPoint::Make(255, 0), GPoint::Make(255, 127)
    };

    GPoint border3[] = {
        GPoint::Make(255, 255), GPoint::Make(127, 255), GPoint::Make(255, 255), GPoint::Make(255, 127)
    };

    GPoint border4[] = {
        GPoint::Make(0, 255), GPoint::Make(0, 127), GPoint::Make(0, 255), GPoint::Make(127, 255)
    };

    GColor triColor[] = {
        GColor::MakeARGB(1, 0.52, 0.58, 0.65), GColor::MakeARGB(1, 0.73, 0.8, 0.87), GColor::MakeARGB(1, 0.92, 0.81, 0.79), GColor::MakeARGB(1, 0.64, 0.51, 0.52)
    };

    // Drawing borders.
    int numOfSegs = cubicSegments(border1);
    canvas->drawQuad(border1, triColor, nullptr, numOfSegs, paint);
    canvas->drawQuad(border2, triColor, nullptr, numOfSegs, paint);
    canvas->drawQuad(border3, triColor, nullptr, numOfSegs, paint);
    canvas->drawQuad(border4, triColor, nullptr, numOfSegs, paint);

    return "Night sky";
}
