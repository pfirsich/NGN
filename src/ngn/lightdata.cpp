#include <limits>

#include "lightdata.hpp"
#include "camera.hpp"

namespace ngn {
    LightData::Shadow::~Shadow() {
        for(int i = 0; i < mCascadeCount; ++i) delete mCameras[i];
    }

    float LightData::Shadow::getCascadeSplit(float _near, float _far, int index) const {
        float ratio = static_cast<float>(index) / mCascadeCount;
        float z_log = _near * std::pow(_far / _near, ratio);
        float z_lin = _near + (_far - _near) * ratio;
        return mCascadeLambda * z_log + (1.0f - mCascadeLambda) * z_lin;
    }

    void LightData::Shadow::updateCamera(const Camera& viewCamera, const AABoundingBox& sceneBoundingBox, int cascadeIndex) {
        if(mCameras[cascadeIndex] && mAutoCam) {
            switch(mParent->getType()) {
                case LightData::LightType::DIRECTIONAL: {
                    OrthographicCamera* shadowCam = static_cast<OrthographicCamera*>(mCameras[cascadeIndex]);
                    glm::mat4 inverseProject = glm::inverse(viewCamera.getProjectionMatrix() * viewCamera.getViewMatrix());
                    glm::vec4 frustumCorners[8]; // world space
                    // near
                    frustumCorners[0] = inverseProject * glm::vec4(-1.0f, -1.0f, -1.0f, 1.0f);
                    frustumCorners[1] = inverseProject * glm::vec4(-1.0f,  1.0f, -1.0f, 1.0f);
                    frustumCorners[2] = inverseProject * glm::vec4( 1.0f,  1.0f, -1.0f, 1.0f);
                    frustumCorners[3] = inverseProject * glm::vec4( 1.0f, -1.0f, -1.0f, 1.0f);
                    // far
                    frustumCorners[4] = inverseProject * glm::vec4(-1.0f, -1.0f,  1.0f, 1.0f);
                    frustumCorners[5] = inverseProject * glm::vec4(-1.0f,  1.0f,  1.0f, 1.0f);
                    frustumCorners[6] = inverseProject * glm::vec4( 1.0f,  1.0f,  1.0f, 1.0f);
                    frustumCorners[7] = inverseProject * glm::vec4( 1.0f, -1.0f,  1.0f, 1.0f);

                    /*This is not so easy.
                    All our 4-dimensional vectors represent points in a projective space (not actual 3d space ("affine") points)
                    The points that are real points in the 3-dimensional space are only those that have w = 1
                    (i.e. they correspond to the affine subspace of our projective space)
                    If we apply homogeneous mappings on our projective points we get other projective points out, potentially even points
                    that are *not* part of the affine subspace! In general this happens all the time, and also with the inverse of our
                    projection matrix. Since all multiples of a projective point in homogeneous coordinates correspond to the same point
                    ("are in the same equivalence class") ([x:y:z:w] = [kx:ky:kz:kz] for k in the field underlying our space, in this case real numbers)
                    we can just divide by w to get the coordinates of our affine point!

                    There is probably a way to understand this without knowing about projective geometry, but I think it's way harder and this was
                    hard enough to remember already.
                    */
                    for(int i = 0; i < 8; ++i) {
                        frustumCorners[i] = glm::vec4(glm::vec3(frustumCorners[i]) / frustumCorners[i].w, 1.0);
                    }

                    float viewNear = viewCamera.getNear();
                    float viewFar = viewCamera.getFar();
                    float cascadeStart = (getCascadeSplit(viewNear, viewFar, cascadeIndex) - viewNear) / (viewFar - viewNear);
                    float cascadeEnd = (getCascadeSplit(viewNear, viewFar, cascadeIndex + 1) - viewNear) / (viewFar - viewNear);
                    // transform to light space and find min/max
                    glm::mat4 toLightSpace = shadowCam->getViewMatrix();
                    float inf = std::numeric_limits<float>::infinity();
                    glm::vec4 min(inf, inf, inf, 0.0f), max(-inf, -inf, -inf, 1.0f);
                    for(int nearFactor = 0; nearFactor <= 1; ++nearFactor) {
                        for(int i = 0; i < 4; ++i) {
                            glm::vec4 point = toLightSpace * glm::mix(frustumCorners[i], frustumCorners[i+4], nearFactor ? cascadeStart : cascadeEnd);
                            min = glm::min(min, point);
                            max = glm::max(max, point);
                        }
                    }

                    // use scene bounds to determine min/max z
                    // do this by looping through all the corner points, transforming them to light pos and min/max ing z
                    min.z = inf; max.z = -inf;
                    glm::vec3 point, dir;
                    for(int i = 0; i < 8; ++i) {
                        dir = glm::vec3(0.0f, 0.0f, 0.0f);
                        if(i & 1) dir += glm::vec3(1.0f, 0.0f, 0.0f);
                        if(i & 2) dir += glm::vec3(0.0f, 1.0f, 0.0f);
                        if(i & 4) dir += glm::vec3(0.0f, 0.0f, 1.0f);
                        point = sceneBoundingBox.min + dir * sceneBoundingBox.max;
                        //LOG_DEBUG("max: %f, %f, %f - point: %f, %f, %f", sceneBoundingBox.max.x, sceneBoundingBox.max.y, sceneBoundingBox.max.z, point.x, point.y, point.z);
                        point = transformPoint(toLightSpace, point);
                        min.z = std::min(min.z, point.z);
                        max.z = std::max(max.z, point.z);
                    }

                    /*glm::vec4 worldPos = (min + max) * 0.5f;
                    worldPos.z = max.z;
                    worldPos = glm::inverse(toLightSpace) * worldPos;
                    shadowCam->setPosition(glm::vec3(glm::inverse(shadowCam->getParent()->getWorldMatrix()) * worldPos));*/

                    // regarding near/far: http://stackoverflow.com/questions/14836606/opengl-orthogonal-view-near-far-values
                    //shadowCam->set(min.x, max.x, min.y, max.y, 0.0f, max.z - min.z);
                    shadowCam->set(min.x, max.x, min.y, max.y, min.z, max.z - min.z);
                    break;
                }
                case LightData::LightType::SPOT: { // ez
                    PerspectiveCamera* shadowCam = static_cast<PerspectiveCamera*>(mCameras[cascadeIndex]);
                    shadowCam->set(glm::acos(mParent->getOuterAngle())*2.0f, 1.0f, 0.1f, mParent->getRange());
                    break;
                }
                default:
                    LOG_ERROR("Automatic camera not supported for light type: %d", static_cast<int>(mParent->getType()));
            }
        }
    }

