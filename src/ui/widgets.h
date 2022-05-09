// Widgets for editing all sorts of params.
// All edits must be used in a table context.
#pragma once
#include "hkx/hkxfile.h"

#include <type_traits>

#include <fmt/format.h>
#include <pugixml.hpp>
#include <imgui.h>
#include <extern/font_awesome_5.h>
#include <extern/imgui_stdlib.h>

namespace Haviour
{

namespace Hkx
{
VariableTypeEnum getVarTypeEnum(std::string_view enumstr);
}

namespace Ui
{
const auto g_color_invalid = ImColor(0.2f, 0.2f, 0.2f).Value;
const auto g_color_bool    = ImColor(0xFF, 0x9C, 0x83).Value;
const auto g_color_int     = ImColor(0x07, 0xd2, 0xd9).Value;
const auto g_color_float   = ImColor(0xF6, 0x6f, 0x9a).Value;
const auto g_color_attr    = ImColor(0x9d, 0x00, 0x1c).Value;
const auto g_color_quad    = ImColor(0x5a, 0xe6, 0xb8).Value;

inline ImVec4 getVarColor(Hkx::Variable& var)
{
    auto var_type_enum = Hkx::getVarTypeEnum(var.get<Hkx::PropVarInfo>().getByName("type").text().as_string());
    if (var_type_enum < 0)
        return g_color_invalid;
    else if (var_type_enum < 1)
        return g_color_bool;
    else if (var_type_enum < 4)
        return g_color_int;
    else if (var_type_enum < 5)
        return g_color_float;
    else if (var_type_enum < 6)
        return g_color_attr;
    else
        return g_color_quad;
}

#define copyableText(text, ...)                \
    ImGui::TextUnformatted(text, __VA_ARGS__); \
    addTooltip("Left click to copy text");     \
    if (ImGui::IsItemClicked())                \
        ImGui::SetClipboardText(text);

#define addTooltip(...) \
    if (ImGui::IsItemHovered()) ImGui::SetTooltip(__VA_ARGS__);

// Listbox with clipping and custom item widget function
// This function returns selected index if selected
// item function should return true if selected
template <typename T>
std::optional<int> pickerListBox(const char* label, ImVec2 size, std::vector<T>& items, std::function<bool(T&)> item_func)
{
    if (ImGui::BeginListBox(label, size))
    {
        ImGuiListClipper clipper;
        clipper.Begin(items.size());

        while (clipper.Step())
            for (int row_n = clipper.DisplayStart; row_n < clipper.DisplayEnd; row_n++)
            {
                if (item_func(items[row_n]))
                {
                    ImGui::EndListBox();
                    return row_n;
                }
            }
        ImGui::EndListBox();
    }
    return std::nullopt;
}

// pickerListBox + filter
// filter function returns true if item should remain
template <typename T>
std::optional<T> filteredPickerListBox(const char*                               label,
                                       std::vector<T>&                           items,
                                       std::function<bool(T&, std::string_view)> filter_func,
                                       std::function<bool(T&)>                   item_func)
{
    static std::string filter_text = {};
    ImGui::InputText("Filter", &filter_text);

    std::vector<T> items_filtered = {};
    for (auto& item : items)
        if (filter_func(item, filter_text))
            items_filtered.push_back(item);

    if (items_filtered.empty())
        ImGui::TextDisabled("No item");
    else if (auto res = pickerListBox(label, {400.0f, 21.0f * std::min(items_filtered.size(), 30ull)}, items_filtered, item_func); res.has_value())
        return items[res.value()];
    return std::nullopt;
}

template <typename T>
std::optional<T> pickerPopup(const char*                               str_id,
                             std::vector<T>&                           items,
                             std::function<bool(T&, std::string_view)> filter_func,
                             std::function<bool(T&)>                   item_func,
                             bool                                      just_open)
{
    if (ImGui::BeginPopup(str_id, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar))
    {
        if (just_open)
            ImGui::SetKeyboardFocusHere(); // set focus to keyboard
        auto res = filteredPickerListBox(fmt::format("##{}", str_id).c_str(), items, filter_func, item_func);
        if (res.has_value())
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
        return res;
    }
    return std::nullopt;
}

template <typename Entry>
bool linkedPropSelectable(Entry& prop)
{
    if constexpr (std::is_same_v<Entry, Hkx::Variable>)
        ImGui::PushStyleColor(ImGuiCol_Text, getVarColor(prop));
    if (ImGui::Selectable(prop.getItemName().c_str()))
    {
        if constexpr (std::is_same_v<Entry, Hkx::Variable>)
            ImGui::PopStyleColor();
        return true;
    }
    if constexpr (std::is_same_v<Entry, Hkx::Variable>)
        ImGui::PopStyleColor();
    return false;
};

template <typename Manager>
std::optional<typename Manager::Entry> linkedPropPickerPopup(const char* str_id, Manager& prop_manager, bool just_open)
{
    using Entry = typename Manager::Entry;

    auto entries = prop_manager.getEntryList();
    return pickerPopup<Entry>(
        str_id, entries,
        [](Entry& prop, std::string_view filter_str) { return hasText(prop.getItemName(), filter_str); },
        linkedPropSelectable<Entry>,
        just_open);
}

// very ugly, but templates just don't fucking works
#define pickerButton(func, ...)                        \
    ImGui::PushID(str_id);                             \
    bool open = false;                                 \
    if (ImGui::Button(ICON_FA_SEARCH))                 \
    {                                                  \
        open = true;                                   \
        ImGui::OpenPopup("select prop");               \
    }                                                  \
    auto res = func("select prop", __VA_ARGS__, open); \
    ImGui::PopID();                                    \
    return res;

template <typename Manager>
std::optional<typename Manager::Entry> linkedPropPickerButton(const char* str_id, Manager& prop_manager)
{
    pickerButton(linkedPropPickerPopup, prop_manager);
}

template <bool as_int, size_t N>
void flagEditButton(const char* str_id, pugi::xml_node hkparam, const std::array<EnumWrapper, N>& flags)
{
    ImGui::PushID(str_id);

    if (ImGui::Button(ICON_FA_FLAG))
        ImGui::OpenPopup("edit flags");

    if (ImGui::BeginPopup("edit flags"))
    {
        uint32_t value;

        if constexpr (as_int)
            value = hkparam.text().as_uint();
        else
            value = str2FlagVal(hkparam.text().as_string(), flags);

        for (auto& flag : flags)
            if (flag.val)
            {
                if (ImGui::CheckboxFlags(flag.name.data(), &value, flag.val))
                {
                    if constexpr (as_int)
                        hkparam.text() = value;
                    else
                        hkparam.text() = flagVal2Str(value, flags).c_str();
                }
                if (!flag.hint.empty())
                    addTooltip(flag.hint.data());
            }
        ImGui::EndPopup();
    }

    ImGui::PopID();
}

std::optional<pugi::xml_node>        statePickerPopup(const char* str_id, pugi::xml_node state_machine, Hkx::HkxFile& file, int selected_state_id, bool just_open);
inline std::optional<pugi::xml_node> statePickerButton(const char* str_id, pugi::xml_node state_machine, Hkx::HkxFile& file, int selected_state_id)
{
    pickerButton(statePickerPopup, state_machine, file, selected_state_id);
}

std::optional<int16_t>        bonePickerPopup(const char* str_id, Hkx::SkeletonFile& skel_file, int16_t selected_bone_id, bool just_open);
inline std::optional<int16_t> bonePickerButton(const char* str_id, Hkx::SkeletonFile& skel_file, int16_t selected_bone_id)
{
    pickerButton(bonePickerPopup, skel_file, selected_bone_id);
}

std::optional<std::string_view>        animPickerPopup(const char* str_id, Hkx::CharacterFile& char_file, std::string_view selected_anim, bool just_open);
inline std::optional<std::string_view> animPickerButton(const char* str_id, Hkx::CharacterFile& char_file, std::string_view selected_anim)
{
    pickerButton(animPickerPopup, char_file, selected_anim);
}
std::optional<std::string_view> animPickerButton(Hkx::CharacterFile& char_file, std::string_view value);

void varBindingButton(const char* str_id, pugi::xml_node hkparam, Hkx::BehaviourFile& file);

////////////////    EDITS
inline void fullTableSeparator()
{
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    while (ImGui::TableGetColumnIndex() < ImGui::TableGetColumnCount() - 1)
    {
        ImGui::Separator();
        ImGui::TableNextColumn();
    }
    ImGui::Separator();
}

void stringEdit(pugi::xml_node   hkparam,
                std::string_view hint        = {},
                std::string_view manual_name = {});

void animEdit(pugi::xml_node      hkparam,
              Hkx::CharacterFile& char_file,
              std::string_view    hint        = {},
              std::string_view    manual_name = {});

void boolEdit(pugi::xml_node      hkparam,
              Hkx::BehaviourFile& file,
              std::string_view    hint        = {},
              std::string_view    manual_name = {});

template <size_t N>
void enumEdit(pugi::xml_node hkparam, const std::array<EnumWrapper, N>& enums, std::string_view hint = {}, std::string_view manual_name = {})
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
    else if (!hint.empty())
        addTooltip(hint.data());

