#pragma once

#include "uniformblock.hpp"

namespace ngn {
    struct RendererData {
        UniformList uniforms;
        glm::mat4 worldMatrix;
        AABoundingBox boundingBox;
        virtual ~RendererData() {}
    };
}