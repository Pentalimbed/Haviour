#include "widgets.h"
#include "hkx/hkclass.inl"
#include "propedit.h"

#include <imgui.h>
#include <spdlog/spdlog.h>

namespace Haviour
{
namespace Ui
{
#define addTooltip(...) \
    if (ImGui::IsItemHovered()) ImGui::SetTooltip(__VA_ARGS__);

bool iconButton(const char* label)
{
    return ImGui::Button(label, ImVec2(10, 10));
}

void statePickerPopup(const char* str_id, pugi::xml_node hkparam, pugi::xml_node state_machine)
{
    if (ImGui::BeginPopup(str_id, ImGuiWindowFlags_AlwaysAutoResize))
    {
        auto&                    file        = Hkx::HkxFileManager::getSingleton()->getCurrentFile();
        auto                     states_node = state_machine.getByName("states");
        std::vector<std::string> objs        = {};
        size_t                   num_objs    = states_node.attribute("numelements").as_ullong();
        std::istringstream       states_stream(states_node.text().as_string());

        for (size_t i = 0; i < num_objs; ++i)
        {
            std::string temp_str;
            states_stream >> temp_str;

            auto state    = file.getObj(temp_str);
            auto state_id = state.getByName("stateId").text().as_int();

            const bool selected = false;
            if (ImGui::Selectable(fmt::format("{:4} {}", state_id, state.getByName("name").text().as_string()).c_str(), selected))
            {
                hkparam.text() = state_id;
                ImGui::CloseCurrentPopup();
                break;
            }
        }

        ImGui::EndPopup();
    }
}

////////////////    EDITS

void stringEdit(pugi::xml_node hkparam, std::string_view hint, std::string_view manual_name)
{
    ImGui::TableNextColumn();
    std::string value = hkparam.text().as_string();
    if (ImGui::InputText(manual_name.empty() ? hkparam.attribute("name").as_string() : manual_name.data(), &value))
        hkparam.text() = value.c_str();
    if (!hint.empty())
        addTooltip(hint.data());

    ImGui::TableNextColumn();
}

void boolEdit(pugi::xml_node hkparam, std::string_view hint, std::string_view manual_name)
{
    ImGui::TableNextColumn();
    bool value = hkparam.text().as_bool();
    if (ImGui::Checkbox(manual_name.empty() ? hkparam.attribute("name").as_string() : manual_name.data(), &value))
        hkparam.text() = value ? "true" : "false";
    if (!hint.empty())
        addTooltip(hint.data());

    ImGui::TableNextColumn();
}

void intScalarEdit(pugi::xml_node hkparam, ImGuiDataType data_type, std::string_view hint, std::string_view manual_name)
{
    ImGui::TableNextColumn();
    union
    {
        uint8_t  u8;
        uint16_t u16;
        uint32_t u32;
        uint64_t u64;
        int8_t   s8;
        int16_t  s16;
        int32_t  s32;
        int64_t  s64;
    } value;

    switch (data_type)
    {
        case ImGuiDataType_U8:
            value.u8 = hkparam.text().as_uint();
            if (ImGui::InputScalar(manual_name.empty() ? hkparam.attribute("name").as_string() : manual_name.data(), data_type, &value.u8))
                hkparam.text() = value.u8;
            break;
        case ImGuiDataType_U16:
            value.u16 = hkparam.text().as_uint();
            if (ImGui::InputScalar(manual_name.empty() ? hkparam.attribute("name").as_string() : manual_name.data(), data_type, &value.u16))
                hkparam.text() = value.u16;
            break;
        case ImGuiDataType_U32:
            value.u32 = hkparam.text().as_uint();
            if (ImGui::InputScalar(manual_name.empty() ? hkparam.attribute("name").as_string() : manual_name.data(), data_type, &value.u32))
                hkparam.text() = value.u32;
            break;
        case ImGuiDataType_U64:
            value.u64 = hkparam.text().as_ullong();
            if (ImGui::InputScalar(manual_name.empty() ? hkparam.attribute("name").as_string() : manual_name.data(), data_type, &value.u64))
                hkparam.text() = value.u64;
        case ImGuiDataType_S8:
            value.s8 = hkparam.text().as_int();
            if (ImGui::InputScalar(manual_name.empty() ? hkparam.attribute("name").as_string() : manual_name.data(), data_type, &value.s8))
                hkparam.text() = value.s8;
            break;
        case ImGuiDataType_S16:
            value.s16 = hkparam.text().as_int();
            if (ImGui::InputScalar(manual_name.empty() ? hkparam.attribute("name").as_string() : manual_name.data(), data_type, &value.s16))
                hkparam.text() = value.s16;
            break;
        case ImGuiDataType_S32:
            value.s32 = hkparam.text().as_int();
            if (ImGui::InputScalar(manual_name.empty() ? hkparam.attribute("name").as_string() : manual_name.data(), data_type, &value.s32))
                hkparam.text() = value.s32;
            break;
        case ImGuiDataType_S64:
            value.s64 = hkparam.text().as_llong();
            if (ImGui::InputScalar(manual_name.empty() ? hkparam.attribute("name").as_string() : manual_name.data(), data_type, &value.s64))
                hkparam.text() = value.s64;
            break;
        default:
            ImGui::TableNextColumn();
            return;
            break;
    }
    if (!hint.empty())
        addTooltip(hint.data());
    ImGui::TableNextColumn();
}

void sliderIntEdit(pugi::xml_node hkparam, int lb, int ub, std::string_view hint, std::string_view manual_name)
{
    ImGui::TableNextColumn();
    int value = hkparam.text().as_int();
    if (ImGui::SliderInt(manual_name.empty() ? hkparam.attribute("name").as_string() : manual_name.data(), &value, lb, ub, "%.6f"))
        hkparam.text() = value;
    if (!hint.empty())
        addTooltip(hint.data());

    ImGui::TableNextColumn();
}

void sliderFloatEdit(pugi::xml_node hkparam, float lb, float ub, std::string_view hint, std::string_view manual_name)
{
    ImGui::TableNextColumn();
    float value = hkparam.text().as_float();
    if (ImGui::SliderFloat(manual_name.empty() ? hkparam.attribute("name").as_string() : manual_name.data(), &value, lb, ub, "%.6f"))
        hkparam.text() = fmt::format("{:.6f}", value).c_str();
    if (!hint.empty())
        addTooltip(hint.data());

    ImGui::TableNextColumn();
}

void floatEdit(pugi::xml_node hkparam, std::string_view hint, std::string_view manual_name)
{
    ImGui::TableNextColumn();
    float value = hkparam.text().as_float();
    if (ImGui::InputFloat(manual_name.empty() ? hkparam.attribute("name").as_string() : manual_name.data(), &value, 0.0f, 0.0f, "%.6f"))
        hkparam.text() = fmt::format("{:.6f}", value).c_str();
    if (!hint.empty())
        addTooltip(hint.data());

    ImGui::TableNextColumn();
}

// for floats written in UINT32 values
void convertFloatEdit(pugi::xml_node hkparam, std::string_view hint, std::string_view manual_name)
{
    ImGui::TableNextColumn();
    auto  raw_value = hkparam.text().as_uint();
    float value     = *reinterpret_cast<float*>(&raw_value);
    if (ImGui::InputFloat(manual_name.empty() ? hkparam.attribute("name").as_string() : manual_name.data(), &value, 0.0f, 0.0f))
        hkparam.text() = *reinterpret_cast<uint32_t*>(&value);
    if (!hint.empty())
        addTooltip(hint.data());

    ImGui::TableNextColumn();
}

void quadEdit(pugi::xml_node hkparam, std::string_view hint, std::string_view manual_name)
{
    ImGui::TableNextColumn();

    float       value[4];
    std::string text(hkparam.text().as_string());
    std::erase_if(text, [](char& c) { return (c == '(') || (c == ')'); });
    std::istringstream text_stream(text);
    text_stream >> value[0] >> value[1] >> value[2] >> value[3];

    if (ImGui::InputFloat4(manual_name.empty() ? hkparam.attribute("name").as_string() : manual_name.data(), value, "%.6f"))
        hkparam.text() = fmt::format("({:.6f} {:.6f} {:.6f} {:.6f})", value[0], value[1], value[2], value[3]).c_str();
    if (!hint.empty())
        addTooltip(hint.data());

    ImGui::TableNextColumn();
}

void refEdit(pugi::xml_node hkparam, const std::vector<std::string_view>& classes, Hkx::HkxFile& file, std::string_view hint, std::string_view manual_name)
{
    ImGui::TableNextColumn();

    ImGui::PushID(manual_name.empty() ? hkparam.attribute("name").as_string() : manual_name.data());

    bool        edited    = false;
    std::string old_value = hkparam.text().as_string();
    auto        value     = old_value;

    if (ImGui::InputText(manual_name.empty() ? hkparam.attribute("name").as_string() : manual_name.data(), &value))
    {
        edited = true;
        if (auto obj = file.getObj(value))                                                                         // Is obj
            if (std::ranges::find(classes, std::string_view(obj.attribute("class").as_string())) != classes.end()) // Same class
                hkparam.text() = value.c_str();
            else
                hkparam.text() = "null";
        else
            hkparam.text() = "null";
    }
    if (!hint.empty())
        addTooltip(hint.data());

    ImGui::TableNextColumn();
    if (ImGui::Button(ICON_FA_ARROW_CIRCLE_RIGHT))
        PropEdit::getSingleton()->setObject(value);
    addTooltip("Go to");

    ImGui::SameLine();
    if (ImGui::BeginPopup("Append"))
    {
        for (auto class_name : classes)
            if (ImGui::Selectable(class_name.data()))
            {
                value          = file.addObj(class_name);
                hkparam.text() = value.c_str();
                edited         = true;
                ImGui::CloseCurrentPopup();
                break;
            }
        ImGui::EndPopup();
    }
    if (ImGui::Button(ICON_FA_PLUS_CIRCLE))
        ImGui::OpenPopup("Append");
    addTooltip("Append");

    if (edited)
    {
        auto parent_obj = getParentObj(hkparam);
        assert(parent_obj);
        auto parent_id = parent_obj.attribute("name").as_string();
        if (!isRefBy(old_value, parent_obj))
            file.deRef(old_value, parent_id);
        file.addRef(value, parent_id);
    }

    ImGui::PopID();
}

void refEdit(std::string&                         value,
             const std::vector<std::string_view>& classes,
             pugi::xml_node                       parent,
             Hkx::HkxFile&                        file,
             std::string_view                     hint,
             std::string_view                     manual_name)
{
    ImGui::TableNextColumn();

    ImGui::PushID(manual_name.data());

    bool        edited    = false;
    std::string old_value = value;

    if (ImGui::InputText(manual_name.data(), &value))
    {
        edited = true;
        if (auto obj = file.getObj(value))                                                                         // Is obj
            if (std::ranges::find(classes, std::string_view(obj.attribute("class").as_string())) == classes.end()) // Different class
                value = "null";
            else
                value = "null";
    }
    if (!hint.empty())
        addTooltip(hint.data());

    ImGui::TableNextColumn();
    if (ImGui::Button(ICON_FA_ARROW_CIRCLE_RIGHT))
        PropEdit::getSingleton()->setObject(value);
    addTooltip("Go to");

    ImGui::SameLine();
    if (ImGui::BeginPopup("Append"))
    {
        for (auto class_name : classes)
            if (ImGui::Selectable(class_name.data()))
            {
                value  = file.addObj(class_name);
                edited = true;
                ImGui::CloseCurrentPopup();
                break;
            }
        ImGui::EndPopup();
    }
    if (ImGui::Button(ICON_FA_PLUS_CIRCLE))
        ImGui::OpenPopup("Append");
    addTooltip("Append");

    if (edited)
    {
        auto parent_id = parent.attribute("name").as_string();
        if (!isRefBy(old_value, parent))
            file.deRef(old_value, parent_id);
        file.addRef(value, parent_id);
    }

    ImGui::PopID();
}

void refList(pugi::xml_node                       hkparam,
             const std::vector<std::string_view>& classes,
             Hkx::HkxFile&                        file,
             std::string_view                     hint_attribute,
             std::string_view                     name_attribute,
             std::string_view                     manual_name)
{
    auto                     parent   = getParentObj(hkparam);
    std::vector<std::string> objs     = {};
    size_t                   num_objs = hkparam.attribute("numelements").as_ullong();
    std::istringstream       states_stream(hkparam.text().as_string());
    for (size_t i = 0; i < num_objs; ++i)
    {
        std::string temp_str;
        states_stream >> temp_str;
        objs.push_back(temp_str);
    }

    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(manual_name.empty() ? hkparam.attribute("name").as_string() : manual_name.data()), ImGui::SameLine();
    if (ImGui::Button(ICON_FA_PLUS_CIRCLE))
    {
        hkparam.attribute("numelements") = hkparam.attribute("numelements").as_int() + 1;
        objs.push_back("null");
    }
    addTooltip("Add new object reference");
    ImGui::SameLine();
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(hkparam.attribute("numelements").as_string());

    constexpr auto table_flag = ImGuiTableFlags_SizingFixedFit |
        ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY |
        ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_NoBordersInBody;
    if (ImGui::BeginTable("states", 2, table_flag, ImVec2(-FLT_MIN, -FLT_MIN)))
    {
        size_t mark_delete = UINT64_MAX;
        bool   do_delete   = false;
        for (size_t i = 0; i < objs.size(); ++i)
        {
            ImGui::PushID(i);
            ImGui::SetNextItemWidth(60);
            refEdit(objs[i], classes, parent, file,
                    Hkx::HkxFileManager::getSingleton()->getCurrentFile().getObj(objs[i]).getByName(hint_attribute.data()).text().as_string(),
                    Hkx::HkxFileManager::getSingleton()->getCurrentFile().getObj(objs[i]).getByName(name_attribute.data()).text().as_string());
            ImGui::SameLine();
            if (ImGui::Button(ICON_FA_MINUS_CIRCLE))
            {
                do_delete   = true;
                mark_delete = i;
            }
            addTooltip("Remove");
            ImGui::PopID();
        }
        if (do_delete)
        {
            objs.erase(objs.begin() + mark_delete);
            hkparam.attribute("numelements") = hkparam.attribute("numelements").as_int() - 1;
        }

        hkparam.text() = printIdVector(objs).c_str();

        ImGui::EndTable();
    }
}

void stateEdit(pugi::xml_node hkparam,
               Hkx::HkxFile&  file,
               //    pugi::xml_node   state_machine,
               std::string_view hint,
               std::string_view manual_name)
{
    ImGui::PushID(manual_name.empty() ? hkparam.attribute("name").as_string() : manual_name.data());

    intScalarEdit(hkparam, 4, hint, manual_name);

    if (ImGui::Button(ICON_FA_SEARCH))
        ImGui::OpenPopup("select state");

    auto state_machine = getParentObj(hkparam);
    while (strcmp(state_machine.attribute("class").as_string(), "hkbStateMachine"))
    {
        std::vector<std::string> refs = {};
        file.getObjRefs(state_machine.attribute("name").as_string(), refs);
        if (refs.empty())
            break;
        state_machine = file.getObj(refs[0]);
    }
    if (strcmp(state_machine.attribute("class").as_string(), "hkbStateMachine"))
        spdlog::warn("Failed to find parent state machine.");
    else
        statePickerPopup("select state", hkparam, state_machine);

    ImGui::PopID();
}

////////////////    Linked Prop Edits

void varEditPopup(const char* str_id, Hkx::Variable& var, Hkx::HkxFile& file)
{
    if (ImGui::BeginPopup(str_id))
    {
        if (ImGui::BeginTable("varedit", 2, ImGuiTableFlags_SizingStretchProp, ImVec2(600, -FLT_MIN)))
        {
            stringEdit(var.get<Hkx::PropName>(), "", "name");
            if (ImGui::Button(ICON_FA_TRASH))
            {
                auto name     = var.get<Hkx::PropName>().text().as_string();
                auto ref_node = file.getFirstVarRef(var.m_index);
                if (ref_node)
                {
                    spdlog::warn("This variable is still referenced by {} and potentially more object.\nID copied to clipboard.", ref_node.attribute("name").as_string());
                    ImGui::SetClipboardText(ref_node.attribute("name").as_string());
                }
                else
                {
                    file.m_var_manager.delEntry(var.m_index);
                    spdlog::info("Variable {} deleted!", name);
                    ImGui::EndTable();
                    ImGui::CloseCurrentPopup();
                    ImGui::EndPopup();
                    return;
                }
            }
            addTooltip("Delete/Mark vairable obsolete\nIf it is still referenced, the variable won't delete\nand one of its referencing object will be copied to clipboard.");

            auto var_type_enum  = Hkx::getVarTypeEnum(var.get<Hkx::PropVarInfo>().getByName("type").text().as_string());
            auto var_value_node = var.get<Hkx::PropWordValue>().first_child();
            switch (var_type_enum)
            {
                case Hkx::VARIABLE_TYPE_BOOL:
                    boolEdit(var_value_node);
                    break;
                case Hkx::VARIABLE_TYPE_INT8:
                    intScalarEdit(var_value_node, ImGuiDataType_S8);
                    break;
                case Hkx::VARIABLE_TYPE_INT16:
                    intScalarEdit(var_value_node, ImGuiDataType_S16);
                    break;
                case Hkx::VARIABLE_TYPE_INT32:
                    intScalarEdit(var_value_node, ImGuiDataType_S32);
                    break;
                case Hkx::VARIABLE_TYPE_REAL:
                    convertFloatEdit(var_value_node);
                    break;
                case Hkx::VARIABLE_TYPE_POINTER:
                    ImGui::TableNextColumn();
                    ImGui::TextDisabled("Currently not supported");
                    ImGui::TableNextColumn();
                    break;
                case Hkx::VARIABLE_TYPE_VECTOR3:
                    ImGui::TableNextColumn();
                    ImGui::InputFloat3("value", file.m_var_manager.getQuadValue(var_value_node.text().as_uint()).data(), "%.6f");
                    ImGui::TableNextColumn();
                    break;
                case Hkx::VARIABLE_TYPE_VECTOR4:
                case Hkx::VARIABLE_TYPE_QUATERNION:
                    ImGui::TableNextColumn();
                    ImGui::InputFloat4("value", file.m_var_manager.getQuadValue(var_value_node.text().as_uint()).data(), "%.6f");
                    ImGui::TableNextColumn();
                    break;
                default:
                    ImGui::TableNextColumn();
                    ImGui::TextColored(ImColor(1.0f, 0.3f, 0.3f, 1.0f), "Invalid value type!");
                    ImGui::TableNextColumn();
                    break;
            }

            auto role_node = var.get<Hkx::PropVarInfo>().getByName("role").first_child();
            enumEdit(role_node.getByName("role"), Hkx::e_hkbRoleAttribute_Role);
            flagEdit(role_node.getByName("flags"), Hkx::f_hkbRoleAttribute_roleFlags);

            ImGui::EndTable();
        }
        ImGui::EndPopup();
    }
}

void evtEditPopup(const char* str_id, Hkx::AnimationEvent& evt, Hkx::HkxFile& file)
{
    if (ImGui::BeginPopup(str_id))
    {
        if (ImGui::BeginTable("eventedit", 2, ImGuiTableFlags_SizingStretchProp, ImVec2(300.0, 0.0)))
        {
            stringEdit(evt.get<Hkx::PropName>(), "", "name");
            if (ImGui::Button(ICON_FA_TRASH))
            {
                auto name     = evt.get<Hkx::PropName>().text().as_string();
                auto ref_node = file.getFirstEventRef(evt.m_index);
                if (ref_node)
                {
                    spdlog::warn("This event is still referenced by {} and potentially more object.\nID copied to clipboard.", ref_node.attribute("name").as_string());
                    ImGui::SetClipboardText(ref_node.attribute("name").as_string());
                }
                else
                {
                    file.m_evt_manager.delEntry(evt.m_index);
                    spdlog::info("Event {} deleted!", name);
                    ImGui::EndTable();
                    ImGui::CloseCurrentPopup();
                    ImGui::EndPopup();
                    return;
                }
            }
            addTooltip("Delete\nIf it is still referenced, the event won't delete\nand one of its referencing object will be copied to clipboard.");

            flagEdit(evt.get<Hkx::PropEventInfo>().getByName("flags"), Hkx::f_hkbEventInfo_Flags);

            ImGui::EndTable();
        }
        ImGui::EndPopup();
    }
}

void propEditPopup(const char* str_id, Hkx::CharacterProperty& prop, Hkx::HkxFile& file)
{
    if (ImGui::BeginPopup(str_id))
    {
        if (ImGui::BeginTable("eventedit", 2, ImGuiTableFlags_SizingStretchProp, ImVec2(300.0, 0.0)))
        {
            stringEdit(prop.get<Hkx::PropName>(), "", "name");
            if (ImGui::Button(ICON_FA_TRASH))
            {
                auto name     = prop.get<Hkx::PropName>().text().as_string();
                auto ref_node = file.getFirstEventRef(prop.m_index);
                if (ref_node)
                {
                    spdlog::warn("This property is still referenced by {} and potentially more object.\nID copied to clipboard.", ref_node.attribute("name").as_string());
                    ImGui::SetClipboardText(ref_node.attribute("name").as_string());
                }
                else
                {
                    file.m_evt_manager.delEntry(prop.m_index);
                    spdlog::info("Event {} deleted!", name);
                    ImGui::EndTable();
                    ImGui::CloseCurrentPopup();
                    ImGui::EndPopup();
                    return;
                }
            }
            addTooltip("Delete\nIf it is still referenced, the property won't delete\nand one of its referencing object will be copied to clipboard.");

            auto role_node = prop.get<Hkx::PropVarInfo>().getByName("role").first_child();
            enumEdit(role_node.getByName("role"), Hkx::e_hkbRoleAttribute_Role);
            flagEdit(role_node.getByName("flags"), Hkx::f_hkbRoleAttribute_roleFlags);

            ImGui::EndTable();
        }
        ImGui::EndPopup();
    }
}

} // namespace Ui
} // namespace Haviour