    ImGui::TableNextColumn();
}

void intScalarEdit(pugi::xml_node hkparam, Hkx::BehaviourFile& file, ImGuiDataType data_type = ImGuiDataType_S32, std::string_view hint = {}, std::string_view manual_name = {});

// void sliderIntEdit(pugi::xml_node   hkparam,
//                    int              lb,
//                    int              ub,
//                    std::string_view hint        = {},
//                    std::string_view manual_name = {});

template <bool as_int, size_t N>
void flagEdit(pugi::xml_node hkparam, const std::array<EnumWrapper, N>& flags, std::string_view hint = {}, std::string_view manual_name = {})
{
    stringEdit(hkparam, hint, manual_name.empty() ? hkparam.attribute("name").as_string() : manual_name.data());
    flagEditButton<as_int>("Edit flags", hkparam, flags);
}

template <size_t N>
void flagEdit(pugi::xml_node hkparam, const std::array<EnumWrapper, N>& flags, std::string_view hint = {}, std::string_view manual_name = {})
{
    flagEdit<false>(hkparam, flags, hint, manual_name);
}

template <typename Manager>
void linkedPropPickerEdit(pugi::xml_node      hkparam,
                          Hkx::BehaviourFile& file,
                          Manager&            prop_manager,
                          std::string_view    hint        = {},
                          std::string_view    manual_name = {})
{
    intScalarEdit(hkparam, file, ImGuiDataType_S32, hint, manual_name);
    if (getParentObj(hkparam).getByName("variableBindingSet"))
        ImGui::SameLine();
    auto res = linkedPropPickerButton(manual_name.empty() ? hkparam.attribute("name").as_string() : manual_name.data(), prop_manager);
    if (res.has_value())
        hkparam.text() = res.value().m_index;
}

void sliderFloatEdit(pugi::xml_node      hkparam,
                     float               lb,
                     float               ub,
                     Hkx::BehaviourFile& file,
                     std::string_view    hint        = {},
                     std::string_view    manual_name = {});

void floatEdit(pugi::xml_node      hkparam,
               Hkx::BehaviourFile& file,
               std::string_view    hint        = {},
               std::string_view    manual_name = {});

// for floats written in UINT32 values
void convertFloatEdit(pugi::xml_node   hkparam,
                      std::string_view hint        = {},
                      std::string_view manual_name = {});

void quadEdit(pugi::xml_node      hkparam,
              Hkx::BehaviourFile& file,
              std::string_view    hint        = {},
              std::string_view    manual_name = {});

void refEdit(pugi::xml_node                       hkparam,
             const std::vector<std::string_view>& classes, // should've used array but i don't wanna stuff too much templates here
             Hkx::BehaviourFile&                  file,
             std::string_view                     hint        = {},
             std::string_view                     manual_name = {});

// return true if delete is pressed
bool refEdit(std::string&                         value,
             const std::vector<std::string_view>& classes,
             pugi::xml_node                       parent,
             Hkx::BehaviourFile&                  file,
             std::string_view                     hint,
             std::string_view                     manual_name);

void refList(pugi::xml_node                       hkparam,
             const std::vector<std::string_view>& classes,
             Hkx::BehaviourFile&                  file,
             std::string_view                     hint_attribute = {},
             std::string_view                     name_attribute = {},
             std::string_view                     manual_name    = {});

pugi::xml_node refLiveEditList(
    pugi::xml_node                       hkparam,
    const std::vector<std::string_view>& classes,
    Hkx::BehaviourFile&                  file,
    std::string_view                     hint_attribute = {},
    std::string_view                     name_attribute = {},
    std::string_view                     manual_name    = {});

void objLiveEditList(
    pugi::xml_node                             hkparam,
    pugi::xml_node&                            edit_item,
    const char*                                def_str,
    Hkx::BehaviourFile&                        file,
    std::string_view                           str_id,
    std::function<std::string(pugi::xml_node)> disp_name_func);

void stateEdit(pugi::xml_node      hkparam,
               Hkx::BehaviourFile& file,
               pugi::xml_node      state_machine = {},
               std::string_view    hint          = {},
               std::string_view    manual_name   = {});

void boneEdit(pugi::xml_node      hkparam,
              Hkx::BehaviourFile& file,
              Hkx::SkeletonFile&  skel_file,
              std::string_view    hint        = {},
              std::string_view    manual_name = {});

////////////////    Linked Prop Edits

void varEditPopup(const char* str_id, Hkx::Variable& var, Hkx::BehaviourFile& file);
void evtEditPopup(const char* str_id, Hkx::AnimationEvent& evt, Hkx::BehaviourFile& file);
void propEditPopup(const char* str_id, Hkx::CharacterProperty& evt, Hkx::BehaviourFile& file);

} // namespace Ui
} // namespace Haviour