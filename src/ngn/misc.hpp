#pragma once

namespace ngn {
    constexpr vec3x = glm::vec3(1.0f, 0.0f, 0.0f);
    constexpr vec3y = glm::vec3(0.0f, 1.0f, 0.0f);
    constexpr vec3z = glm::vec3(0.0f, 0.0f, 1.0f);
    constexpr vec4x = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
    constexpr vec4y = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
    constexpr vec4z = glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);

    // you may use world up for up
    // aligns -z to dir
    glm::quat quatFromLookAt(const glm::vec3& dir, const glm::vec3& up) {
        glm::vec3 right = glm::cross(dir, up);
        up = glm::cross(right, dir); // re-orthogonalize

        glm::quat rot = glm::quat(vec3(0.0f, 0.0f, -1.0f), dir);
        return glm::quat(rot * vec3(0.0f, 1.0f, 0.0f), up) * rot;
    }
}
