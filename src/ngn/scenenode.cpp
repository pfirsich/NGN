#include "scenenode.hpp"

namespace ngn {
    SceneNode::Id SceneNode::nextId = 0;
    std::unordered_map<SceneNode::Id, SceneNode*> SceneNode::nodeIdMap;

    void SceneNode::updateTRSFromMatrix() {
        mPosition = glm::vec3(mMatrix * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
        mQuaternion = glm::normalize(glm::quat_cast(mMatrix));
        // remove the translation and put the scale factors in the columns
        glm::mat3 temp = glm::transpose(glm::mat3(mMatrix));
        mScale = glm::vec3(glm::length(temp[0]), glm::length(temp[1]), glm::length(temp[2]));
    }
}