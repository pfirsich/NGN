in vec2 ngn_texCoord;

out vec4 fragColor;

uniform sampler2D hdrImage;
uniform sampler2D logLuminance;

uniform float whitePoint = 100.0;
uniform float logLumLod = 10.0;
uniform float keyValue = 2.0;

vec3 Uncharted2Tonemap(in vec3 x) {
    float A = 0.15;
    float B = 0.50;
    float C = 0.10;
    float D = 0.20;
    float E = 0.02;
    float F = 0.30;
    return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

vec3 reinhard(in vec3 x) {
    return x / (vec3(1.0) + x);
}

void main() {
    float avgLuminance = exp(textureLod(logLuminance, ngn_texCoord, logLumLod).r);
    avgLuminance = min(100.0, max(0.001, avgLuminance));
    //fragColor = vec4(avgLuminance / 4.0); return;

    //float keyValueAuto = 1.03 - (2.0 / (2.0 + log(avgLuminance + 1) / log(10.0)));

    float exposure = keyValue / avgLuminance;
    //fragColor = vec4(exposure / 60.0); return;

    vec3 texColor = texture(hdrImage, ngn_texCoord).rgb;
    //fragColor = vec4(texColor / 4.0, 1.0); return;

    vec3 exposedColor = exposure * texColor;
    //fragColor = vec4(exposedColor / 10.0, 1.0); return;

    // reinhard(exposedColor) / reinhard(vec3(whitePoint))
    fragColor = vec4(reinhard(exposedColor), 1.0);
}