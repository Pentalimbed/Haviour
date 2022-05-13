// Widgets for editing all sorts of params.
// All edits must be used in a table context.
#pragma once
#include "hkx/hkxfile.h"
#include "hkx/hkclass.inl"

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
// get type from imgui datatype enum
template <ImGuiDataType data_type>
constexpr auto imgui_type()
{
    if constexpr (data_type == ImGuiDataType_S8)
        return int8_t();
    else if constexpr (data_type == ImGuiDataType_S16)
        return int16_t();
    else if constexpr (data_type == ImGuiDataType_S32)
        return int32_t();
    else if constexpr (data_type == ImGuiDataType_S64)
        return int64_t();
    else if constexpr (data_type == ImGuiDataType_U8)
        return uint8_t();
    else if constexpr (data_type == ImGuiDataType_U16)
        return uint16_t();
    else if constexpr (data_type == ImGuiDataType_U32)
        return uint32_t();
    else if constexpr (data_type == ImGuiDataType_U64)
        return uint64_t();
    else if constexpr (data_type == ImGuiDataType_Float)
        return float();
}
template <ImGuiDataType data_type>
using imgui_type_t = decltype(imgui_type<data_type>());

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

inline void addTooltipSv(std::string_view text)
{
    if (!text.empty() && ImGui::IsItemHovered())
        ImGui::SetTooltip(text.data());
}

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

///////////////////////// PICKERS

