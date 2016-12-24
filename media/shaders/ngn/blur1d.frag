in vec2 ngn_texCoord;

out vec3 fragColor;

uniform sampler2D image;
uniform vec2 imageSize;
uniform bool horizontal;
uniform float radius = 1.0;

// http://rastergrid.com/blog/2010/09/efficient-gaussian-blur-with-linear-sampling/
const int TAP_COUNT = 3;
uniform float[TAP_COUNT] offset = float[](0.0, 1.3846153846, 3.2307692308);
uniform float[TAP_COUNT] weight = float[](0.2270270270, 0.3162162162, 0.0702702703);

void main() {
    vec2 dir = vec2(0.0, radius);
    float size = imageSize.y;
    if(horizontal) {
        dir = dir.yx;
        size = imageSize.y;
    }

    fragColor = texture(image, ngn_texCoord).rgb * weight[0];
    for(int i = 1; i < TAP_COUNT; ++i) {
        fragColor += texture(image, ngn_texCoord + (offset[i] * dir) / size);
        fragColor += texture(image, ngn_texCoord - (offset[i] * dir) / size);
    }
}