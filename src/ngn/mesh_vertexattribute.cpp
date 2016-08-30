#include "mesh_vertexattribute.hpp"

namespace ngn {
    const char* attrTypeNames[] = {
        "POSITION",
        "NORMAL",
        "TANGENT",
        "BITANGENT",
        "COLOR0",
        "COLOR1",
        "BONEINDICES",
        "BONEWEIGHTS",
        "TEXCOORD0", "TEXCOORD1", "TEXCOORD2", "TEXCOORD3",
        "CUSTOM0", "CUSTOM1", "CUSTOM2", "CUSTOM3", "CUSTOM4", "CUSTOM5", "CUSTOM6", "CUSTOM7",
    };

    static_assert(static_cast<int>(AttributeType::FINAL_COUNT_ENTRY) == sizeof(attrTypeNames)/sizeof(attrTypeNames[0]),
                  "There should be just as many AttributeType names as there are AttributeTypes");

    const char* getVertexAttributeTypeName(AttributeType attrType) {
        return attrTypeNames[static_cast<int>(attrType)];
    }

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