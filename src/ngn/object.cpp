#include "object.hpp"

namespace ngn {
    Object::ObjectId Object::nextId = 0;
    std::map<Object::ObjectId, Object*> Object::objectIdMap;

    void Object::updateTRSFromModelMatrix() {
        mPosition = glm::vec3(mModelMatrix * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
        mQuaternion = glm::normalize(glm::quat_cast(mModelMatrix));
        // remove the translation and put the scale factors in the columns
        glm::mat3 temp = glm::transpose(glm::mat3(mModelMatrix));
        mScale = glm::vec3(glm::length(temp[0]), glm::length(temp[1]), glm::length(temp[2]));
    }
}