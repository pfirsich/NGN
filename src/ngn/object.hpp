#pragma once

#include <vector>
#include <map>
#include <algorithm>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace ngn {
    // maybe let Object instances have names or tags, they can be found by.
    class Object {
    public:
        using ObjectId = uint32_t;

    private:
        ObjectId mId;
        Object* mParent;
        std::vector<Object*> mChildren;

    public:
        static ObjectId nextId;
        static std::map<ObjectId, Object*> objectIdMap;

        static Object* getById(ObjectId id) {
            auto it = objectIdMap.find(id);
            if(it != objectIdMap.end()) {
                return it->second;
            } else {
                return nullptr;
            }
        }

        Object() : mParent(nullptr) {
            mId = nextId;
            objectIdMap[mId] = this;
            nextId++;
        }
        ~Object() {
            if(mParent != nullptr) {
                mParent->remove(this);
            }
        }

        // -------------------------------------------------- "Maintenance" - ids, children

        ObjectId getId() const {return mId;}
        // if you use setId, please make sure Object::nextId has a valid value after it
        // probably something like: Object::nextId == std::max(Object::nextId + 1, theIdISetTo);
        void setId(ObjectId id) {
            mId = id;
            objectIdMap.erase(id);
            objectIdMap[id] = this;
        }

        Object* getParent() {return mParent;}

        // you may add a node twice to the graph, which is not intended, but the overhead of checking is undesirable
        void add(Object* obj) {
            obj->mParent = this;
            mChildren.push_back(obj);
        }

        void remove(Object* obj) {
            auto it = mChildren.begin();
            while(it != mChildren.end()) {
                if(*it == obj) {
                    (*it)->mParent = nullptr;
                    it = mChildren.erase(it);
                } else {
                    ++it;
                }
            }
        }

        // -------------------------------------------------- Transforms
    protected:
        glm::vec3 mPosition, mScale;
        glm::quat mQuaternion;
        glm::mat4 mModelMatrix;
        bool mModelDirty;

    protected:
        void updateTRSFromModelMatrix();

    public:
        // To get a "looking direction" in world space, just use obj.getWorldMatrix() * vector, with vector being
        // the looking direction in object space (probably x/y/z axis) (w component = 0)

        void setPosition(const glm::vec3& position) {mPosition = position; mModelDirty = true;}
        glm::vec3 getPosition() const {return mPosition;}

        void setScale(const glm::vec3& scale) {mScale = scale; mModelDirty = true;}
        glm::vec3 getScale() const {return mScale;}

        void setQuaternion(const glm::quat& quat) {mQuaternion = quat; mModelDirty = true;}
        glm::quat getQuaternion() const {return mQuaternion;}
        void rotate(const glm::quat& quat) {mQuaternion = glm::normalize(quat * mQuaternion); mModelDirty = true;}
        void rotate(float angleRadians, const glm::vec3& axis) {
            // this normalization is not necessary mathematically, but technically (because of floating point numbers) it is
            mQuaternion = glm::normalize(glm::angleAxis(angleRadians, axis) * mQuaternion);
            mModelDirty = true;
        }
        void turn(float rad) {rotate(rad, mQuaternion * glm::vec3(0.0f, 1.0f, 0.0f));}

        // For FPS camera ust localDirToWorld, then set y = 0 and renormalize
        glm::vec3 localDirToWorld(const glm::vec3& vec) const {return glm::conjugate(mQuaternion) * vec;}
        glm::vec3 getForward() const {return localDirToWorld(glm::vec3(0.0f, 0.0f, -1.0f));}
        glm::vec3 getRight() const {return localDirToWorld(glm::vec3(1.0f, 0.0f, 0.0f));}
        glm::vec3 getUp() const {return localDirToWorld(glm::vec3(0.0f, 1.0f, 0.0f));}

        glm::mat4 getWorldMatrix() {
            if(mParent)
                return mParent->getWorldMatrix() * getModelMatrix();
            else
                return getModelMatrix();
        }

        glm::mat4 getModelMatrix() {
            if(mModelDirty) {
                mModelMatrix = glm::scale(glm::translate(glm::mat4(), mPosition) * glm::mat4_cast(mQuaternion), mScale);
                mModelDirty = false;
            }
            return mModelMatrix;
        }

        void setModelMatrix(const glm::mat4& matrix, bool updateTRS = true) {
            mModelMatrix = matrix;
            mModelDirty = false;
            if(updateTRS) updateTRSFromModelMatrix();
        }

        // this works just as gluLookAt, so may put in "world space up" (this might not work sometimes, but makes everything a lot easier most of the time)
        // it also means, that the negative z-axis is aligned to face "at"
        void lookAt(const glm::vec3& pos, const glm::vec3& at, const glm::vec3& up = glm::vec3(0.0f, 1.0f, 0.0f)) {
            mPosition = pos;
            mModelMatrix = glm::scale(glm::lookAt(pos, at, up), mScale);
            mQuaternion = glm::normalize(glm::quat_cast(mModelMatrix));
        }

        void lookAt(const glm::vec3& at, const glm::vec3& up = glm::vec3(0.0f, 1.0f, 0.0f)) {
            lookAt(mPosition, at, up);
        }
    };

    // So you can type objects that are only supposed to have other objects as children more clearly
    typedef Object Scene;
}
