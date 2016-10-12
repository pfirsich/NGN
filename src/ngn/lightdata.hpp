#pragma once

#include <glm/glm.hpp>

#include "texture.hpp"
#include "rendertarget.hpp"
#include "aabb.hpp"

namespace ngn {
    class Camera;
    class SceneNode;

    class LightData {
    public:
        enum class LightType : int {
            POINT = 0,
            DIRECTIONAL,
            SPOT,
            // this always has to be the last element and is not an actual light type
            LIGHT_TYPES_LAST
        };

        class Shadow {
        friend class Renderer;
        public:
            constexpr static int MAX_CASCADES = 6;
        private:

            LightData* mParent;
            Rendertarget mShadowMap;
            Texture mShadowMapTexture;
            Camera* mCameras[MAX_CASCADES];
            int mShadowMapWidth, mShadowMapHeight;
            float mShadowBias;
            float mNormalShadowBias;
            bool mAutoCam;
            int mPCFSamples, mPCFEarlyBailSamples;
            float mPCFRadius;
            int mCascadeCount;
            float mCascadeLambda;

            float getCascadeSplit(float _near, float _far, int cascadeIndex) const;

        public:
            Shadow(LightData* parent, int shadowMapWidth, int shadowMapHeight, int cascades = 1, PixelFormat format = GL_DEPTH_COMPONENT24);
            ~Shadow();

            void setAutoCam(bool autocam) {mAutoCam = autocam;}
            bool getAutoCam() const {return mAutoCam;}

            void setBias(float bias) {mShadowBias = bias;}
            float getBias() const {return mShadowBias;}

            void setNormalBias(float bias) {mNormalShadowBias = bias;}
            float getNormalBias() const {return mNormalShadowBias;}

            void setPCFSamples(int samples, int earlyBailSamples = -1) {
                if(earlyBailSamples < 0) earlyBailSamples = samples > 8 ? 4 : 0;
                mPCFSamples = samples;
                mPCFEarlyBailSamples = earlyBailSamples;
            }
            int getPCFSamples() const {return mPCFSamples;}
            int getPCFEarlyBailSamples() const {return mPCFEarlyBailSamples;}

            // in shadow map texels
            void setPCFRadius(float radius) {mPCFRadius = radius;}
            float getPCFRadius() const {return mPCFRadius;}

            int getCascadeCount() const {return mCascadeCount;}

            float getCascadeLambda() const {return mCascadeLambda;}
            void setCascadeLambda(float lambda) {mCascadeLambda = lambda;}

            int getShadowMapWidth() const {return mShadowMapWidth;}
            int getShadowMapHeight() const {return mShadowMapHeight;}

            int getXCascadeCount() const {return (mCascadeCount + 1) / 2;}
            int getYCascadeCount() const {return mCascadeCount >= 2 ? 2 : 1;}

            void setShadowMapViewport(int cascadeIndex);
            void updateCamera(const Camera& viewCamera, const AABoundingBox& sceneBoundingBox, int cascadeIndex);
            Camera* getCamera(int cascadeIndex = 0) {return mCameras[cascadeIndex];}
        };

    private:
        SceneNode* mParent;
        LightType mType;
        float mRadius;
        float mAttenCutoff;
        //Should color and luminance be different parameters?
        glm::vec3 mColor;
        //Texture* mAttenuationTexture;
        Shadow* mShadow;

        // spotlight only
        float mInnerAngle, mOuterAngle;
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
        // cos(45°) = 0.7071067811865476 -- outer
        // cos(40°) = 0.766044443118978 -- inner
        LightData(SceneNode* parent, LightType type) : mParent(parent), mType(type), mRadius(1.0f),
                mAttenCutoff(0.004f), mColor(1.0f, 1.0f, 1.0f), mShadow(nullptr),
                mInnerAngle(glm::cos(glm::radians(40.0f))), mOuterAngle(glm::cos(glm::radians(45.0f))) {}

        ~LightData() {
            delete mShadow;
        }

        LightType getType() const {return mType;}

        float getRadius() const {return mRadius;}
        void setRadius(float radius) {mRadius = radius;}

        float getAttenCutoff() const {return mAttenCutoff;}
        void setAttenCutoff(float cutoff) {mAttenCutoff = cutoff;}

        float getRange() const {return getAttenInverse(mAttenCutoff / glm::max(glm::max(mColor.r, mColor.g), mColor.b));}
        void setRange(float range) {mAttenCutoff = glm::max(glm::max(mColor.r, mColor.g), mColor.b) * getAtten(range);}

        float getInnerAngle() const {return mInnerAngle;}
        void setInnerAngle(float cosangle) {mInnerAngle = cosangle;}
        void setInnerAngleDegrees(float degrees) {mInnerAngle = glm::cos(glm::radians(degrees));}

        float getOuterAngle() const {return mOuterAngle;}
        void setOuterAngle(float cosangle) {mOuterAngle = cosangle;}
        void setOuterAngleDegrees(float degrees) {mOuterAngle = glm::cos(glm::radians(degrees));}

        glm::vec3 getColor() const {return mColor;}
        void setColor(const glm::vec3& col) {mColor = col;}

        template<typename... Args>
        void addShadow(Args&& ...args) {if(!mShadow) mShadow = new Shadow(this, std::forward<Args>(args)...);}
        Shadow* getShadow() {return mShadow;}

        SceneNode* getParent() {return mParent;}

        /*Texture* getAttenuationTexture() {return mAttenuationTexture;}
        void setAttenuationTexture(Texture* tex) {mAttenuationTexture = tex;}

        Texture* getConeTexture() {return mConeTexture;}
        void setConeTexture(Texture* tex) {mConeTexture = tex;}*/
    };
}