#pragma once

#include "scenenode.hpp"

namespace ngn {
    //TODO: Camera has to account for parent transforms
    class Camera : public SceneNode {
    protected:
        glm::mat4 mViewMatrix, mInverseViewMatrix, mProjectionMatrix, mInverseProjectionMatrix;
        bool mViewDirty, mProjectionDirty;

        void dirty() {mViewDirty = true;}

    public:
        Camera() : mViewDirty(true), mProjectionDirty(true) {}
        ~Camera() {}

        // Here I will reuse Object::mModelDirty and hope that no one calls getModelMatrix on a camera object
        virtual glm::mat4 getViewMatrix() {
            if(mViewDirty) {
                mViewMatrix = glm::translate(glm::mat4_cast(mQuaternion), -mPosition);
                mInverseViewMatrix = glm::inverse(mViewMatrix);
                mViewDirty = false;
            }
            return mViewMatrix;
        }

        virtual glm::mat4 getInverseViewMatrix() {
            getViewMatrix();
            return mInverseViewMatrix;
        }

        // TODO:
        virtual void setViewMatrix(const glm::mat4& matrix, bool updateTRS = true) {
            //transforms.setMatrix(glm::inverse(matrix), updateTRS);
        }

        virtual glm::mat4 getProjectionMatrix() {
            if(mProjectionDirty) {
                updateProjectionMatrix();
                mInverseProjectionMatrix = glm::inverse(mProjectionMatrix);
                mProjectionDirty = false;
            }
            return mProjectionMatrix;
        }

        virtual glm::mat4 getInverseProjectionMatrix() {
            getProjectionMatrix();
            return mInverseProjectionMatrix;
        }

        virtual void updateProjectionMatrix() = 0;

        glm::vec4 project(const glm::vec4& v) {
            return getProjectionMatrix() * getViewMatrix() * v;
        }

        glm::vec4 unproject(const glm::vec4& v) {
            return getInverseViewMatrix() * getInverseProjectionMatrix() * v;
        }
    };

    class PerspectiveCamera : public Camera {
    public:
        float mFovY, mAspect, mNear, mFar;

        PerspectiveCamera() : mFovY(45.0f), mAspect(1.0f), mNear(0.1f), mFar(100.0f) {
            mProjectionDirty = true;
        }
        PerspectiveCamera(float fovy, float aspect, float _near, float _far) {
            set(fovy, aspect, _near, _far);
        }
        ~PerspectiveCamera() {}

        float getFOV() const {return mFovY;}
        void setFOV(float fovy) {mFovY = fovy; mProjectionDirty = true;}

        float getAspect() const {return mAspect;}
        void setAspect(float aspect) {mAspect = aspect; mProjectionDirty = true;}

        float getNear() const {return mNear;}
        void setNear(float _near) {mNear = _near; mProjectionDirty = true;}

        float getFar() const {return mFar;}
        void setFar(float _far) {mFar = _far; mProjectionDirty = true;}

        void set(float fovy, float aspect) {
            set(fovy, aspect, mNear, mFar);
        }

        void set(float fovy, float aspect, float _near, float _far) {
            mFovY = fovy, mAspect = aspect, mNear = _near, mFar = _far;
            mProjectionDirty = true;
        }

        void updateProjectionMatrix() {
            mProjectionMatrix = glm::perspective(mFovY, mAspect, mNear, mFar);
        }
    };

    class OrthographicCamera : public Camera {
    public:
        float mLeft, mRight, mTop, mBottom, mNear, mFar;

        OrthographicCamera() : mLeft(-1.0f), mRight(1.0f), mTop(-1.0f), mBottom(1.0f), mNear(-1.0f), mFar(1.0f) {
            mProjectionDirty = true;
        }
        OrthographicCamera(float left, float right, float top, float bottom, float _near = -1.0f, float _far = 1.0f) {
            set(left, right, top, bottom, _near, _far);
        }
        ~OrthographicCamera() {}

        float getLeft() const {return mLeft;}
        void setLeft(float left) {mLeft = left; mProjectionDirty = true;}

        float getRight() const {return mRight;}
        void setRight(float right) {mRight = right; mProjectionDirty = true;}

        float getTop() const {return mTop;}
        void setTop(float top) {mTop = top; mProjectionDirty = true;}

        float getBottom() const {return mBottom;}
        void setBottom(float bottom) {mBottom = bottom; mProjectionDirty = true;}

        float getNear() const {return mNear;}
        void setNear(float _near) {mNear = _near; mProjectionDirty = true;}

        float getFar() const {return mFar;}
        void setFar(float _far) {mFar = _far; mProjectionDirty = true;}

        void setNearFar(float _near, float _far) {
            mNear = _near, mFar = _far;
            mProjectionDirty = true;
        }

        void set(float left, float right, float top, float bottom) {
            set(left, right, top, bottom, mNear, mFar);
        }

        void set(float left, float right, float top, float bottom, float _near, float _far) {
            mLeft = left; mRight = right; mTop = top; mBottom = bottom; mNear = _near, mFar = _far;
            mProjectionDirty = true;
        }

        void updateProjectionMatrix() {
            mProjectionMatrix = glm::ortho(mLeft, mRight, mTop, mBottom, mNear, mFar);
        }
    };
}
