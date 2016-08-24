#pragma once

#include "object.hpp"

namespace ngn {
    class Camera : public Object {
    protected:
        glm::mat4 mViewMatrix, mInverseViewMatrix;
        bool mViewDirty, mInverseViewDirty;
        glm::mat4 mProjectionMatrix, mInverseProjectionMatrix;
        bool mProjectionDirty, mInverseProjectionDirty;

    public:
        Camera() : mViewDirty(false), mInverseViewDirty(false), mInverseProjectionDirty(false) {}
        ~Camera() {}

        // Here I will reuse Object::mModelDirty and hope that no one calls getModelMatrix on a camera object
        virtual glm::mat4 getViewMatrix() {
            if(mModelDirty) {
                mViewMatrix = glm::translate(glm::mat4(), -mPosition) * glm::mat4_cast(mQuaternion);
                mModelDirty = false;
            }
            return mViewMatrix;
        }

        virtual void setViewMatrix(const glm::mat4& matrix, bool updateTRS = true) {
            mViewMatrix = matrix;
            mModelDirty = false;
            if(updateTRS) {
                updateTRSFromModelMatrix();
                mPosition = -mPosition;
                mQuaternion = glm::conjugate(mQuaternion);
            }
        }

        virtual glm::mat4 getInverseViewMatrix() {
            if(mInverseViewDirty) {
                mInverseViewMatrix = glm::inverse(getViewMatrix());
                mInverseViewDirty = false;
            }
            return mInverseViewMatrix;
        }

        virtual glm::mat4 getProjectionMatrix() {
            if(mProjectionDirty) {
                updateProjectionMatrix();
                mProjectionDirty = false;
            }
            return mProjectionMatrix;
        }

        virtual glm::mat4 getInverseProjectionMatrix() {
            if(mInverseProjectionDirty) {
                mInverseProjectionMatrix = glm::inverse(getProjectionMatrix());
                mInverseProjectionDirty = false;
            }
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
            mInverseProjectionDirty = mProjectionDirty = true;
        }
        PerspectiveCamera(float fovy, float aspect, float _near, float _far) {
            set(fovy, aspect, _near, _far);
        }
        ~PerspectiveCamera() {}

        float getFOV() const {return mFovY;}
        void setFOV(float fovy) {mFovY = fovy; mInverseProjectionDirty = mProjectionDirty = true;}

        float getAspect() const {return mAspect;}
        void setAspect(float aspect) {mAspect = aspect; mInverseProjectionDirty = mProjectionDirty = true;}

        float getNear() const {return mNear;}
        void setNear(float _near) {mNear = _near; mInverseProjectionDirty = mProjectionDirty = true;}

        float getFar() const {return mFar;}
        void setFar(float _far) {mFar = _far; mInverseProjectionDirty = mProjectionDirty = true;}

        void set(float fovy, float aspect) {
            set(fovy, aspect, mNear, mFar);
        }

        void set(float fovy, float aspect, float _near, float _far) {
            mFovY = fovy, mAspect = aspect, mNear = _near, mFar = _far;
            mInverseProjectionDirty = mProjectionDirty = true;
        }

        void updateProjectionMatrix() {
            mProjectionMatrix = glm::perspective(mFovY, mAspect, mNear, mFar);
        }
    };

    class OrthographicCamera : public Camera {
    public:
        float mLeft, mRight, mTop, mBottom, mNear, mFar;

        OrthographicCamera() : mLeft(-1.0f), mRight(1.0f), mTop(-1.0f), mBottom(1.0f), mNear(-1.0f), mFar(1.0f) {
            mInverseProjectionDirty = mProjectionDirty = true;
        }
        OrthographicCamera(float left, float right, float top, float bottom, float _near = -1.0f, float _far = 1.0f) {
            set(left, right, top, bottom, _near, _far);
        }
        ~OrthographicCamera() {}

        float getLeft() const {return mLeft;}
        void setLeft(float left) {mLeft = left; mInverseProjectionDirty = mProjectionDirty = true;}

        float getRight() const {return mRight;}
        void setRight(float right) {mRight = right; mInverseProjectionDirty = mProjectionDirty = true;}

        float getTop() const {return mTop;}
        void setTop(float top) {mTop = top; mInverseProjectionDirty = mProjectionDirty = true;}

        float getBottom() const {return mBottom;}
        void setBottom(float bottom) {mBottom = bottom; mInverseProjectionDirty = mProjectionDirty = true;}

        float getNear() const {return mNear;}
        void setNear(float _near) {mNear = _near; mInverseProjectionDirty = mProjectionDirty = true;}

        float getFar() const {return mFar;}
        void setFar(float _far) {mFar = _far; mInverseProjectionDirty = mProjectionDirty = true;}

        void setNearFar(float _near, float _far) {
            mNear = _near, mFar = _far;
            mInverseProjectionDirty = mProjectionDirty = true;
        }

        void set(float left, float right, float top, float bottom) {
            set(left, right, top, bottom, mNear, mFar);
        }

        void set(float left, float right, float top, float bottom, float _near, float _far) {
            mLeft = left; mRight = right; mTop = top; mBottom = bottom; mNear = _near, mFar = _far;
            mInverseProjectionDirty = mProjectionDirty = true;
        }

        void updateProjectionMatrix() {
            mProjectionMatrix = glm::ortho(mLeft, mRight, mTop, mBottom, mNear, mFar);
        }
    };
}