// ListBox with clipping, filter, max size limit and custom item widget function
// filter function returns true if item should remain
// size specifies width & item height
template <typename T>
std::optional<T> filteredPickerListBox(const char*                               label,
                                       std::vector<T>&                           items,
                                       std::function<bool(T&, std::string_view)> filter_func,
                                       std::function<bool(T&)>                   item_func,
                                       ImVec2                                    size     = {400.0f, 21.0f},
                                       size_t                                    max_item = 30)
{
    static std::string filter_text = {};
    ImGui::InputText("Filter", &filter_text);

    std::vector<size_t> items_filtered = {};
    for (auto& item : items)
        if (filter_func(item, filter_text))
            items_filtered.push_back(&item - items.data());

    if (items_filtered.empty())
        ImGui::TextDisabled("No item");
    else if (ImGui::BeginListBox(label, {size.x, size.y * std::min(items.size(), max_item)}))
    {
        ImGuiListClipper clipper;
        clipper.Begin(items_filtered.size());

        while (clipper.Step())
            for (int row_n = clipper.DisplayStart; row_n < clipper.DisplayEnd; row_n++)
            {
                if (item_func(items[items_filtered[row_n]]))
                {
                    ImGui::EndListBox();
                    return items[items_filtered[row_n]];
                }
            }
        ImGui::EndListBox();
    }
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

// template <auto fn, typename... Args>
// typename std::invoke_result_t<decltype(fn), const char*, Args..., bool> pickerButton(const char* str_id, Args... args)
// {
//     ImGui::PushID(str_id);
//     bool open = false;
//     if (ImGui::Button(ICON_FA_SEARCH))
//     {
//         open = true;
//         ImGui::OpenPopup("select prop");
//     }
//     auto res = func("select prop", std::forward<Args>(args)..., open);
//     ImGui::PopID();
//     return res;
// }

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

template <typename Entry>
bool linkedPropSelectable(Entry& prop)
{
    bool res = false;
    if constexpr (std::is_same_v<Entry, Hkx::Variable>)
        ImGui::PushStyleColor(ImGuiCol_Text, getVarColor(prop));
    auto idstr = fmt::format("{}", prop.m_index);
    ImGui::TextUnformatted(idstr.c_str()), ImGui::SameLine();
    ImGui::Dummy({std::max(0.0f, 35.0f - ImGui::CalcTextSize(idstr.c_str()).x), 0}), ImGui::SameLine();
    if (ImGui::Selectable(prop.getName()))
        res = true;
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
template <typename Manager>
std::optional<typename Manager::Entry> linkedPropPickerButton(const char* str_id, Manager& prop_manager)
{
    pickerButton(linkedPropPickerPopup, prop_manager);
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

///////////////////////// OTHER BUTTONS

template <auto& flags>
bool flagEditButton(const char* str_id, uint32_t& value)
{
    ImGui::PushID(str_id);

    bool res = false;
    if (ImGui::Button(ICON_FA_FLAG))
        ImGui::OpenPopup("edit flags");
    addTooltip("Edit individual flags");
    if (ImGui::BeginPopup("edit flags"))
    {
        for (auto& flag : flags)
            if (flag.val)
            {
                if (ImGui::CheckboxFlags(flag.name.data(), &value, flag.val))
                    res = true;
                if (!flag.hint.empty())
                    addTooltip(flag.hint.data());
            }
        ImGui::EndPopup();
    }

    ImGui::PopID();

    return res;
}

void varBindingButton(const char* str_id, pugi::xml_node hkparam, Hkx::BehaviourFile& file);
void varBindingButton(const char* str_id, pugi::xml_node hkparam, Hkx::BehaviourFile* file);

////////////////    hkParam Edits

#define DEF_EDIT_CONSTR(derived, base)     \
    derived() = default;                   \
    inline derived(                        \
        pugi::xml_node   hkparam,          \
        Hkx::HkxFile*    file = nullptr,   \
        std::string_view hint = {},        \
        std::string_view name = {}) :      \
        base(hkparam, file, hint, name) {} \
    inline derived(                        \
        pugi::xml_node   hkparam,          \
        Hkx::HkxFile&    file,             \
        std::string_view hint = {},        \
        std::string_view name = {}) :      \
        derived(hkparam, &file, hint, name) {}

struct ParamEdit
{
    DEF_FACT_MEM(pugi::xml_node, hkparam, {})
    DEF_FACT_MEM(Hkx::HkxFile*, file, nullptr) // for binding
    DEF_FACT_MEM(std::string_view, hint, {})
    DEF_FACT_MEM(std::string_view, name, {})

    virtual inline const char* getName() { return m_name.empty() ? m_hkparam.attribute("name").as_string() : m_name.data(); }
    virtual inline void        show()
    {
        fetchValue();

        ImGui::TableNextColumn();
        auto update = showEdit();
        addTooltipSv(m_hint);
        ImGui::TableNextColumn();
        update |= showButton();

        if (update)
            updateValue();
    }
    virtual bool showEdit() = 0;    // edit internal value, return true if value edited
    virtual bool showButton();      // return true if value edited
    virtual void fetchValue()  = 0; // update internal value from hkparam
    virtual void updateValue() = 0; // update hkparam with new value

    ParamEdit() = default;
    inline ParamEdit(pugi::xml_node hkparam, Hkx::HkxFile* file = nullptr, std::string_view hint = {}, std::string_view name = {}) :
        m_hkparam(hkparam), m_file(file), m_hint(hint), m_name(name){};
    inline ParamEdit(pugi::xml_node hkparam, Hkx::HkxFile& file, std::string_view hint = {}, std::string_view name = {}) :
        ParamEdit(hkparam, &file, hint, name){};

    virtual inline void operator()() { show(); }
};

struct BoolEdit : public ParamEdit
{
    bool         m_value;
    virtual void fetchValue() override;
    virtual void updateValue() override;
    virtual bool showEdit() override;
    DEF_EDIT_CONSTR(BoolEdit, ParamEdit)
};

struct QuadEdit : public ParamEdit
{
    float        m_value[4];
    virtual void fetchValue() override;
    virtual void updateValue() override;
    virtual bool showEdit() override;
    DEF_EDIT_CONSTR(QuadEdit, ParamEdit)
};

// scalar based
template <ImGuiDataType data_type = ImGuiDataType_Float>
struct ScalarEdit : public ParamEdit
{
    imgui_type_t<data_type> m_value;
    virtual void            fetchValue() override
    {
        if constexpr (data_type == ImGuiDataType_S8 || data_type == ImGuiDataType_S16 || data_type == ImGuiDataType_S32 || data_type == ImGuiDataType_S64)
            m_value = m_hkparam.text().as_llong();
        else if constexpr (data_type == ImGuiDataType_U8 || data_type == ImGuiDataType_U16 || data_type == ImGuiDataType_U32 || data_type == ImGuiDataType_U64)
            m_value = m_hkparam.text().as_ullong();
        else if constexpr (data_type == ImGuiDataType_Float)
            m_value = m_hkparam.text().as_float();
    }
    virtual void updateValue() override
    {
        if constexpr (data_type == ImGuiDataType_Float)
            m_hkparam.text() = fmt::format("{:.6f}", m_value).c_str();
        else
            m_hkparam.text() = m_value;
    }
    virtual bool showEdit() override
    {
        constexpr auto format = (data_type == ImGuiDataType_Float) ? "%.6f" : nullptr;
        ImGui::SetNextItemWidth(120);
        return ImGui::InputScalar(getName(), data_type, &m_value, nullptr, nullptr, format);
    }
    DEF_EDIT_CONSTR(ScalarEdit, ParamEdit)
};

template <ImGuiDataType data_type = ImGuiDataType_Float>
struct SliderScalarEdit : public ScalarEdit<data_type>
{
    DEF_FACT_MEM(imgui_type_t<data_type>, lb, 0.0f)
    DEF_FACT_MEM(imgui_type_t<data_type>, ub, 100.0f)
    virtual bool showEdit() override
    {
        constexpr auto format = (data_type == ImGuiDataType_Float) ? "%.6f" : nullptr;
        return ImGui::SliderScalar(ParamEdit::getName(), data_type,
                                   std::addressof(ScalarEdit<data_type>::m_value), // CPP IS STUPID
                                   &m_lb, &m_ub, format);
    }
    DEF_EDIT_CONSTR(SliderScalarEdit, ScalarEdit<data_type>)
};

struct FloatAsIntEdit : public ScalarEdit<ImGuiDataType_Float>
{
    virtual void fetchValue() override;
    virtual void updateValue() override;
    DEF_EDIT_CONSTR(FloatAsIntEdit, ScalarEdit<ImGuiDataType_Float>)
};

template <auto& flags>
struct FlagAsIntEdit : public ScalarEdit<ImGuiDataType_U32>
{
    virtual bool showButton() override
    {
        ParamEdit::showButton();
        ImGui::SameLine();
        return flagEditButton<flags>(getName(), m_value);
    }
    DEF_EDIT_CONSTR(FlagAsIntEdit, ScalarEdit<ImGuiDataType_U32>)
};

template <typename Manager>
struct LinkedPropPickerEdit : public ScalarEdit<ImGuiDataType_S32>
{
    DEF_FACT_MEM(Manager*, manager, nullptr)
    virtual bool showButton() override
    {
        ParamEdit::showButton();
        ImGui::SameLine();
        auto res = linkedPropPickerButton(getName(), *m_manager);
        if (res.has_value())
            m_value = res.value().m_index;
        if (auto prop = m_manager->getEntry(m_value); prop.m_valid)
        {
            ImGui::SameLine();
            ImGui::TextUnformatted(prop.getName());
        }
        return res.has_value();
    }
    DEF_EDIT_CONSTR(LinkedPropPickerEdit, ScalarEdit<ImGuiDataType_S32>)
};
using EvtPickerEdit  = LinkedPropPickerEdit<Hkx::AnimationEventManager>;
using VarPickerEdit  = LinkedPropPickerEdit<Hkx::VariableManager>;
using PropPickerEdit = LinkedPropPickerEdit<Hkx::CharacterPropertyManager>;

struct StateEdit : public ScalarEdit<ImGuiDataType_S32>
{
    DEF_FACT_MEM(pugi::xml_node, state_machine, {})
    virtual bool showButton() override;
    DEF_EDIT_CONSTR(StateEdit, ScalarEdit<ImGuiDataType_S32>)
};

struct BoneEdit : public ScalarEdit<ImGuiDataType_S16>
{
    virtual bool showButton() override;
    DEF_EDIT_CONSTR(BoneEdit, ScalarEdit<ImGuiDataType_S16>)
};

// string based
struct StringEdit : public ParamEdit
{
    std::string  m_value;
    virtual void fetchValue() override;
    virtual void updateValue() override;
    virtual bool showEdit() override;
    DEF_EDIT_CONSTR(StringEdit, ParamEdit)
};

struct AnimEdit : public StringEdit
{
    virtual bool showButton() override;
    DEF_EDIT_CONSTR(AnimEdit, StringEdit)
};

template <auto& enums> // should've add a requires to check for arrays but alas
struct EnumEdit : public StringEdit
{
    virtual bool showEdit() override
    {
        if (ImGui::BeginCombo(getName(), m_value.data()))
        {
            for (auto& enum_wrapper : enums)
            {
                const bool is_selected = m_value == enum_wrapper.name;
                if (ImGui::Selectable(enum_wrapper.name.data(), is_selected))
                {
                    ImGui::EndCombo();
                    m_value = enum_wrapper.name;
                    return true;
                }
                if (!enum_wrapper.hint.empty())
                    addTooltip(enum_wrapper.hint.data());
            }
            ImGui::EndCombo();
        }
        return false;
    }
    DEF_EDIT_CONSTR(EnumEdit, StringEdit)
};

template <auto& flags>
struct FlagEdit : public StringEdit
{
    virtual bool showButton() override
    {
        ParamEdit::showButton();
        ImGui::SameLine();
        auto int_value = str2FlagVal(m_hkparam.text().as_string(), flags);
        auto res       = flagEditButton<flags>(getName(), int_value);
        if (res)
            m_value = flagVal2Str(int_value, flags);
        return res;
    }
    DEF_EDIT_CONSTR(FlagEdit, StringEdit)
};

void setPropObj(std::string_view value); // for removing PropEdit dependency in header
template <auto& classes>                 // std::array<std::string_view>
struct RefEdit : public StringEdit
{
    virtual bool showEdit() override
    {
        ImGui::SetNextItemWidth(60);
        auto res = StringEdit::showEdit();
        if (res)
        {
            auto obj = m_file->getObj(m_value);
            if (!obj || std::ranges::find(classes, std::string_view(obj.attribute("class").as_string())) == classes.end())
                m_value = "null";
        }
        return res;
    }
    virtual bool showButton() override
    {
        ParamEdit::showButton();
        ImGui::PushID(getName());
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_ARROW_CIRCLE_RIGHT))
            setPropObj(m_value);
        addTooltip("Go to");
        ImGui::SameLine();
        bool edited = false;
        if (ImGui::Button(ICON_FA_PLUS_CIRCLE))
            ImGui::OpenPopup("Append");
        addTooltip("Append");
        if (ImGui::BeginPopup("Append"))
        {
            auto& class_info = Hkx::getClassInfo();
            for (auto class_name : classes)
            {
                if (!Hkx::getClassDefaultMap().contains(class_name))
                    ImGui::BeginDisabled();
                if (ImGui::Selectable(class_name.data()))
                {
                    auto res = m_file->addObj(class_name);
                    if (!m_value.empty())
                    {
                        m_value = res;
                        edited  = true;
                    }
                    ImGui::CloseCurrentPopup();
                    break;
                }
                if (!Hkx::getClassDefaultMap().contains(class_name))
                    ImGui::EndDisabled();
                if (class_info.contains(class_name))
                    addTooltip(class_info.at(class_name).data())
            }

            ImGui::EndPopup();
        }
        ImGui::PopID();
        if (auto obj = m_file->getObj(m_value); obj)
        {
            ImGui::SameLine();
            ImGui::TextUnformatted(getObjContextName(obj));
        }
        return edited;
    }
    virtual void updateValue() override
    {
        auto old_value  = m_hkparam.text().as_string();
        auto parent_obj = getParentObj(m_hkparam);
        assert(parent_obj);
        auto parent_id = parent_obj.attribute("name").as_string();

        StringEdit::updateValue();

        if (!isRefBy(old_value, parent_obj))
            m_file->deRef(old_value, parent_id);
        m_file->addRef(m_value, parent_id);
    }
    DEF_EDIT_CONSTR(RefEdit, StringEdit)
};

////////////////    HKPARARM EDITS

// I kinda don't wanna refactor them so leave it be

// return true if delete is pressed
bool refEdit(std::string&                         value,
             const std::vector<std::string_view>& classes,
             pugi::xml_node                       parent,
             Hkx::HkxFile&                        file,
             std::string_view                     hint,
             std::string_view                     manual_name);

void refList(pugi::xml_node                       hkparam,
             const std::vector<std::string_view>& classes,
             Hkx::HkxFile&                        file,
             std::string_view                     hint_attribute = {},
             std::string_view                     name_attribute = {},
             std::string_view                     manual_name    = {});

pugi::xml_node refLiveEditList(
    pugi::xml_node                       hkparam,
    const std::vector<std::string_view>& classes,
    Hkx::HkxFile&                        file,
    std::string_view                     hint_attribute = {},
    std::string_view                     name_attribute = {},
    std::string_view                     manual_name    = {});

void objLiveEditList(
    pugi::xml_node                             hkparam,
    pugi::xml_node&                            edit_item,
    const char*                                def_str,
    std::string_view                           str_id,
    std::function<std::string(pugi::xml_node)> disp_name_func);

////////////////    Linked Prop Edits

void varEditPopup(const char*                           str_id,
                  Hkx::Variable&                        var,
                  Hkx::VariableManager&                 manager,
                  std::function<pugi::xml_node(size_t)> get_ref_fn,
                  Hkx::HkxFile&                         file);
void evtEditPopup(const char* str_id, Hkx::AnimationEvent& evt, Hkx::BehaviourFile& file);
void propEditPopup(const char* str_id, Hkx::CharacterProperty& evt, Hkx::BehaviourFile& file);

} // namespace Ui
} // namespace Haviour