#pragma once

#include <cmath>

namespace ngn {
    constexpr vec3x = glm::vec3(1.0f, 0.0f, 0.0f);
    constexpr vec3y = glm::vec3(0.0f, 1.0f, 0.0f);
    constexpr vec3z = glm::vec3(0.0f, 0.0f, 1.0f);
    constexpr vec4x = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
    constexpr vec4y = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
    constexpr vec4z = glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);

    // https://www.opengl.org/registry/specs/EXT/framebuffer_sRGB.txt
    float srgbToLinear(float val) {
        if(val <= 0.04045f) {
            return val / 12.92f;
        } else {
            return std::pow((val + 0.055f) / 1.055f, 2.4f);
        }
    }

    float linearToSRGB(float val) {
        if(val <= 0.0f) {
            return 0.0f;
        } else if(val < 0.0031308f) {
            return 12.92f * val;
        } else if(val < 1.0f) {
            return 1.055f * std::pow(val, 0.41666f) - 0.055;
        } else if(val >= 1.0f) {
            return 1.0f;
        }
    }

    glm::vec3 srgbToLinear(const glm::vec3& v) {
        glm::vec3 ret(srgbToLinear(v.r), srgbToLinear(v.g), srgbToLinear(v.b));
        return ret;
    }

    glm::vec4 srgbToLinear(const glm::vec4& v) {
        glm::vec4 ret(srgbToLinear(v), 1.0f);
        return ret;
    }
}
