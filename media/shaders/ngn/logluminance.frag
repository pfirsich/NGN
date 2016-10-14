in vec2 ngn_texCoord;

out vec4 fragColor;

uniform sampler2D hdrImage;

void main() {
    const float eps = 1e-6;
    float lum = dot(texture(hdrImage, ngn_texCoord).xyz, vec3(0.2125, 0.7154, 0.0721));
    float logLum = log(lum + eps);
    fragColor = vec4(logLum, 0.0, 0.0, 0.0);
}

