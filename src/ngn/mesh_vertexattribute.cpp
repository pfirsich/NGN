#include "mesh_vertexattribute.hpp"

namespace ngn {
    int getAttributeDataTypeSize(AttributeDataType type) {
        switch(type) {
            case AttributeDataType::I8:
            case AttributeDataType::UI8:
                return 1;
                break;
            case AttributeDataType::I16:
            case AttributeDataType::UI16:
                return 2;
                break;
            default:
                return 4;
                break;
        }
    }
}