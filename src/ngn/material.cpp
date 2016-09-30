#include "material.hpp"
#include "renderer.hpp"

namespace ngn {
    void Material::validate() const {
        if((mBlendMode == BlendMode::MODULATE || mBlendMode == BlendMode::SCREEN) && (hasPass(Renderer::LIGHT_PASS)))
            LOG_WARNING("Blend mode MODULATE and SCREEN don't work properly with lit materials!");
    }

    void Material::setBlendMode(BlendMode mode) {
        mBlendMode = mode;
        switch(mode) {
            case BlendMode::REPLACE:
                mStateBlock.setBlendEnabled(false);
                mStateBlock.setDepthWrite(true);
                break;
            case BlendMode::TRANSLUCENT:
                mStateBlock.setBlendEnabled(true);
                mStateBlock.setBlendEquation(RenderStateBlock::BlendEq::ADD);
                mStateBlock.setBlendFactors(RenderStateBlock::BlendFactor::SRC_ALPHA, RenderStateBlock::BlendFactor::ONE_MINUS_SRC_ALPHA);
                mStateBlock.setDepthWrite(false);
                break;
            case BlendMode::ADD:
                mStateBlock.setBlendEnabled(true);
                mStateBlock.setBlendEquation(RenderStateBlock::BlendEq::ADD);
                mStateBlock.setBlendFactors(RenderStateBlock::BlendFactor::ONE, RenderStateBlock::BlendFactor::ONE);
                mStateBlock.setDepthWrite(false);
                break;
            case BlendMode::MODULATE:
                mStateBlock.setBlendEnabled(true);
                mStateBlock.setBlendEquation(RenderStateBlock::BlendEq::ADD);
                mStateBlock.setBlendFactors(RenderStateBlock::BlendFactor::DST_COLOR, RenderStateBlock::BlendFactor::ZERO);
                mStateBlock.setDepthWrite(false);
                break;
            case BlendMode::SCREEN:
                mStateBlock.setBlendEnabled(true);
                mStateBlock.setBlendEquation(RenderStateBlock::BlendEq::ADD);
                mStateBlock.setBlendFactors(RenderStateBlock::BlendFactor::ONE, RenderStateBlock::BlendFactor::ONE_MINUS_SRC_COLOR);
                mStateBlock.setDepthWrite(false);
                break;
        }
        validate();
    }
}