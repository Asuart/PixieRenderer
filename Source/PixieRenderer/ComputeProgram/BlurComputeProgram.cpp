#include "BlurComputeProgram.h"

static const char* cBlurCS = R"(
#version 450

layout(local_size_x = 8, local_size_y = 8) in;

layout(set = 0, binding = 0, rgba32f) uniform readonly  image2D inputImage;
layout(set = 0, binding = 1, rgba32f) uniform writeonly image2D outputImage;

layout(push_constant) uniform PC { int radius; int _p0, _p1, _p2; } pc;

void main() {
    ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
    ivec2 size  = imageSize(outputImage);
    if (coord.x >= size.x || coord.y >= size.y) return;
    vec4 sum = vec4(0.0);
    int count = 0;
    int r = pc.radius;
    for (int y = -r; y <= r; y++)
    for (int x = -r; x <= r; x++) {
        ivec2 c = clamp(coord + ivec2(x, y), ivec2(0), size - 1);
        sum += imageLoad(inputImage, c);
        count++;
    }
    imageStore(outputImage, coord, sum / float(count));
}
)";

namespace PixieRenderer {

BlurComputeProgram::BlurComputeProgram() : IComputeProgram(cBlurCS) {
}

} // namespace PixieRenderer
