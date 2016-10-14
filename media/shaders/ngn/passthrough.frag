in vec2 ngn_texCoord;

out vec4 fragColor;

uniform sampler2D input;

void main() {
    fragColor = texture(input, ngn_texCoord);
}