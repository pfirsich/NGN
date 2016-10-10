#include <limits>

#include "lightdata.hpp"
#include "camera.hpp"

namespace ngn {
    LightData::Shadow::~Shadow() {
        delete mCamera;
    }

    void LightData::Shadow::updateCamera(const Camera& viewCamera) {
        if(mCamera && mAutoCam) {
            switch(mParent->getType()) {
                case LightData::LightType::DIRECTIONAL: {
                    //TODO: In z this should clamp the scene, not the camer frustum!
                    OrthographicCamera* shadowCam = static_cast<OrthographicCamera*>(mCamera);
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

                    //TODO: explain this
                    for(int i = 0; i < 8; ++i) {
                        frustumCorners[i] = glm::vec4(glm::vec3(frustumCorners[i]) / frustumCorners[i].w, 1.0);
                    }

                    // debug, cube in center of world
                    /*const float hsize = 50.0f;
                    frustumCorners[0] = glm::vec4(-hsize, hsize,  hsize, 1.0f);
                    frustumCorners[1] = glm::vec4(-hsize,  0.0f,  hsize, 1.0f);
                    frustumCorners[2] = glm::vec4( hsize,  0.0f,  hsize, 1.0f);
                    frustumCorners[3] = glm::vec4( hsize, hsize,  hsize, 1.0f);
                    frustumCorners[4] = glm::vec4(-hsize, hsize, -hsize, 1.0f);
                    frustumCorners[5] = glm::vec4(-hsize,  0.0f, -hsize, 1.0f);
                    frustumCorners[6] = glm::vec4( hsize,  0.0f, -hsize, 1.0f);
                    frustumCorners[7] = glm::vec4( hsize, hsize, -hsize, 1.0f);*/

                    // transform to light space and find min/max
                    glm::mat4 toLightSpace = shadowCam->getViewMatrix();
                    float inf = std::numeric_limits<float>::infinity();
                    glm::vec4 min(inf, inf, inf, 0.0f), max(-inf, -inf, -inf, 1.0f);
                    for(int i = 0; i < 8; ++i) {
                        frustumCorners[i] = toLightSpace * frustumCorners[i];
                        min = glm::min(min, frustumCorners[i]);
                        max = glm::max(max, frustumCorners[i]);
                    }

                    min.z -= 100.0f;
                    max.z += 100.0f;
                    glm::vec4 worldPos = (min + max) * 0.5f;
                    worldPos.z = max.z;
                    worldPos = glm::inverse(toLightSpace) * worldPos;

                    shadowCam->setPosition(glm::vec3(glm::inverse(shadowCam->getParent()->getWorldMatrix()) * worldPos));
                    shadowCam->set(min.x, max.x, min.y, max.y, 0.0f, max.z - min.z);
                    break;
                }
                case LightData::LightType::SPOT: {
                    PerspectiveCamera* shadowCam = static_cast<PerspectiveCamera*>(mCamera);
                    shadowCam->set(glm::acos(mParent->getOuterAngle())*2.0f, 1.0f, 0.1f, mParent->getRange());
                    break;
                }
                default:
                    LOG_ERROR("Automatic camera not supported for light type: %d", static_cast<int>(mParent->getType()));
            }
        }
    }

    LightData::Shadow::Shadow(LightData* parent, int shadowMapWidth, int shadowMapHeight, PixelFormat format) :
            mParent(parent), mCamera(nullptr), mShadowBias(0.0005f), mAutoCam(true) {
        mShadowMapTexture.setStorage(format, shadowMapWidth, shadowMapHeight);
        LOG_DEBUG("shadow map texture object: %d", mShadowMapTexture.getTextureObject());
        mShadowMapTexture.setCompareFunc();
        mShadowMap.attachTexture(Rendertarget::Attachment::DEPTH, &mShadowMapTexture);
        switch(mParent->getType()) {
            case LightData::LightType::POINT:
                LOG_ERROR("Point light shadows are not supported yet!");
                break;
            case LightData::LightType::DIRECTIONAL:
                mCamera = new OrthographicCamera;
                break;
            case LightData::LightType::SPOT:
                mCamera = new PerspectiveCamera;
                break;
            default:
                LOG_CRITICAL("Unknown light type!");
                return;
        }
        mParent->getParent()->add(*mCamera); // add camera as child of light scene node
    }
}