#include "mesh_vertexaccessor.hpp"

namespace ngn {
    // Template specializations for attribute types
    // TODO: Think about generating the code for these.
    /*
        Missing:

        I8, I16, I32, UI8, UI16, UI32 * 1, norm -> float
        I8, I16, I32, UI8, UI16, UI32 * 2, norm -> vec2
        I8, I16, I32, UI8, UI16, UI32 * 3, norm -> vec3
        I8, I16, I32, UI8, UI16, UI32 * 4, norm -> vec4

        I8, I16, I32, UI8, UI16, UI32 * 1 -> respective types
        I8, I16, I32, UI8, UI16, UI32 * 2 -> vec2<respective type>
        I8, I16, I32, UI8, UI16, UI32 * 3 -> vec3<respective type>
        I8, I16, I32, UI8, UI16, UI32 * 4 -> vec4<respective type>

        I2_10_10_10 -> vec4<uint16_t>
        UI2_10_10_10 -> vec4<uint16_t>

        I2_10_10_10, norm -> vec4 (2 most significant bits to w/alpha component - last component)
        UI2_10_10_10, norm -> vec4

        F32 * 2 -> vec4 (for 3d positions, even with w)
        F32 * 3 -> vec4
     */

    template<typename dataType, typename vecType>
    void copyElements(dataType* data, const vecType& from, int num) {
        for(int i = 0; i < num; ++i) data[i] = from[i];
    }

    ////////// float
    template<>
    float VertexAttributeAccessor<float>::getWrapped(int index) const {
        // F32 * 1
        if(mAttribute->dataType == AttributeDataType::F32 && mAttribute->num == 1) return *(getPointer<float>(index));
        assert(false && "You seem to be using the wrong data type with this vertex attribute");
    }

    template<>
    void VertexAttributeAccessor<float>::setWrapped(int index, const float& val) {
        // F32 * 1
        if(mAttribute->dataType == AttributeDataType::F32 && mAttribute->num == 1) {
            *(getPointer<float>(index)) = val;
            return;
        }
        assert(false && "You seem to be using the wrong data type with this vertex attribute");
    }

    ////////// glm::vec2
    template<>
    glm::vec2 VertexAttributeAccessor<glm::vec2>::getWrapped(int index) const {
        // F32 * 2
        if(mAttribute->dataType == AttributeDataType::F32 && mAttribute->num == 2) return glm::make_vec2(getPointer<float>(index));
        assert(false && "You seem to be using the wrong data type with this vertex attribute");
    }

    template<>
    void VertexAttributeAccessor<glm::vec2>::setWrapped(int index, const glm::vec2& val) {
        // F32 * 2
        if(mAttribute->dataType == AttributeDataType::F32 && mAttribute->num == 2) {
            copyElements(getPointer<float>(index), val, 2);
            return;
        }
        assert(false && "You seem to be using the wrong data type with this vertex attribute");
    }

    ////////// glm::vec3
    template<>
    glm::vec3 VertexAttributeAccessor<glm::vec3>::getWrapped(int index) const {
        // F32 * 3
        if(mAttribute->dataType == AttributeDataType::F32 && mAttribute->num == 3) return glm::make_vec3(getPointer<float>(index));
        assert(false && "You seem to be using the wrong data type with this vertex attribute");
    }

    template<>
    void VertexAttributeAccessor<glm::vec3>::setWrapped(int index, const glm::vec3& val) {
        // F32 * 3
        if(mAttribute->dataType == AttributeDataType::F32 && mAttribute->num == 3) {
            copyElements(getPointer<float>(index), val, 3);
            return;
        }
        assert(false && "You seem to be using the wrong data type with this vertex attribute");
    }


    ////////// glm::vec4
    template<>
    glm::vec4 VertexAttributeAccessor<glm::vec4>::getWrapped(int index) const {
        // F32 * 4
        if(mAttribute->dataType == AttributeDataType::F32 && mAttribute->num == 4) return glm::make_vec4(getPointer<float>(index));
        assert(false && "You seem to be using the wrong data type with this vertex attribute");
    }

    template<>
    void VertexAttributeAccessor<glm::vec4>::setWrapped(int index, const glm::vec4& val) {
        // F32 * 4
        if(mAttribute->dataType == AttributeDataType::F32 && mAttribute->num == 4) {
            copyElements(getPointer<float>(index), val, 4);
            return;
        }
        assert(false && "You seem to be using the wrong data type with this vertex attribute");
    }

    ////////// int16_t
    ////////// uint16_t
    ////////// int32_t
    ////////// uint32_t

    ////////// int8_t
    ////////// glm::vec2<int8_t>
    ////////// glm::vec3<int8_t>
    ////////// glm::vec4<int8_t>

    ////////// uint8_t
    ////////// glm::vec2<uint8_t>
    ////////// glm::vec3<uint8_t>
    ////////// glm::vec4<uint8_t>

    ////////// int16_t
    ////////// glm::vec2<int16_t>
    ////////// glm::vec3<int16_t>
    ////////// glm::vec4<int16_t>

    ////////// uint16_t
    ////////// glm::vec2<uint16_t>
    ////////// glm::vec3<uint16_t>
    ////////// glm::vec4<uint16_t>

    ////////// int32_t
    ////////// glm::vec2<int32_t>
    ////////// glm::vec3<int32_t>
    ////////// glm::vec4<int32_t>

    ////////// uint32_t
    ////////// glm::vec2<uint32_t>
    ////////// glm::vec3<uint32_t>
    ////////// glm::vec4<uint32_t>
}