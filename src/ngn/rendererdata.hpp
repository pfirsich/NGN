#pragma once

#include "uniformblock.hpp"

namespace ngn {
    struct RendererData {
        UniformList uniforms;
        virtual ~RendererData() {}
    };
}