#pragma once

#include <glm/glm.hpp>

#include "mesh_vertexdata.hpp"

namespace ngn {
    class VertexAttributeAccessor;
    class VertexFormat;
    class VertexAttribute;

    template<typename T>
    class VertexAttributeAssigner {
    private:
        VertexAttributeAccessor& mAccessor;
        const int mIndex;

    public:
        VertexAttributeAccessor(const VertexAttributeAccessor& accessor, int index) :
                mAccessor(accessor), mIndex(index) {}

        T& operator=(const T& val) {
            mAccessor.set(index, val);
            return val;
        }
    };

    class VertexAttributeAccessor {
    private:
        const VertexFormat& mFormat;
        const int mAttributeIndex;
        void* mData;

        template<class T>
        T* getPointer(int index) {
            // every attribute is aligned to 4 bytes
            return mData + mFormat.getAttrCount() * 4 * index + mAttributeIndex * 4;
        }

    public:
        VertexAttributeAccessor(const VertexFormat& format, int attrIndex, void* data) :
                mFormat(format), mAttributeIndex(attrIndex), mData(data) {}

        template<typename T>
        T get(int index);

        template<typename T>
        void set(int index, const T& val);

        template<typename T>
        const T operator[](int index) const {
            return get<T>(index);
        }

        template<typename T>
        T& operator[](const int index) {
            return VertexAttributeAssigner(*this, index);
        }
    };

    // Template specializations for attribute types

    /*
        Missing:
        unnormalized integer types?? - convert to what? - probably glm::vec<uint8_t> etc.

        I8, I16, I32, UI8, UI16, UI32 * 1, norm -> float
        I8, I16, I32, UI8, UI16, UI32 * 2, norm -> vec2
        I8, I16, I32, UI8, UI16, UI32 * 3, norm -> vec3
        I8, I16, I32, UI8, UI16, UI32 * 4, norm -> vec4
        UI2_10_10_10, norm -> vec4

        high priority:
        map types to itself
        F32 * 1 -> float
        F32 * 2 -> vec2
        F32 * 4 -> vec4
        I2_10_10_10 * 1 -> vec4 (2 most significant bits to w/alpha component - last component), norm -> vec4
     */

    template<>
    glm::vec3 VertexAttributeAccessor::get<glm::vec3>(int index) const {
        if(mAttribute.type == AttributeDataType::F32 && mAttribute.num == 3) {
            const float* data = getPointer(index);
            return glm::vec3(data[0], data[1], data[2]);
        } else {
            assert(true) // You seem to be using the wrong data type with this vertex attribute
        }
    }

    template<>
    void VertexAttributeAccessor::set<glm::vec3>(int index, const glm::vec3& val) {
        if(mAttribute.type == AttributeDataType::F32 && mAttribute.num == 3) {
            const float* data = getPointer(index);
            data[0] = val.x;
            data[1] = val.y;
            data[2] = val.z;
        } else {
            assert(true) // You seem to be using the wrong data type with this vertex attribute
        }
    }
}