    void LightData::Shadow::setShadowMapViewport(int cascadeIndex) {
        int x = cascadeIndex / 2;
        int y = cascadeIndex % 2;
        glViewport(x*mShadowMapWidth, y*mShadowMapHeight, mShadowMapWidth, mShadowMapHeight);
        glScissor( x*mShadowMapWidth, y*mShadowMapHeight, mShadowMapWidth, mShadowMapHeight);
    }

    LightData::Shadow::Shadow(LightData* parent, int shadowMapWidth, int shadowMapHeight, int cascades, PixelFormat format) :
            mParent(parent), mShadowMapWidth(shadowMapWidth), mShadowMapHeight(shadowMapHeight), mShadowBias(0.0002f),
            mNormalShadowBias(1.0f), mAutoCam(true), mPCFSamples(16), mPCFEarlyBailSamples(4), mPCFRadius(2.5f), mCascadeCount(cascades), mCascadeLambda(0.8f) {
        if(cascades < 1) cascades = 1;
        if(cascades > MAX_CASCADES) {
            LOG_ERROR("Maximum number of cascades is %d! Clamping to %d cascades.", MAX_CASCADES, MAX_CASCADES);
            cascades = MAX_CASCADES;
        }
        if(cascades > 1 && mParent->getType() != LightData::LightType::DIRECTIONAL) {
            LOG_ERROR("Shadow map cascades not supported for non-directional lights. Clamping to 1 cascade.");
            cascades = 1;
        }

        mShadowMapTexture.setStorage(format, shadowMapWidth * getXCascadeCount(), shadowMapHeight * getYCascadeCount());
        mShadowMapTexture.setCompareFunc();
        mShadowMapTexture.setBorderColor(glm::vec4(1.0f));
        mShadowMap.attachTexture(Rendertarget::Attachment::DEPTH, mShadowMapTexture);

        for(int i = 0; i < mCascadeCount; ++i) mCameras[i] = nullptr;
        switch(mParent->getType()) {
            case LightData::LightType::POINT:
                LOG_ERROR("Point light shadows are not supported yet!");
                break;
            case LightData::LightType::DIRECTIONAL:
                for(int i = 0; i < mCascadeCount; ++i) mCameras[i] = new OrthographicCamera;
                break;
            case LightData::LightType::SPOT:
                mCameras[0] = new PerspectiveCamera;
                break;
            default:
                LOG_CRITICAL("Unknown light type!");
                return;
        }
        for(int i = 0; i < mCascadeCount; ++i) {
            mParent->getParent()->add(*mCameras[i]); // add camera as child of light scene node
        }
    }
}