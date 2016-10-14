in vec2 ngn_texCoord;

out float fragColor;

uniform sampler2D logLuminance;
uniform sampler2D logLuminanceLast;

// smaller -> slower
uniform float tau = 1.0;
uniform float dt;

void main() {
    float currLuminance = exp(texture(logLuminance,     ngn_texCoord).r);
    float lastLuminance = exp(texture(logLuminanceLast, ngn_texCoord).r);
    // Pattanaik's technique
    // NOTE: if you want to smooth into a value exponentially, delta = (target - current) * constant is obviously
    // the way, but if you want it framerate-independent, then this is the extension of that method
    float adaptedLuminance = lastLuminance + (currLuminance - lastLuminance) * (1.0 - exp(-dt/tau));
    fragColor = log(adaptedLuminance);
}