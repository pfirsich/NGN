#include <limits>

#include "lightdata.hpp"
#include "camera.hpp"

namespace ngn {
    LightData::Shadow::~Shadow() {
        delete mCamera;
    }

    void LightData::Shadow::updateCamera(const Camera& viewCamera, const AABoundingBox& sceneBoundingBox) {
        if(mCamera && mAutoCam) {
            switch(mParent->getType()) {
                case LightData::LightType::DIRECTIONAL: {
                    float cascadeStart = 0.0f;
                    float cascadeEnd = 1.0f;

                    //TODO: In z this should clamp the scene, not the camer frustum!
                    OrthographicCamera* shadowCam = static_cast<OrthographicCamera*>(mCamera);
                    glm::mat4 inverseProject = glm::inverse(viewCamera.getProjectionMatrix() * viewCamera.getViewMatrix());
                    glm::vec4 frustumCorners[8]; // world space
                    cascadeStart = cascadeStart * 2.0f - 1.0f;
                    cascadeEnd = cascadeEnd * 2.0f - 1.0f;
                    // near
                    frustumCorners[0] = inverseProject * glm::vec4(-1.0f, -1.0f, cascadeStart, 1.0f);
                    frustumCorners[1] = inverseProject * glm::vec4(-1.0f,  1.0f, cascadeStart, 1.0f);
                    frustumCorners[2] = inverseProject * glm::vec4( 1.0f,  1.0f, cascadeStart, 1.0f);
                    frustumCorners[3] = inverseProject * glm::vec4( 1.0f, -1.0f, cascadeStart, 1.0f);
                    // far
                    frustumCorners[4] = inverseProject * glm::vec4(-1.0f, -1.0f, cascadeEnd,   1.0f);
                    frustumCorners[5] = inverseProject * glm::vec4(-1.0f,  1.0f, cascadeEnd,   1.0f);
                    frustumCorners[6] = inverseProject * glm::vec4( 1.0f,  1.0f, cascadeEnd,   1.0f);
                    frustumCorners[7] = inverseProject * glm::vec4( 1.0f, -1.0f, cascadeEnd,   1.0f);

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

                    // transform to light space and find min/max
                    glm::mat4 toLightSpace = shadowCam->getViewMatrix();
                    float inf = std::numeric_limits<float>::infinity();
                    glm::vec4 min(inf, inf, inf, 0.0f), max(-inf, -inf, -inf, 1.0f);
                    for(int i = 0; i < 8; ++i) {
                        frustumCorners[i] = toLightSpace * frustumCorners[i];
                        min = glm::min(min, frustumCorners[i]);
                        max = glm::max(max, frustumCorners[i]);
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
            mParent(parent), mCamera(nullptr), mShadowBias(0.0002f), mNormalShadowBias(1.0f), mAutoCam(true),
            mPCFSamples(16), mPCFEarlyBailSamples(4), mPCFRadius(2.5f) {
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