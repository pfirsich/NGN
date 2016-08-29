#pragma once

#include <string>
#include <vector>
#include <cstring>
#include <utility>
#include <memory>

#include <glad/glad.h>
#include <glm/glm.hpp>

#include "mesh_vertexattribute.hpp"
#include "mesh_vertexaccessor.hpp"

namespace ngn {
    enum class UsageHint : GLenum {
        STATIC = GL_STATIC_DRAW,
        STREAM = GL_STREAM_DRAW,
        DYNAMIC = GL_DYNAMIC_DRAW
    };

    struct VertexFormat {
    private:
        std::vector<VertexAttribute> mAttributes;
        std::vector<int> mAttributeOffsets;
        int mStride;
        int mAttributeCount;

    public:
        VertexFormat() : mStride(0), mAttributeCount(0) {}

        const std::vector<VertexAttribute>& getAttributes() const {return mAttributes;}
        int getAttributeCount() const {return mAttributeCount;}
        int getStride() const {return mStride;}
        int getAttributeOffset(int index) const {return mAttributeOffsets[index];}

        template <typename... Ts>
        void add(Ts&&... args) {
            mAttributes.emplace_back(std::forward<Ts>(args)...);
            mAttributeOffsets.push_back(mStride);
            const VertexAttribute& attr = mAttributes.back();
            mStride += getAttributeDataTypeSize(attr.dataType) * attr.alignedNum;
            mAttributeCount++;
        }

        bool hasAttribute(AttributeType attrType) const;
        bool hasAttribute(const char* name) const;

        // These return a VertexAttributeAccessor instance that represents an invalid state (doesn't read or write)
        // if the attribute does not exist. use isValid()
        template<typename T>
        VertexAttributeAccessor<T> getAccessor(const char* name, void* data) const {
            // If we want the vector to be reorder-able and resizable, we have to look for the element every time
            // Also if the number of elements is small (which it mostly is), this is probably even faster than std::map
            for(std::size_t i = 0; i < mAttributes.size(); ++i) {
                if(mAttributes[i].name == name) {
                    return VertexAttributeAccessor<T>(mAttributes[i], getStride(), getAttributeOffset(i), data);
                }
            }
            printf("There is no attribute with name '%s'!", name);
            return VertexAttributeAccessor<T>();
        }

        template<typename T>
        VertexAttributeAccessor<T> getAccessor(AttributeType attrType, void* data) const {
            for(std::size_t i = 0; i < mAttributes.size(); ++i) {
                if(mAttributes[i].type == attrType) {
                    return VertexAttributeAccessor<T>(mAttributes[i], getStride(), getAttributeOffset(i), data);
                }
            }
            printf("There is no attribute of type '%s', make sure to call hasAttribute!", AttributeTypeNames[static_cast<int>(attrType)]);
            return VertexAttributeAccessor<T>();
        }
    };

    class GLBuffer {
    protected:
        GLenum mTarget;
        int mSize;
        UsageHint mUsage;
        using VBODataType = uint8_t;
        std::unique_ptr<VBODataType[]> mData; // Actually we don't care about the type, but we can't declare a std::unique_ptr<void>
        // this is because void doesn't have a destructor, making that type incomplete
        GLuint mVBO;
        int mLastUploadedSize;
        bool mUploadedOnce;

    public:
        GLBuffer(GLenum target, void* data, size_t size, UsageHint usage) :
                mTarget(target), mSize(size), mUsage(usage), mData(reinterpret_cast<VBODataType*>(data)),
                mVBO(0), mLastUploadedSize(0), mUploadedOnce(false) {}

        ~GLBuffer() {
            if(mVBO != 0) glDeleteBuffers(1, &mVBO);
        }

        GLBuffer(const GLBuffer& other) = delete;
        GLBuffer& operator=(const GLBuffer& other) = delete;

        // http://hacksoflife.blogspot.de/2015/06/glmapbuffer-no-longer-cool.html - Don't implement map()?

        void upload();

        virtual void reallocate(size_t num, bool copyOld) = 0;

        virtual void* getData() {
            return mData.get();
        }

        // If you uploaded your data, you can call release to delete the local copy
        void freeLocal() {
            mData.reset();
        }

        void bind() {
            if(!mUploadedOnce) upload();
            glBindBuffer(mTarget, mVBO);
        }

        void unbind() const {
            glBindBuffer(mTarget, 0);
        }
    };

    // VertexBuffer/IndexBuffer represent the vertex data in it.
    // That means that they have full ownership of the data, you may reallocate it or get access to the data they points to
    // but never change the data they point to

    class VertexBuffer : public GLBuffer {
    private:
        const VertexFormat& mVertexFormat;
        size_t mNumVertices;

    public:
        VertexBuffer(const VertexFormat& format, UsageHint usage = UsageHint::STATIC) :
                GLBuffer(GL_ARRAY_BUFFER, nullptr, 0, usage),
                mVertexFormat(format), mNumVertices(0) {}

        VertexBuffer(const VertexFormat& format, size_t numVertices, UsageHint usage = UsageHint::STATIC) :
                GLBuffer(GL_ARRAY_BUFFER, nullptr, 0, usage),
                mVertexFormat(format) {
            reallocate(numVertices);
        }

