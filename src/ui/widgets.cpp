#include "widgets.h"
#include "hkx/hkclass.inl"

#include <imgui.h>
#include <spdlog/spdlog.h>
#include <extern/imgui_stdlib.h>
#include <extern/font_awesome_5.h>

namespace Haviour
{
namespace Ui
{
#define addTooltip(...) \
    if (ImGui::IsItemHovered()) ImGui::SetTooltip(__VA_ARGS__);

bool variablePickerPopup(const char* str_id, Hkx::VariableManager& var_manager, Hkx::Variable& out)
{
    static std::vector<Hkx::Variable> var_list_cache;
    static bool                       need_update = true;
    static std::string                search_text;

    if (need_update)
    {
        need_update    = false;
        search_text    = {};
        var_list_cache = var_manager.getEntryList();
    }

    if (ImGui::BeginPopup(str_id))
    {
        ImGui::InputText("Filter", &search_text);
        ImGui::Separator();

        std::vector<Hkx::Variable> var_filtered;
        var_filtered.reserve(var_list_cache.size());
        std::ranges::copy_if(var_list_cache, std::back_inserter(var_filtered),
                             [](auto& var) {
                                 auto var_disp_name = fmt::format("{:3} {}", var.m_index, var.get<Hkx::PropName>().text().as_string());
                                 return var.m_valid &&
                                     (search_text.empty() ||
                                      !std::ranges::search(var_disp_name, search_text, [](char ch1, char ch2) { return std::toupper(ch1) == std::toupper(ch2); }).empty());
                             });

        ImGuiListClipper clipper;
        clipper.Begin(var_filtered.size());
        while (clipper.Step())
            for (int row_n = clipper.DisplayStart; row_n < clipper.DisplayEnd; row_n++)
            {
                auto& var = var_filtered[row_n];

                if (var.m_valid)
                {
                    auto var_disp_name = fmt::format("{:3} {}", var.m_index, var.get<Hkx::PropName>().text().as_string());
                    if (ImGui::Selectable(var_disp_name.c_str()))
                    {
                        ImGui::CloseCurrentPopup();
                        out = var;
                        ImGui::EndPopup();
                        return true;
                    }
                }
            }

        ImGui::EndPopup();
    }
    else
        need_update = true;

    return false;
}

template <size_t N>
void flagPopup(const char* str_id, pugi::xml_node hkparam, const std::array<EnumWrapper, N>& flags)
{
    std::string value_text = hkparam.text().as_string();
    uint32_t    value      = 0;
    if (ImGui::BeginPopup(str_id))
    {
        bool value_changed = false;
        for (EnumWrapper flag : flags)
        {
            if (flag.val && value_text.contains(flag.name))
                value |= flag.val;
        }
        for (EnumWrapper flag : flags)
            if (flag.val)
            {
                if (ImGui::CheckboxFlags(flag.name.data(), &value, flag.val))
                    value_changed = true;
                if (!flag.hint.empty())
                    addTooltip(flag.hint.data());
            }
        if (value_changed)
        {
            value_text = {};
            if (!value) value_text = "0";
            else
            {
                for (EnumWrapper flag : flags)
                    if (value & flag.val)
                        value_text.append(fmt::format("{}|", flag.name));
                if (value_text.ends_with('|'))
                    value_text.pop_back();
            }
            hkparam.text() = value_text.c_str();
        }
        ImGui::EndPopup();
    }
}
template <size_t N>
void flagEditButton(const char* str_id, pugi::xml_node hkparam, const std::array<EnumWrapper, N>& flags)
{
    flagPopup(str_id, hkparam, flags);
    if (ImGui::Button(ICON_FA_FLAG))
        ImGui::OpenPopup(str_id);
    addTooltip("Edit individual flags");
}

////////////////    EDITS

void stringEdit(pugi::xml_node hkparam, std::string_view manual_name)
{
    ImGui::TableNextColumn();
    std::string value = hkparam.text().as_string();
    if (ImGui::InputText(manual_name.empty() ? hkparam.attribute("name").as_string() : manual_name.data(), &value))
        hkparam.text() = value.c_str();

    ImGui::TableNextColumn();
}

void boolEdit(pugi::xml_node hkparam, std::string_view manual_name)
{
    ImGui::TableNextColumn();
    bool value = hkparam.text().as_bool();
    if (ImGui::Checkbox(manual_name.empty() ? hkparam.attribute("name").as_string() : manual_name.data(), &value))
        hkparam.text() = value;

    ImGui::TableNextColumn();
}

template <size_t N>
void enumEdit(pugi::xml_node hkparam, const std::array<EnumWrapper, N>& enums, std::string_view manual_name)
{
    ImGui::TableNextColumn();
    std::string value = hkparam.text().as_string();
    if (ImGui::BeginCombo(manual_name.empty() ? hkparam.attribute("name").as_string() : manual_name.data(), value.c_str()))
    {
        for (EnumWrapper enum_wrapper : enums)
        {
            const bool is_selected = value == enum_wrapper.name;
            if (ImGui::Selectable(enum_wrapper.name.data(), is_selected))
                hkparam.text().set(enum_wrapper.name.data());
            if (!enum_wrapper.hint.empty())
                addTooltip(enum_wrapper.hint.data());
            if (is_selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    ImGui::TableNextColumn();
}

void intEdit(pugi::xml_node hkparam, std::string_view manual_name)
{
    ImGui::TableNextColumn();
    auto value = hkparam.text().as_int();
    if (ImGui::InputInt(manual_name.empty() ? hkparam.attribute("name").as_string() : manual_name.data(), &value))
        hkparam.text() = value;

    ImGui::TableNextColumn();
}

void sliderIntEdit(pugi::xml_node hkparam, int lb, int ub, std::string_view manual_name)
{
    ImGui::TableNextColumn();
    int value = hkparam.text().as_int();
    if (ImGui::SliderInt(manual_name.empty() ? hkparam.attribute("name").as_string() : manual_name.data(), &value, lb, ub, "%.6f"))
        hkparam.text() = value;

    ImGui::TableNextColumn();
}

template <size_t N>
void flagEdit(pugi::xml_node hkparam, const std::array<EnumWrapper, N>& flags, std::string_view manual_name)
{
    ImGui::PushID(manual_name.empty() ? hkparam.attribute("name").as_string() : manual_name.data());

    stringEdit(hkparam, manual_name.empty() ? hkparam.attribute("name").as_string() : manual_name.data());
    flagEditButton("Edit flags", hkparam, flags);

    ImGui::PopID();
}

void variablePickerEdit(pugi::xml_node hkparam, Hkx::VariableManager& var_manager, std::string_view manual_name)
{
    ImGui::PushID(manual_name.empty() ? hkparam.attribute("name").as_string() : manual_name.data());

    intEdit(hkparam, manual_name);

    Hkx::Variable out_var;
    if (variablePickerPopup("Pickvar", var_manager, out_var))
        hkparam.text() = out_var.m_index;
    if (ImGui::Button(ICON_FA_SEARCH))
        ImGui::OpenPopup("Pickvar");
    addTooltip("Select variable");

    ImGui::PopID();
}

void sliderFloatEdit(pugi::xml_node hkparam, float lb, float ub, std::string_view manual_name)
{
    ImGui::TableNextColumn();
    float value = hkparam.text().as_float();
    if (ImGui::SliderFloat(manual_name.empty() ? hkparam.attribute("name").as_string() : manual_name.data(), &value, lb, ub, "%.6f"))
        hkparam.text() = fmt::format("{:.6f}", value).c_str();

    ImGui::TableNextColumn();
}

void floatEdit(pugi::xml_node hkparam, std::string_view manual_name)
{
    ImGui::TableNextColumn();
    float value = hkparam.text().as_float();
    if (ImGui::InputFloat(manual_name.empty() ? hkparam.attribute("name").as_string() : manual_name.data(), &value, 0.0f, 0.0f, "%.6f"))
        hkparam.text() = fmt::format("{:.6f}", value).c_str();

    ImGui::TableNextColumn();
}

// for floats written in UINT32 values
void convertFloatEdit(pugi::xml_node hkparam, std::string_view manual_name)
{
    ImGui::TableNextColumn();
    auto  raw_value = hkparam.text().as_uint();
    float value     = *reinterpret_cast<float*>(&raw_value);
    if (ImGui::InputFloat(manual_name.empty() ? hkparam.attribute("name").as_string() : manual_name.data(), &value, 0.0f, 0.0f))
        hkparam.text() = *reinterpret_cast<uint32_t*>(&value);

    ImGui::TableNextColumn();
}

void quadEdit(pugi::xml_node hkparam, std::string_view manual_name)
{
    ImGui::TableNextColumn();

    float       value[4];
    std::string text(hkparam.text().as_string());
    std::erase_if(text, [](char& c) { return (c == '(') || (c == ')'); });
    std::istringstream text_stream(text);
    text_stream >> value[0] >> value[1] >> value[2] >> value[3];

    if (ImGui::InputFloat4(manual_name.empty() ? hkparam.attribute("name").as_string() : manual_name.data(), value, "%.6f"))
        hkparam.text() = fmt::format("({:.6f} {:.6f} {:.6f} {:.6f})", value[0], value[1], value[2], value[3]).c_str();

    ImGui::TableNextColumn();
}

void refEdit(pugi::xml_node hkparam, const std::vector<std::string_view>& classes, Hkx::HkxFile& file, std::string_view manual_name)
{
    ImGui::TableNextColumn();

    ImGui::PushID(manual_name.empty() ? hkparam.attribute("name").as_string() : manual_name.data());

    bool        edited    = false;
    std::string old_value = hkparam.text().as_string();
    auto        value     = old_value;

    if (ImGui::InputText(manual_name.empty() ? hkparam.attribute("name").as_string() : manual_name.data(), &value))
    {
        edited = true;
        if (auto obj = file.getNode(value))                                                                        // Is obj
            if (std::ranges::find(classes, std::string_view(obj.attribute("class").as_string())) != classes.end()) // Same class
                hkparam.text() = value.c_str();
            else
                hkparam.text() = "null";
        else
            hkparam.text() = "null";
    }

    ImGui::TableNextColumn();
    // copyButton(hkparam.text().as_string());

    // ImGui::SameLine();
    if (ImGui::Button(ICON_FA_ARROW_CIRCLE_RIGHT)) {}
    // PropEdit::getSingleton()->setObject(value);
    addTooltip("Go to");

    ImGui::SameLine();
    if (ImGui::BeginPopup("Append"))
    {
        for (auto class_name : classes)
            if (ImGui::Selectable(class_name.data()))
            {
                // TODO add item
                ImGui::CloseCurrentPopup();
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

////////////////    Linked Prop Edits

void varEditPopup(const char* str_id, Hkx::Variable& var, Hkx::HkxFile& file)
{
    if (ImGui::BeginPopup(str_id))
    {
        if (ImGui::BeginTable("varedit", 2, ImGuiTableFlags_SizingStretchProp, ImVec2(600, -FLT_MIN)))
        {
            stringEdit(var.get<Hkx::PropName>(), "name");
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

            auto var_type_enum  = Hkx::getVarTypeEnum(var.get<Hkx::PropVarInfo>().find_child_by_attribute("name", "type").text().as_string());
            auto var_value_node = var.get<Hkx::PropWordValue>().first_child();
            switch (var_type_enum)
            {
                case Hkx::VARIABLE_TYPE_BOOL:
                    boolEdit(var_value_node);
                    break;
                case Hkx::VARIABLE_TYPE_INT8:
                    sliderIntEdit(var_value_node, INT8_MIN, INT8_MAX);
                    break;
                case Hkx::VARIABLE_TYPE_INT16:
                    sliderIntEdit(var_value_node, INT16_MIN, INT16_MAX);
                    break;
                case Hkx::VARIABLE_TYPE_INT32:
                    intEdit(var_value_node);
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

            auto role_node = var.get<Hkx::PropVarInfo>().find_child_by_attribute("name", "role").first_child();
            enumEdit(role_node.find_child_by_attribute("name", "role"), Hkx::e_hkbRoleAttribute_Role);
            flagEdit(role_node.find_child_by_attribute("name", "flags"), Hkx::f_hkbRoleAttribute_roleFlags);

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
            stringEdit(evt.get<Hkx::PropName>(), "name");
            if (ImGui::Button(ICON_FA_TRASH))
            {
                auto& file     = Hkx::HkxFileManager::getSingleton()->getCurrentFile();
                auto  name     = evt.get<Hkx::PropName>().text().as_string();
                auto  ref_node = file.getFirstEventRef(evt.m_index);
                if (ref_node)
                {
                    spdlog::warn("This event is still referenced by {} and potentially more object.\nID copied to clipboard.", ref_node.attribute("name").as_string());
                    ImGui::SetClipboardText(ref_node.attribute("name").as_string());
                }
                else
                {
                    file.m_evt_manager.delEntry(evt.m_index);
                    spdlog::info("Event {} deleted!", name);
                    ImGui::CloseCurrentPopup();
                    ImGui::EndTable();
                    return;
                }
            }
            addTooltip("Delete\nIf it is still referenced, the event won't delete\nand one of its referencing object will be copied to clipboard.");

            flagEdit(evt.get<Hkx::PropEventInfo>().find_child_by_attribute("name", "flags"), Hkx::f_hkbEventInfo_Flags);

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
            stringEdit(prop.get<Hkx::PropName>(), "name");
            if (ImGui::Button(ICON_FA_TRASH))
            {
                auto& file     = Hkx::HkxFileManager::getSingleton()->getCurrentFile();
                auto  name     = prop.get<Hkx::PropName>().text().as_string();
                auto  ref_node = file.getFirstEventRef(prop.m_index);
                if (ref_node)
                {
                    spdlog::warn("This property is still referenced by {} and potentially more object.\nID copied to clipboard.", ref_node.attribute("name").as_string());
                    ImGui::SetClipboardText(ref_node.attribute("name").as_string());
                }
                else
                {
                    file.m_evt_manager.delEntry(prop.m_index);
                    spdlog::info("Event {} deleted!", name);
                    ImGui::CloseCurrentPopup();
                    ImGui::EndTable();
                    return;
                }
            }
            addTooltip("Delete\nIf it is still referenced, the property won't delete\nand one of its referencing object will be copied to clipboard.");

            auto role_node = prop.get<Hkx::PropVarInfo>().find_child_by_attribute("name", "role").first_child();
            enumEdit(role_node.find_child_by_attribute("name", "role"), Hkx::e_hkbRoleAttribute_Role);
            flagEdit(role_node.find_child_by_attribute("name", "flags"), Hkx::f_hkbRoleAttribute_roleFlags);

            ImGui::EndTable();
        }
        ImGui::EndPopup();
    }
}

} // namespace Ui
} // namespace Haviour