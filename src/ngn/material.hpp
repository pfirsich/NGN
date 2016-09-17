#pragma once

#include <vector>

#include "shader.hpp"
#include "renderstateblock.hpp"
#include "uniformblock.hpp"
#include "texture.hpp"

namespace ngn {
    class Material : public UniformList {
    public:
        ShaderProgram* mShader;
        RenderStateBlock mStateBlock;

        Material(ShaderProgram* shader) : mShader(shader) {}
    };
}