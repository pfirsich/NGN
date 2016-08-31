#pragma once

#define GLM_META_PROG_HELPERS // number of components etc. as static members

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "mesh_vertexattribute.hpp"

/*template<typename elementType, int elements>
struct GLMVecType {
    typedef void type;
};

template<typename elementType> struct GLMVecType<typename elementType, 2> {
    typedef glm::vec2<elementType> type;
};

template<typename elementType> struct GLMVecType<typename elementType, 3> {
    typedef glm::vec3<elementType> type;
};

template<typename elementType> struct GLMVecType<typename elementType, 4> {
    typedef glm::vec4<elementType> type;
};*/

namespace ngn {
    template<typename T>
    class VertexAttributeAssigner;

    template<typename T>
    class VertexAttributeAccessor {
    private:
        const VertexAttribute* mAttribute; // Would probablyy use std::optional
        const int mStride;
        const int mAttributeOffset;
        const size_t mCount;
        void* mData;

    public:
        // This represents an invalid state
        VertexAttributeAccessor() : mAttribute(nullptr), mStride(0), mAttributeOffset(0), mCount(0), mData(nullptr) {}

        VertexAttributeAccessor(const VertexAttribute& attr, int stride, int offset, size_t num, void* data) :
                mAttribute(&attr), mStride(stride), mAttributeOffset(offset), mCount(num), mData(data) {}

        bool isValid() const {return mData != nullptr;}
        size_t getCount() const {return mCount;}

        // every attribute is aligned to 4 bytes
        template<typename pT>
        const pT* getPointer(int index) const {
            return reinterpret_cast<pT*>(reinterpret_cast<uint8_t*>(mData) + mStride * index + mAttributeOffset);
        }

        template<typename pT>
        pT* getPointer(int index) {
            return reinterpret_cast<pT*>(reinterpret_cast<uint8_t*>(mData) + mStride * index + mAttributeOffset);
        }

        /*template<typename vecType>
        vecType getVec(int index) const {
            const typename vecType::value_type* data = getPointer<typename vecType::value_type>(index);
            if(vecType::components == 2) {
                return glm::make_vec2<typename vecType::value_type>(data);
            } else if(vecType::components == 3) {
                return glm::make_vec3<typename vecType::value_type>(data);
            } else { // vecType::components >= 4
                return glm::make_vec4<typename vecType::value_type>(data);
            }
        }*/

        T getWrapped(int index) const;

        void setWrapped(int index, const T& val);

        T get(int index) const {
            if(mData == nullptr) return T();
            return getWrapped(index);
        }

        void set(int index, const T& val) {
            if(mData == nullptr) return;
            return setWrapped(index, val);
        }

        const T operator[](int index) const {
            return get(index);
        }

        VertexAttributeAssigner<T> operator[](const int index);
    };

    template<typename T>
    class VertexAttributeAssigner {
    private:
        VertexAttributeAccessor<T>& mAccessor;
        int mIndex;

    public:
        VertexAttributeAssigner(VertexAttributeAccessor<T>& accessor, int index) :
                mAccessor(accessor), mIndex(index) {}

        const T& operator=(const T& val) {
            mAccessor.set(mIndex, val);
            return val;
        }
    };

    template<typename T>
    VertexAttributeAssigner<T> VertexAttributeAccessor<T>::operator[](const int index) {
        return VertexAttributeAssigner<T>(*this, index);
    }
}