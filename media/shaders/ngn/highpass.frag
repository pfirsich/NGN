in vec2 ngn_texCoord;
out vec3 fragColor;

uniform sampler2D image;
uniform float threshold;

void main() {
    vec3 input = texture(image, ngn_texCoord).rgb;
    fragColor = vec3(0.0, 0.0, 0.0);
    float brightness = dot(input.rgb, vec3(0.2126, 0.7152, 0.0722));
    if(brightness > 1.0) {
        fragColor = input;
    }
}