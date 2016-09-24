#pragma once

#include <glm/glm.hpp>

#include "texture.hpp"

namespace ngn {
    class LightData {
    public:
        enum class LightType {
            POINT,
            DIRECTIONAL,
            SPOT,
            AREA
        };

    private:
        LightType mType;
        float mRange;
        //Shadow* mShadow;
        glm::vec3 mColor;
        Texture* mAttenuationTexture;
        Texture* mConeTexture;

    public:
        LightData() : mType(LightType::DIRECTIONAL), mRange(1.0f), mColor(1.0f, 1.0f, 1.0f),
            mAttenuationTexture(nullptr), mConeTexture(nullptr) {}

        LightType getType() const {return mType;}
        void setType(LightType type) {mType = type;}
    };
}