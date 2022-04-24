#pragma once

#include <array>
#include <string_view>
#include <map>

#include <pugixml.hpp>

namespace Haviour
{
//////////////////////////    XML Helpers
inline pugi::xml_node appendXmlString(pugi::xml_node target, std::string_view srcString)
{
    // parse XML string as document
    pugi::xml_document doc;
    if (!doc.load_buffer(srcString.data(), srcString.length()))
        return {};

    target.attribute("numelements") = target.attribute("numelements").as_uint() + 1;
    return target.append_copy(doc.first_child());
}

inline pugi::xml_node getNthChild(pugi::xml_node target, uint32_t n)
{
    auto node = target.first_child();
    while (node && (n > 1))
        node = node.next_sibling();
    return node;
}

namespace Hkx
{

struct EnumWrapper
{
    std::string_view name;
    uint32_t         val;
    std::string_view hint = {};
};

//////////////////////////    ENUMS & FLAGS

// hkbVariableInfo
enum VariableTypeEnum : int8_t
{
    VARIABLE_TYPE_INVALID = -1,
    VARIABLE_TYPE_BOOL    = 0,
    VARIABLE_TYPE_INT8,
    VARIABLE_TYPE_INT16,
    VARIABLE_TYPE_INT32,
    VARIABLE_TYPE_REAL,
    VARIABLE_TYPE_POINTER,
    VARIABLE_TYPE_VECTOR3,
    VARIABLE_TYPE_VECTOR4,
    VARIABLE_TYPE_QUATERNION,
};
constexpr auto e_variableType = std::to_array<std::string_view>(
    {
        "VARIABLE_TYPE_INVALID",
        "VARIABLE_TYPE_BOOL", // word-sized (at most)
        "VARIABLE_TYPE_INT8",
        "VARIABLE_TYPE_INT16",
        "VARIABLE_TYPE_INT32",
        "VARIABLE_TYPE_REAL",
        "VARIABLE_TYPE_POINTER", // this must be after the word-sized types and before the quad-sized types
        "VARIABLE_TYPE_VECTOR3", // quad-word sized
        "VARIABLE_TYPE_VECTOR4",
        "VARIABLE_TYPE_QUATERNION",
    });
enum VariableSizeEnum
{
    kVarSizeInvalid,
    kVarSizeWord,
    kVarSizeVariant,
    kVarSizeQuad
};
constexpr VariableTypeEnum getVarTypeEnum(std::string_view enumstr)
{
    int i = 0;
    for (; i < e_variableType.size(); ++i)
        if (e_variableType[i] == enumstr)
            return VariableTypeEnum(i - 1);
    return VARIABLE_TYPE_INVALID;
}

// hkbVariableInfo -> hkbRoleAttribute
constexpr auto e_hkbRoleAttribute_Role = std::to_array<EnumWrapper>(
    {
        {"ROLE_DEFAULT", 0},
        {"ROLE_FILE_NAME", 1},
        {"ROLE_BONE_INDEX", 2, "The property is a bone index (hkInt16) or contains bone indices (hkbBoneIndexArray, hkbBoneWeightArray).\n"
                               "hkbBoneIndexArrays, hkbBoneWeightArrays, and individual bones can be given a group name using hk.Ui(group=\"groupName\").\n"
                               "HAT will use the Select Bones Dialog for groups of related bones."},
        {"ROLE_EVENT_ID", 3, "The property is an event ID (hkInt32). HAT will allow you to choose the event by name."},
        {"ROLE_VARIABLE_INDEX", 4, "The property is a behavior variable index (hkInt32). HAT will allow you to choose a variable by name."},
        {"ROLE_ATTRIBUTE_INDEX", 5, "The property is a behavior attribute index (hkInt32). HAT will allow you to choose an attribute by name."},
        {"ROLE_TIME", 6, "The property is a time in seconds (hkReal) (currently unused)."},
        {"ROLE_SCRIPT", 7, "The property is a script"},
        {"ROLE_LOCAL_FRAME", 8, "The property is a local frame"},
        {"ROLE_BONE_ATTACHMENT", 9, "The property is a bone attachment"},
    });

constexpr auto f_hkbRoleAttribute_roleFlags = std::to_array<EnumWrapper>(
    {
        {"FLAG_NONE", 0},
        {"FLAG_RAGDOLL", 1, "The property refers to the ragdoll skeleton. Without this flag HAT defaults to the animation skeleton.\n"
                            "Use this in conjunction with ROLE_BONE_INDEX, hkbBoneIndexArrays, or hkbBoneWeightArrays."},
        {"FLAG_NORMALIZED", 1 << 1, "The property should be normalized (apply this to hkVector4) (currently unused)."},
        {"FLAG_NOT_VARIABLE", 1 << 2, "HAT will not allow the property to be bound to a variable."},
        {"FLAG_HIDDEN", 1 << 3, "HAT will not show the property in the UI."},
        {"FLAG_OUTPUT", 1 << 4, "By default, all node properties are considered inputs, which means that behavior variable values are copied to them.\n"
                                "If you want the property to serve as an output, you must use this flag.\n"
                                "Note: a property cannot be both an input and an output.\n"},
        {"FLAG_NOT_CHARACTER_PROPERTY", 1 << 5, "HAT will not allow the property to be bound to a character property."},
        {"FLAG_CHAIN", 1 << 6, "HAT will interpret the property as contributing to a bone chain (no siblings allowed).\n"
                               "Use this in conjunction with ROLE_BONE_INDEX or hkbBoneIndexArrays."},
    });

// hkbEventInfo -> Flags
constexpr auto f_hkbEventInfo_Flags = std::to_array<EnumWrapper>(
    {
        {"FLAG_SILENT", 0x1, "Whether or not clip generators should raise the event."},
        {"FLAG_SYNC_POINT", 0x2, "Whether or not the sync point will be"},
    });

// hkbBehaviorGraph
constexpr auto e_variableMode = std::to_array<EnumWrapper>(
    {
        {"VARIABLE_MODE_DISCARD_WHEN_INACTIVE", 0, "Throw away the variable values and memory on deactivate().\n"
                                                   "In this mode, variable memory is allocated and variable values are reset each time activate() is called."},
        {"VARIABLE_MODE_MAINTAIN_VALUES_WHEN_INACTIVE", 1, "Don't discard the variable memory on deactivate(),\n"
                                                           "and don't reset the variable values on activate() (except the first time)."},
    });

// hkbVariableBindingSet -> Binding
constexpr auto e_bindingType = std::to_array<EnumWrapper>(
    {
        {"BINDING_TYPE_VARIABLE"},
        {"BINDING_TYPE_CHARACTER_PROPERTY"},
    });


//////////////////////////    DEFAULT VALUES
constexpr const char* g_def_hkbVariableInfo =
    R"(<hkobject>
    <hkparam name="role">
        <hkobject>
            <hkparam name="role">ROLE_DEFAULT</hkparam>
            <hkparam name="flags">0</hkparam>
        </hkobject>
    </hkparam>
    <hkparam name="type">{}</hkparam>
</hkobject>)"; // This one needs formatting

constexpr const char* g_def_hkbVariableInfo_characterPropertyInfo =
    R"(<hkobject>
    <hkparam name="role">
        <hkobject>
            <hkparam name="role">ROLE_DEFAULT</hkparam>
            <hkparam name="flags">FLAG_OUTPUT|FLAG_HIDDEN|FLAG_NOT_VARIABLE|FLAG_NONE</hkparam>
        </hkobject>
    </hkparam>
    <hkparam name="type">VARIABLE_TYPE_POINTER</hkparam>
</hkobject>)";

constexpr const char* g_def_hkbVariableValue =
    R"(<hkobject>
	<hkparam name="value">0</hkparam>
</hkobject>)";

constexpr const char* g_def_hkStringPtr = R"(<hkcstring>TEXT</hkcstring>)";

constexpr const char* g_def_hkbEventInfo =
    R"(<hkobject>
    <hkparam name="flags">0</hkparam>
</hkobject>)";

// hkbVariableBindingSet -> Binding
constexpr const char* g_def_hkbVariableBindingSet_Binding =
    R"(<hkobject>
    <hkparam name="memberPath">variable_name</hkparam>
    <hkparam name="variableIndex">0</hkparam>
    <hkparam name="bitIndex">-1</hkparam>
    <hkparam name="bindingType">BINDING_TYPE_VARIABLE</hkparam>
</hkobject>)";

inline const std::map<std::string_view, const char*>& getClassDefaultMap()
{
#define MAPITEM(hkclass)          \
    {                             \
#        hkclass, g_def_##hkclass \
    }

    static const std::map<std::string_view, const char*> map = {
        MAPITEM(hkbVariableInfo)};
    return map;
}

} // namespace Hkx

} // namespace Haviour