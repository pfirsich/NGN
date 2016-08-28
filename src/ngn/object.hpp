#pragma once

#include <vector>
#include <map>
#include <algorithm>

#include <glm/glm.hpp>

#include "transforms.hpp"

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

        Transforms transforms;

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

        glm::mat4 getModelMatrix() {
            return transforms.getMatrix();
        }

        glm::mat4 getWorldMatrix() {
            if(mParent)
                return mParent->getWorldMatrix() * getModelMatrix();
            else
                return getModelMatrix();
        }
    };

    // So you can type objects that are only supposed to have other objects as children more clearly
    using Scene = Object;
}
