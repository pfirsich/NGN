#include "misc.hpp"

namespace ngn {
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
            return 1.055f * std::pow(val, 0.41666f) - 0.055f;
        } else { // if(val >= 1.0f)
            return 1.0f;
        }
    }
}