        // This constructor will *take ownership* of the data pointed to by data
        // If you are using a std::unique_ptr yourself, you therefore have to use std::move
        VertexBuffer(const VertexFormat& format, void* data, size_t numVertices, UsageHint usage = UsageHint::STATIC) :
                GLBuffer(GL_ARRAY_BUFFER, data, format.getStride()*numVertices, usage),
                mVertexFormat(format), mNumVertices(numVertices) {}

        // If nothing has been allocated yet, also call this function
        void reallocate(size_t numVertices, bool copyOld = false);

        size_t getNumVertices() const {return mNumVertices;}
        const VertexFormat& getVertexFormat() const {return mVertexFormat;}

        template<typename T, typename argType>
        VertexAttributeAccessor<T> getAccessor(argType id) {
            return mVertexFormat.getAccessor<T>(id, mData.get());
        }

        // geometry manipulation
        void calculateVertexNormals(bool faceAreaWeighted);
        // sets normals so that in the fragment shader the normals can be interpolated using a "flat" varying - https://www.opengl.org/wiki/Type_Qualifier_(GLSL)
        void calculateFaceNormals(bool lastVertexConvention = true);
        // moves center to 0, 0, 0 and radius to 1.0 if rescale = true
        void calculateTangents();

        void normalize(bool rescale = false);
        void transform(const glm::mat4& transform);
        void merge(const VertexBuffer& other, const glm::mat4& transform);

        VertexBuffer* convertVertexFormat(const VertexFormat& format);

        // pair of position and sizes
        std::pair<glm::vec3, glm::vec3> boundingBox();
        // position and radius
        std::pair<glm::vec3, float> boundingSphere();
    };

    enum class IndexBufferType : GLenum {
        UI8 = GL_UNSIGNED_BYTE,
        UI16 = GL_UNSIGNED_SHORT,
        UI32 = GL_UNSIGNED_INT
    };

    int getIndexBufferTypeSize(IndexBufferType type);
    IndexBufferType getIndexBufferType(size_t vertexCount);

    class IndexBufferAssigner {
    private:
        void* mData;
        size_t mIndex;
        IndexBufferType mDataType;

    public:
        IndexBufferAssigner(void* data, size_t index, IndexBufferType dataType) : mData(data), mIndex(index), mDataType(dataType) {}

        template<typename T>
        const T& operator=(const T& val) {
            switch(mDataType) {
                case IndexBufferType::UI8:
                    *(reinterpret_cast< uint8_t*>(mData) + mIndex) = val;
                    break;
                case IndexBufferType::UI16:
                    *(reinterpret_cast<uint16_t*>(mData) + mIndex) = val;
                    break;
                case IndexBufferType::UI32:
                    *(reinterpret_cast<uint32_t*>(mData) + mIndex) = val;
                    break;
            }
            return val;
        }
    };

    class IndexBuffer : public GLBuffer {
    private:
        IndexBufferType mDataType;
        size_t mNumIndices;

    public:
        IndexBuffer(IndexBufferType dataType, UsageHint usage = UsageHint::STATIC) :
                GLBuffer(GL_ELEMENT_ARRAY_BUFFER, nullptr, 0, usage),
                mDataType(dataType), mNumIndices(0) {}

        IndexBuffer(IndexBufferType dataType, size_t num, UsageHint usage = UsageHint::STATIC) :
                GLBuffer(GL_ELEMENT_ARRAY_BUFFER, nullptr, 0, usage),
                mDataType(dataType), mNumIndices(num) {
            reallocate(num);
        }

        IndexBuffer(uint8_t* data, size_t num, UsageHint usage = UsageHint::STATIC) :
                GLBuffer(GL_ELEMENT_ARRAY_BUFFER, data, num*sizeof(uint8_t), usage),
                mDataType(IndexBufferType::UI8), mNumIndices(num) {}
        IndexBuffer(uint16_t* data, size_t num, UsageHint usage = UsageHint::STATIC) :
                GLBuffer(GL_ELEMENT_ARRAY_BUFFER, data, num*sizeof(uint16_t), usage),
                mDataType(IndexBufferType::UI16), mNumIndices(num) {}
        IndexBuffer(uint32_t* data, size_t num, UsageHint usage = UsageHint::STATIC) :
                GLBuffer(GL_ELEMENT_ARRAY_BUFFER, data, num*sizeof(uint32_t), usage),
                mDataType(IndexBufferType::UI32), mNumIndices(num) {}

        size_t getNumIndices() const {return mNumIndices;}
        IndexBufferType getDataType() const {return mDataType;}

        template<typename T>
        T* getData() {
            return reinterpret_cast<T*>(mData.get());
        }

        void reallocate(size_t numIndices, bool copyOld = false);

        uint32_t operator[](size_t index) const {
            switch(mDataType) {
                case IndexBufferType::UI8:
                    return static_cast<uint32_t>(*(reinterpret_cast<uint8_t*>(mData.get()) + index));
                    break;
                case IndexBufferType::UI16:
                    return static_cast<uint32_t>(*(reinterpret_cast<uint16_t*>(mData.get()) + index));
                    break;
                case IndexBufferType::UI32:
                    return static_cast<uint32_t>(*(reinterpret_cast<uint32_t*>(mData.get()) + index));
                    break;
            }
        }

        IndexBufferAssigner operator[](size_t index) {
            return IndexBufferAssigner(mData.get(), index, mDataType);
        }
    };
}