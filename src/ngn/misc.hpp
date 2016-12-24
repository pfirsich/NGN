#pragma once

#include <cmath>
#include <glm/glm.hpp>

namespace ngn {
    float srgbToLinear(float val);
    float linearToSRGB(float val);

    inline glm::vec3 srgbToLinear(const glm::vec3& v) {
        glm::vec3 ret(srgbToLinear(v.r), srgbToLinear(v.g), srgbToLinear(v.b));
        return ret;
    }

    inline glm::vec4 srgbToLinear(const glm::vec4& v) {
        glm::vec4 ret(srgbToLinear(glm::vec3(v)), 1.0f);
        return ret;
    }

    // for usual model/view matrices this is fine
    inline glm::vec3 transformPoint(const glm::mat4& transform, const glm::vec3& coord) {
        return glm::vec3(transform * glm::vec4(coord, 1.0f));
    }

    // If we are dealing with general, potentially non-affine transformations, use this (e.g. projections)
    // specifically: the last row of the transformation matrix has to be (0, 0, 0, 1), otherwise, use this
    inline glm::vec3 transformPointDivide(const glm::mat4& transform, const glm::vec3& coord) {
        glm::vec4 v = transform * glm::vec4(coord, 1.0f);
        if(v.w < 1e-5) v.w = 1.0f;
        return glm::vec3(v) / v.w;
    }

    // for vectors the last row can be (0, 0, 0, X)
    inline glm::vec3 transformVector(const glm::mat4& transform, const glm::vec3& coord, float w = 1.0f) {
        return glm::vec3(transform * glm::vec4(coord, 0.0f));
    }

    inline glm::vec3 transformVectorDivide(const glm::mat4& transform, const glm::vec3& coord) {
        glm::vec4 v = transform * glm::vec4(coord, 0.0f);
        if(v.w < 1e-5) v.w = 1.0f;
        return glm::vec3(v) / v.w;
    }

    // glm gives an error for glm::abs(mat4());
    inline glm::mat4 absMat4(const glm::mat4& mat) {
        glm::mat4 ret = mat;
        for(int i = 0; i < 3; ++i) {
            for(int j = 0; j < 3; ++j) {
                ret[i][j] = std::fabs(mat[i][j]);
            }
        }
        return ret;
    }
}
