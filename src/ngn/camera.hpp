#pragma once

#include "scenenode.hpp"

namespace ngn {
    //TODO: Camera has to account for parent transforms
    class Camera : public SceneNode {
    protected:
        glm::mat4 mProjectionMatrix;
        Mesh* mDebugMesh;
        float mNear, mFar;

    public:
        Camera() : mDebugMesh(nullptr) {}
        virtual ~Camera() {
            delete mDebugMesh;
        }

        virtual glm::mat4 getViewMatrix() const {
            return glm::inverse(getWorldMatrix());
        }

        virtual glm::mat4 getInverseViewMatrix() const {
            return getWorldMatrix();
        }

        // TODO:
        virtual void setViewMatrix(const glm::mat4& matrix, bool updateTRS = true) {
            //transforms.setMatrix(glm::inverse(matrix), updateTRS);
        }

        virtual glm::mat4 getProjectionMatrix() const {
            return mProjectionMatrix;
        }

        virtual glm::mat4 getInverseProjectionMatrix() const {
            return glm::inverse(getProjectionMatrix());
        }

        virtual void updateProjectionMatrix() = 0;

        void addDebugMesh() {
            VertexFormat vFormat;
            vFormat.add(AttributeType::POSITION, 3, AttributeDataType::F32);

            mDebugMesh = new Mesh(Mesh::DrawMode::LINES);
            mDebugMesh->addVertexBuffer(vFormat, 24, UsageHint::DYNAMIC);
            updateDebugMesh();
            setMesh(mDebugMesh);
            setMaterial(Material::fallback);
        }

        void updateDebugMesh() {
            if(mDebugMesh) {
                auto position = mDebugMesh->getAccessor<glm::vec3>(AttributeType::POSITION);
                glm::mat4 inverseProject = getInverseProjectionMatrix();
                glm::vec3 frustumCorners[8]; // view space
                // near
                frustumCorners[0] = transformPointDivide(inverseProject, glm::vec3(-1.0f, -1.0f, -1.0f));
                frustumCorners[1] = transformPointDivide(inverseProject, glm::vec3(-1.0f,  1.0f, -1.0f));
                frustumCorners[2] = transformPointDivide(inverseProject, glm::vec3( 1.0f,  1.0f, -1.0f));
                frustumCorners[3] = transformPointDivide(inverseProject, glm::vec3( 1.0f, -1.0f, -1.0f));
                // far
                frustumCorners[4] = transformPointDivide(inverseProject, glm::vec3(-1.0f, -1.0f,  1.0f));
                frustumCorners[5] = transformPointDivide(inverseProject, glm::vec3(-1.0f,  1.0f,  1.0f));
                frustumCorners[6] = transformPointDivide(inverseProject, glm::vec3( 1.0f,  1.0f,  1.0f));
                frustumCorners[7] = transformPointDivide(inverseProject, glm::vec3( 1.0f, -1.0f,  1.0f));

                // near and far rectangle
                int index = 0;
                for(int i = 0; i < 4; ++i) {
                    int next = i<3 ? i+1 : i-3;
                    // near
                    position[index++] = frustumCorners[i];
                    position[index++] = frustumCorners[next];
                    // far
                    position[index++] = frustumCorners[i+4];
                    position[index++] = frustumCorners[next+4];
                }

                // frustum edges between near and far
                for(int i = 0; i < 4; ++i) {
                    position[index++] = frustumCorners[i];
                    position[index++] = frustumCorners[i+4];
                }

                mDebugMesh->hasAttribute(AttributeType::POSITION)->upload();
                mDebugMesh->updateBoundingBox();
            }
        }

        glm::vec4 project(const glm::vec4& v) const {
            return getProjectionMatrix() * getViewMatrix() * v;
        }

        glm::vec4 unproject(const glm::vec4& v) const {
            return getInverseViewMatrix() * getInverseProjectionMatrix() * v;
        }

        float getNear() const {return mNear;}
        void setNear(float _near) {mNear = _near; updateProjectionMatrix();}

        float getFar() const {return mFar;}
        void setFar(float _far) {mFar = _far; updateProjectionMatrix();}

        void setNearFar(float _near, float _far) {
            mNear = _near, mFar = _far;
            updateProjectionMatrix();
        }
    };

    class PerspectiveCamera : public Camera {
    private:
        float mFovY, mAspect;
    public:
        PerspectiveCamera() : mFovY(glm::radians(45.0f)), mAspect(1.0f) {
            mNear = 0.1f; mFar = 100.0f;
            updateProjectionMatrix();
        }
        PerspectiveCamera(float fovy, float aspect, float _near, float _far) {
            set(fovy, aspect, _near, _far);
        }
        ~PerspectiveCamera() {}

        float getFOV() const {return mFovY;}
        void setFOV(float fovy) {mFovY = fovy; updateProjectionMatrix();}

        float getAspect() const {return mAspect;}
        void setAspect(float aspect) {mAspect = aspect; updateProjectionMatrix();}

        void set(float fovy, float aspect) {
            set(fovy, aspect, mNear, mFar);
        }

        void set(float fovy, float aspect, float _near, float _far) {
            mFovY = fovy, mAspect = aspect, mNear = _near, mFar = _far;
            updateProjectionMatrix();
        }

        void updateProjectionMatrix() {
            mProjectionMatrix = glm::perspective(mFovY, mAspect, mNear, mFar);
            updateDebugMesh();
        }
    };

    class OrthographicCamera : public Camera {
    private:
        float mLeft, mRight, mTop, mBottom;
    public:

        OrthographicCamera() : mLeft(-1.0f), mRight(1.0f), mTop(-1.0f), mBottom(1.0f) {
            mNear = 1.0f; mFar = 1.0f;
            updateProjectionMatrix();
        }
        OrthographicCamera(float left, float right, float top, float bottom, float _near = -1.0f, float _far = 1.0f) {
            set(left, right, top, bottom, _near, _far);
        }
        ~OrthographicCamera() {}

        float getLeft() const {return mLeft;}
        void setLeft(float left) {mLeft = left; updateProjectionMatrix();}

        float getRight() const {return mRight;}
        void setRight(float right) {mRight = right; updateProjectionMatrix();}

        float getTop() const {return mTop;}
        void setTop(float top) {mTop = top; updateProjectionMatrix();}

        float getBottom() const {return mBottom;}
        void setBottom(float bottom) {mBottom = bottom; updateProjectionMatrix();}

        void set(float left, float right, float top, float bottom) {
            set(left, right, top, bottom, mNear, mFar);
        }

        void set(float left, float right, float top, float bottom, float _near, float _far) {
            mLeft = left; mRight = right; mTop = top; mBottom = bottom; mNear = _near, mFar = _far;
            updateProjectionMatrix();
        }

        void updateProjectionMatrix() {
            mProjectionMatrix = glm::ortho(mLeft, mRight, mTop, mBottom, mNear, mFar);
            updateDebugMesh();
        }
    };
}
