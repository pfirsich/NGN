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
            // this always has to be the last element and is not an actual light type
            LIGHT_TYPES_LAST
        };

    private:
        LightType mType;
        float mRadius;
        float mAttenCutoff;
        //Shadow* mShadow;
        //Should color and luminance be different parameters?
        glm::vec3 mColor;
        //Texture* mAttenuationTexture;
        //Texture* mConeTexture;

        float getAtten(float distance) const {
            distance = distance / mRadius + 1.0f;
            return 1.0f / (distance*distance);
        }

        float getAttenInverse(float atten) const {
            return mRadius * (sqrt(1.0f / atten) - 1.0f);
        }

        float getAtten_cutoff(float distance) {
            float atten = getAtten(distance);
            return (atten - mAttenCutoff) / (1.0f - mAttenCutoff);
        }

    public:
        // 0.004 ~= 1/256
        LightData() : mType(LightType::DIRECTIONAL), mRadius(1.0f), mAttenCutoff(0.004f), mColor(1.0f, 1.0f, 1.0f) {}

        LightType getType() const {return mType;}
        void setType(LightType type) {mType = type;}

        float getRadius() const {return mRadius;}
        void setRadius(float radius) {mRadius = radius;}

        float getAttenCutoff() const {return mAttenCutoff;}
        void setAttenCutoff(float cutoff) {mAttenCutoff = cutoff;}
        float getRange() const {return getAttenInverse(mAttenCutoff / glm::max(glm::max(mColor.r, mColor.g), mColor.b));}
        void setRange(float range) {mAttenCutoff = glm::max(glm::max(mColor.r, mColor.g), mColor.b) * getAtten(range);}

        glm::vec3 getColor() const {return mColor;}
        void setColor(const glm::vec3& col) {mColor = col;}

        /*Texture* getAttenuationTexture() {return mAttenuationTexture;}
        void setAttenuationTexture(Texture* tex) {mAttenuationTexture = tex;}

        Texture* getConeTexture() {return mConeTexture;}
        void setConeTexture(Texture* tex) {mConeTexture = tex;}*/
    };
}