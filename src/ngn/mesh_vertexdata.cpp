#include "mesh_vertexdata.hpp"

namespace ngn {
    bool VertexFormat::hasAttribute(AttributeType attrType) const {
        for(auto& attr : mAttributes)
            if(attr.type == attrType)
                return true;
        return false;
    }

    bool VertexFormat::hasAttribute(const char* name) const {
        for(auto& attr : mAttributes)
            if(attr.name == name)
                return true;
        return false;
    }

    void VBOWrapper::upload() {
        mUploadedOnce = true;
        if(mVBO == 0) {
            glGenBuffers(1, &mVBO);
        }
        glBindBuffer(mTarget, mVBO);
        if(mLastUploadedSize != mSize) {
            glBufferData(mTarget, mSize, mData.get(), static_cast<GLenum>(mUsage));
            mLastUploadedSize = mSize;
        } else {
            glBufferSubData(mTarget, 0, mSize, mData.get());
        }
        glBindBuffer(mTarget, 0);
    }

    void VertexData::reallocate(size_t numVertices, bool copyOld) {
        size_t newSize = mVertexFormat.getStride()*numVertices;
        std::unique_ptr<VBODataType[]> newData(new VBODataType[newSize]);
        if(mData.get() != nullptr && copyOld) std::copy(mData.get(), mData.get() + mSize, newData.get());
        mData.reset(newData.release());
        mNumVertices = numVertices;
        mSize = newSize;
    }

    int getIndexDataTypeSize(IndexDataType type) {
        switch(type) {
            case IndexDataType::UI8:
                return 1; break;
            case IndexDataType::UI16:
                return 2; break;
            case IndexDataType::UI32:
                return 4; break;
        }
        return 0; // This never happens, but g++ whines
    }

    IndexDataType getIndexDataType(size_t vertexCount) {
        if(vertexCount < (1 << 8)) {
            return IndexDataType::UI8;
        } else if(vertexCount < (1 << 16)) {
            return IndexDataType::UI16;
        } else { // If it's even bigger than 1 << 32, this result will be wrong, but there will be no correct answer and things are strange anyways
            return IndexDataType::UI32;
        }
    }

    void IndexData::reallocate(size_t numIndices, bool copyOld) {
        size_t newSize = getIndexDataTypeSize(mDataType)*numIndices;
        std::unique_ptr<VBODataType[]> newData(new VBODataType[newSize]);
        if(mData.get() != nullptr && copyOld) std::copy(mData.get(), mData.get() + mSize, newData.get());
        mData.reset(newData.release());
        mNumIndices = numIndices;
        mSize = newSize;
    }
}