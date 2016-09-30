#pragma once

#include <glm/glm.hpp>

#include "texture.hpp"

namespace ngn {
    class LightData {
    public:
        enum class LightType : int {
            POINT = 0,
            DIRECTIONAL,
            SPOT,
            //AREA,
            // this always has to be the last element and is not an actual light type
            LIGHT_TYPES_LAST
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

        float getRange() const {return mRange;}
        void setRange(float range) {mRange = range;}

        glm::vec3 getColor() const {return mColor;}
        void setColor(const glm::vec3& col) {mColor = col;}

        Texture* getAttenuationTexture() {return mAttenuationTexture;}
        void setAttenuationTexture(Texture* tex) {mAttenuationTexture = tex;}

        Texture* getConeTexture() {return mConeTexture;}
        void setConeTexture(Texture* tex) {mConeTexture = tex;}
    };
}