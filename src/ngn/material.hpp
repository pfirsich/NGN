#pragma once

#include <vector>

#include "shaderprogram.hpp"
#include "renderstateblock.hpp"
#include "uniformblock.hpp"
#include "texture.hpp"

namespace ngn {
    class Material : public UniformList {
    friend class Renderer;

    public:
        enum class BlendMode {
            REPLACE, // disable GL_BLEND
            TRANSLUCENT, // SRC_ALPHA, ONE_MINUS_SRC_ALPHA and ADD
            ADD, // ONE, ONE and ADD
            MODULATE, // DST_COLOR, ZERO and ADD
            SCREEN, // ONE, ONE_MINUS_SRC_COLOR and ADD
        };

        /*
        wanted: SRC_FACTOR * (ambient + diffspec_1 + diffspec_2 + ... + diffspec_N) + DST_FACTOR * dst
        first pass: SRC_FACTOR * ambient + DST_FACTOR * dst
        ADD:
            naive:
                1st pass: ONE * ambient + ONE * dst
                2nd pass: ONE * src + ONE * diffspec_1 = ONE * ambient + ONE * dst + ONE * diffspec_i
                => works!
        TRANSLUCENT:
            naive:
                1st pass: SRC_ALPHA * ambient + ONE_MINUS_SRC_ALPHA * dst
                2nd pass: SRC_ALPHA * diffspec_1 + ONE_MINUS_SRC_ALPHA * dst = SRC_ALPHA * diffspec_1 + ONE_MINUS_SRC_ALPHA * (SRC_ALPHA * ambient + ONE_MINUS_SRC_ALPHA * dst)
            right: src_factor: src_factor, dst_factor: one
        MODULATE:
            naive:
                1st pass: ambient * dst
                2nd pass: diffspec_1 * dst = diffspec_1 * ambient * dst
            It's a problem that after every pass the frambuffer has to be in a valid state, because then we can not decide to
            encode something cleverly, but have to accept the fact, that we can not retrieve the original dst in the framebuffer on further passes
            => impossible without extra work
        SCREEN:
            naive:
                1st pass: ambient + (1-ambient) * dst
                2nd pass: diffspec_1 + (1-ambient) * ambient + (1-ambient)^2 * dst
            also not possible, since we want to multiply with the final output color, but it has not been determined ye
            => impossible without extra work
         */
    private:
        bool mLit;
        BlendMode mBlendMode;
        RenderStateBlock mStateBlock;

        void validate() const;

    public:
        ShaderProgram* mShader;

        Material(ShaderProgram* shader) : mLit(true), mBlendMode(BlendMode::REPLACE), mShader(shader) {}
        Material(const Material& base) : UniformList(base), mLit(base.mLit), mBlendMode(base.mBlendMode), mStateBlock(base.mStateBlock), mShader(base.mShader) {}

        void setLit(bool lit = true) {mLit = lit;}
        void setUnlit(bool unlit = true) {mLit = !unlit;}
        bool getLit() const {return mLit;}

        void setBlendMode(BlendMode mode);
        BlendMode getBlendMode() const {return mBlendMode;}

        bool getDepthWrite() const {return mStateBlock.getDepthWrite();}
        void setDepthWrite(bool write) {mStateBlock.setDepthWrite(write);}

        DepthFunc getDepthTest() const {return mStateBlock.getDepthTest();}
        void setDepthTest(DepthFunc func = DepthFunc::LEQUAL) {mStateBlock.setDepthTest(func);}

        FaceDirections getCullFaces() const {return mStateBlock.getCullFaces();}
        void setCullFaces(FaceDirections dirs = FaceDirections::BACK) {mStateBlock.setCullFaces(dirs);}

        FaceOrientation getFrontFace() const {return mStateBlock.getFrontFace();}
        void setFrontFace(FaceOrientation ori) {mStateBlock.setFrontFace(ori);}
    };
}