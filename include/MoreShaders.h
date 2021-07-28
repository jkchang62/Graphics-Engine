#include <GShader.h>

/**
 *  Return a subclass of GShader that proxies a real shader.
 */
std::unique_ptr<GShader> GCreateProxyShader(GShader* _realShader, const GPoint _pts[3], const GPoint _coords[3]);

/**
 *  Return a subclass of GShader that takes two shaders and blends their pixels.
 */
std::unique_ptr<GShader> GCreateComposeShader(GShader* _colorShader, GShader* _gradientShader);

/**
 *  Return a subclass of GShader that takes a triangle with color payload and draws them.
 */
std::unique_ptr<GShader> GCreateTriColorShader(const GPoint _pts[3], const GColor _colors[3]);