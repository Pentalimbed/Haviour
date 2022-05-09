#include "widgets.h"
#include "hkx/hkclass.inl"
#include "propedit.h"
#include "hkx/hkutils.h"

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

void statePickerPopup(const char* str_id, pugi::xml_node hkparam, pugi::xml_node state_machine, Hkx::BehaviourFile& file)
{
    if (ImGui::BeginPopup(str_id, ImGuiWindowFlags_AlwaysAutoResize))
    {
        auto               states_node = state_machine.getByName("states");
        size_t             num_objs    = states_node.attribute("numelements").as_ullong();
        std::istringstream states_stream(states_node.text().as_string());

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

std::optional<int16_t> bonePickerButton(Hkx::SkeletonFile& skel_file, int16_t value)
{
    bool set_focus = false;
    if (ImGui::Button(ICON_FA_SEARCH))
    {
        ImGui::OpenPopup("select bone");
        set_focus = true;
    }
    addTooltip("Select");

    static std::string search_text      = {};
    static bool        use_ragdoll_skel = false;

    if (ImGui::BeginPopup("select bone", ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar))
    {
        if (skel_file.isFileLoaded())
        {
            ImGui::Checkbox("Ragdoll Skeleton", &use_ragdoll_skel);
            if (set_focus)
                ImGui::SetKeyboardFocusHere();
            ImGui::InputText("Filter", &search_text);

            ImGui::Separator();

            std::vector<std::string_view> bone_list;
            skel_file.getBoneList(bone_list, use_ragdoll_skel);

            if (bone_list.empty())
                ImGui::TextDisabled("No bones.");
            else
            {
                std::erase_if(bone_list,
                              [=](std::string_view bone) {
                                  return !search_text.empty() && std::ranges::search(bone, search_text, [](char ch1, char ch2) { return std::toupper(ch1) == std::toupper(ch2); }).empty();
                              });

                ImGui::BeginChild("ITEMS", {400.0f, 20.0f * std::min(bone_list.size(), 30ull)}, false);
                {
                    ImGuiListClipper clipper;
                    clipper.Begin(bone_list.size());

                    while (clipper.Step())
                        for (int row_n = clipper.DisplayStart; row_n < clipper.DisplayEnd; row_n++)
                        {
                            auto bone           = bone_list[row_n];
                            auto bone_disp_name = fmt::format("{:4} {}", row_n, bone);

                            if (ImGui::Selectable(bone_disp_name.c_str(), value == row_n))
                            {
                                ImGui::CloseCurrentPopup();
                                ImGui::EndChild();
                                ImGui::EndPopup();
                                return row_n;
                            }
                        }
                }
                ImGui::EndChild();
            }
        }
        else
            ImGui::TextDisabled("No loaded skeleton file.");

        ImGui::EndPopup();
    }
    return std::nullopt;
}

std::optional<std::string_view> animPickerButton(Hkx::CharacterFile& char_file, std::string_view value)
{
    bool set_focus = false;
    if (ImGui::Button(ICON_FA_SEARCH))
    {
        ImGui::OpenPopup("select anim");
        set_focus = true;
    }
    addTooltip("Select");

    static std::string search_text      = {};
    static bool        use_ragdoll_skel = false;

    if (ImGui::BeginPopup("select anim", ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar))
    {
        if (char_file.isFileLoaded())
        {
            if (set_focus)
                ImGui::SetKeyboardFocusHere();
            ImGui::InputText("Filter", &search_text);

            ImGui::Separator();

            std::vector<std::string_view> anim_list;
            auto                          anim_names_node = char_file.getAnimNames();
            for (auto anim_name : anim_names_node.children())
            {
                std::string_view name_str = anim_name.text().as_string();
                if (search_text.empty() ||
                    !std::ranges::search(name_str, search_text, [](char ch1, char ch2) { return std::toupper(ch1) == std::toupper(ch2); }).empty())
                    anim_list.push_back(name_str);
            }

            if (anim_list.empty())
                ImGui::TextDisabled("No animations.");
            else
            {
                ImGui::BeginChild("ITEMS", {400.0f, 20.0f * std::min(anim_list.size(), 30ull)}, false);
                {
                    ImGuiListClipper clipper;
                    clipper.Begin(anim_list.size());

                    while (clipper.Step())
                        for (int row_n = clipper.DisplayStart; row_n < clipper.DisplayEnd; row_n++)
                        {
                            auto anim = anim_list[row_n];

                            if (ImGui::Selectable(anim.data(), value == anim))
                            {
                                ImGui::CloseCurrentPopup();
                                ImGui::EndChild();
                                ImGui::EndPopup();
                                return anim;
                            }
                        }
                }
                ImGui::EndChild();
            }
        }
        else
            ImGui::TextDisabled("No loaded skeleton file.");

        ImGui::EndPopup();
    }
    return std::nullopt;
}

void varBindingButton(const char* str_id, pugi::xml_node hkparam, Hkx::BehaviourFile& file)
{
    static pugi::xml_node param_cache;
    static pugi::xml_node bindings     = {};
    static std::string    param_path   = {};
    static pugi::xml_node prev_binding = {};

    if (auto binding_set = getParentObj(hkparam).getByName("variableBindingSet"); binding_set)
    {
        ImGui::PushID(str_id);

        if (ImGui::Button(ICON_FA_LINK))
        {
            bindings   = file.getObj(binding_set.text().as_string()).getByName("bindings");
            param_path = getParamPath(hkparam);
            // auto prev_bindings = bindings.select_nodes(fmt::format("/hkparam/hkobject/hkparam[@name='memberPath' and text()='{}']", param_path).c_str()); // fuck xpath
            prev_binding = bindings.find_node([](auto node) { return !strcmp(node.attribute("name").as_string(), "memberPath") && (param_path == node.text().as_string()); }).parent();
            ImGui::OpenPopup("bindmenu");
        }
        addTooltip("Edit binding");

        bool open_var  = false;
        bool open_prop = false;
        if (ImGui::BeginPopup("bindmenu", ImGuiWindowFlags_AlwaysAutoResize))
        {
            if (prev_binding)
            {
                std::string var_name;
                auto        varidx = prev_binding.getByName("variableIndex").text().as_string();
                if (!strcmp(prev_binding.getByName("bindingType").text().as_string(), "BINDING_TYPE_VARIABLE"))
                    var_name = file.m_var_manager.getEntry(prev_binding.getByName("variableIndex").text().as_int()).getName();
                else
                    var_name = file.m_prop_manager.getEntry(prev_binding.getByName("variableIndex").text().as_int()).getName();
                ImGui::TextUnformatted(ICON_FA_LINK);
                ImGui::SameLine();
                ImGui::TextUnformatted("Bound with:");
                ImGui::TextUnformatted(var_name.c_str());

                if (ImGui::BeginTable("bit", 2, ImGuiTableFlags_SizingStretchProp))
                {
                    intScalarEdit(prev_binding.getByName("bitIndex"), file, ImGuiDataType_S8,
                                  "The index of the bit to which the variable is bound.\n"
                                  "A value of -1 indicates that we are not binding to an individual bit.");
                    ImGui::EndTable();
                }
            }
            else
                ImGui::TextDisabled("Not bound with any variable.");

            ImGui::Separator();

            if (ImGui::Selectable("Bind Varaibale"))
                open_var = true;
            if (ImGui::Selectable("Bind Character Property"))
                open_prop = true;

            ImGui::EndPopup();
        }

        bool set_focus = false;
        if (open_var)
        {
            set_focus = true;
            ImGui::OpenPopup("bindvar");
        }
        if (open_prop)
        {
            set_focus = true;
            ImGui::OpenPopup("bindprop");
        }

        auto var  = linkedPropPickerPopup("bindvar", file.m_var_manager, set_focus);
        auto prop = linkedPropPickerPopup("bindprop", file.m_prop_manager, set_focus);

        if (var.has_value() || prop.has_value())
        {
            auto parent = getParentObj(hkparam);
            if (!bindings)
            {
                auto bindings_id = file.addObj("hkbVariableBindingSet");
                file.addRef(bindings_id, parent.attribute("name").as_string());
                binding_set.text() = bindings_id.data();
                bindings           = file.getObj(bindings_id).getByName("bindings");
            }
            if (!prev_binding)
            {
                prev_binding                                = appendXmlString(bindings, Hkx::g_def_hkbVariableBindingSet_Binding);
                prev_binding.getByName("memberPath").text() = param_path.c_str();
            }
            if (var.has_value())
            {
                prev_binding.getByName("variableIndex").text() = var.value().m_index;
                prev_binding.getByName("bindingType").text()   = "BINDING_TYPE_VARIABLE";
            }
            else
            {
                prev_binding.getByName("variableIndex").text() = prop.value().m_index;
                prev_binding.getByName("bindingType").text()   = "BINDING_TYPE_CHARACTER_PROPERTY";
            }
            if ((param_path == "enable") && std::string_view(parent.attribute("class").as_string()).contains("Modifier"))
                bindings.parent().getByName("indexOfBindingToEnable").text() = getChildIndex(prev_binding);
        }

        ImGui::PopID();
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

void animEdit(pugi::xml_node      hkparam,
              Hkx::CharacterFile& char_file,
              std::string_view    hint,
              std::string_view    manual_name)
{
    ImGui::PushID(manual_name.empty() ? hkparam.attribute("name").as_string() : manual_name.data());

    stringEdit(hkparam, hint, manual_name);

    auto res = animPickerButton(char_file, hkparam.text().as_string());
    if (res.has_value())
        hkparam.text() = res.value().data();

    ImGui::PopID();
}

void boolEdit(pugi::xml_node hkparam, Hkx::BehaviourFile& file, std::string_view hint, std::string_view manual_name)
{
    ImGui::TableNextColumn();
    bool value = hkparam.text().as_bool();
    if (ImGui::Checkbox(manual_name.empty() ? hkparam.attribute("name").as_string() : manual_name.data(), &value))
        hkparam.text() = value ? "true" : "false";
    if (!hint.empty())
        addTooltip(hint.data());

    ImGui::TableNextColumn();
    varBindingButton(manual_name.empty() ? hkparam.attribute("name").as_string() : manual_name.data(), hkparam, file);
}

void intScalarEdit(pugi::xml_node hkparam, Hkx::BehaviourFile& file, ImGuiDataType data_type, std::string_view hint, std::string_view manual_name)
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
    varBindingButton(manual_name.empty() ? hkparam.attribute("name").as_string() : manual_name.data(), hkparam, file);
}

// void sliderIntEdit(pugi::xml_node hkparam, int lb, int ub, std::string_view hint, std::string_view manual_name)
// {
//     ImGui::TableNextColumn();
//     int value = hkparam.text().as_int();
//     if (ImGui::SliderInt(manual_name.empty() ? hkparam.attribute("name").as_string() : manual_name.data(), &value, lb, ub, "%.6f"))
//         hkparam.text() = value;
//     if (!hint.empty())
//         addTooltip(hint.data());

//     ImGui::TableNextColumn();
// }

void sliderFloatEdit(pugi::xml_node hkparam, float lb, float ub, Hkx::BehaviourFile& file, std::string_view hint, std::string_view manual_name)
{
    ImGui::TableNextColumn();
    float value = hkparam.text().as_float();
    if (ImGui::SliderFloat(manual_name.empty() ? hkparam.attribute("name").as_string() : manual_name.data(), &value, lb, ub, "%.6f"))
        hkparam.text() = fmt::format("{:.6f}", value).c_str();
    if (!hint.empty())
        addTooltip(hint.data());

    ImGui::TableNextColumn();
    varBindingButton(manual_name.empty() ? hkparam.attribute("name").as_string() : manual_name.data(), hkparam, file);
}

void floatEdit(pugi::xml_node hkparam, Hkx::BehaviourFile& file, std::string_view hint, std::string_view manual_name)
{
    ImGui::TableNextColumn();
    float value = hkparam.text().as_float();
    if (ImGui::InputFloat(manual_name.empty() ? hkparam.attribute("name").as_string() : manual_name.data(), &value, 0.0f, 0.0f, "%.6f"))
        hkparam.text() = fmt::format("{:.6f}", value).c_str();
    if (!hint.empty())
        addTooltip(hint.data());

    ImGui::TableNextColumn();
    varBindingButton(manual_name.empty() ? hkparam.attribute("name").as_string() : manual_name.data(), hkparam, file);
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

void quadEdit(pugi::xml_node hkparam, Hkx::BehaviourFile& file, std::string_view hint, std::string_view manual_name)
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
    varBindingButton(manual_name.empty() ? hkparam.attribute("name").as_string() : manual_name.data(), hkparam, file);
}

void refEdit(pugi::xml_node hkparam, const std::vector<std::string_view>& classes, Hkx::BehaviourFile& file, std::string_view hint, std::string_view manual_name)
{
    ImGui::TableNextColumn();

    ImGui::PushID(manual_name.empty() ? hkparam.attribute("name").as_string() : manual_name.data());

    bool        edited    = false;
    std::string old_value = hkparam.text().as_string();
    auto        value     = old_value;

    if (ImGui::InputText(manual_name.empty() ? hkparam.attribute("name").as_string() : manual_name.data(), &value))
    {
        edited = true;
        if (auto obj = file.getObj(value); obj)                                                                                        // Is obj
            if (!classes.empty() && std::ranges::find(classes, std::string_view(obj.attribute("class").as_string())) != classes.end()) // Same class
                hkparam.text() = value.c_str();
            else
                hkparam.text() = "null";
        else
            hkparam.text() = "null";
    }
    if (!hint.empty())
        addTooltip(hint.data());

    ImGui::TableNextColumn();
    varBindingButton(manual_name.empty() ? hkparam.attribute("name").as_string() : manual_name.data(), hkparam, file);

    if (getParentObj(hkparam).getByName("variableBindingSet"))
        ImGui::SameLine();
    if (ImGui::Button(ICON_FA_ARROW_CIRCLE_RIGHT))
        PropEdit::getSingleton()->setObject(value);
    addTooltip("Go to");

    ImGui::SameLine();
    if (ImGui::BeginPopup("Append"))
    {
        auto& class_info = Hkx::getClassInfo();
        for (auto class_name : classes)
        {
            if (!Hkx::getClassDefaultMap().contains(class_name))
                ImGui::BeginDisabled();
            if (ImGui::Selectable(class_name.data()))
            {
                value = file.addObj(class_name);
                if (!value.empty())
                {
                    hkparam.text() = value.c_str();
                    edited         = true;
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
    if (ImGui::Button(ICON_FA_PLUS_CIRCLE))
        ImGui::OpenPopup("Append");
    addTooltip("Append");
    if (auto obj = file.getObj(hkparam.text().as_string()); obj)
    {
        ImGui::SameLine();
        ImGui::TextUnformatted(getObjContextName(obj));
    }

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

bool refEdit(std::string&                         value,
             const std::vector<std::string_view>& classes,
             pugi::xml_node                       parent,
             Hkx::BehaviourFile&                  file,
             std::string_view                     hint,
             std::string_view                     manual_name)
{
    ImGui::TableNextColumn();

    ImGui::PushID(manual_name.data());

    bool        edited    = false;
    std::string old_value = value;

    if (ImGui::Button(ICON_FA_ARROW_CIRCLE_RIGHT))
        PropEdit::getSingleton()->setObject(value);
    addTooltip("Go to");

    ImGui::SameLine();
    if (ImGui::BeginPopup("Append"))
    {
        auto& class_info = Hkx::getClassInfo();
        for (auto class_name : classes)
        {
            if (!Hkx::getClassDefaultMap().contains(class_name))
                ImGui::BeginDisabled();
            if (ImGui::Selectable(class_name.data()))
            {
                value  = file.addObj(class_name);
                edited = true;
                ImGui::CloseCurrentPopup();
                break;
            }
            if (!Hkx::getClassDefaultMap().contains(class_name))
                ImGui::EndDisabled();
            if (class_info.contains(class_name))
                addTooltip(class_info.at(class_name).data());
        }
        ImGui::EndPopup();
    }
    if (ImGui::Button(ICON_FA_PLUS_CIRCLE))
        ImGui::OpenPopup("Append");
    addTooltip("Append");
    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_MINUS_CIRCLE))
    {
        ImGui::PopID();
        return true;
    }
    addTooltip("Remove");

    ImGui::TableNextColumn();

    ImGui::SetNextItemWidth(60);
    if (ImGui::InputText(manual_name.data(), &value))
    {
        edited = true;
        if (auto obj = file.getObj(value); obj) // Is obj
        {
            if (!classes.empty() && std::ranges::find(classes, std::string_view(obj.attribute("class").as_string())) == classes.end()) // Different class
                value = "null";
        }
        else
            value = "null";
    }
    if (!hint.empty())
        addTooltip(hint.data());

    if (edited)
    {
        auto parent_id = parent.attribute("name").as_string();
        if (!isRefBy(old_value, parent))
            file.deRef(old_value, parent_id);
        file.addRef(value, parent_id);
        // for states
        if (auto obj = file.getObj(value); !strcmp(obj.attribute("class").as_string(), "hkbStateMachineStateInfo"))
            obj.getByName("stateId").text() = getBiggestStateId(getParentStateMachine(obj, file), file) + 1;
    }

    ImGui::PopID();

    return false;
}

void refList(pugi::xml_node                       hkparam,
             const std::vector<std::string_view>& classes,
             Hkx::BehaviourFile&                  file,
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
        objs.push_back("null");
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

            if (refEdit(objs[i], classes, parent, file,
                        hint_attribute.empty() ? "" : file.getObj(objs[i]).getByName(hint_attribute.data()).text().as_string(),
                        name_attribute.empty() ? "" : file.getObj(objs[i]).getByName(name_attribute.data()).text().as_string()))
            {
                do_delete   = true;
                mark_delete = i;
            }

            ImGui::PopID();
        }
        if (do_delete)
            objs.erase(objs.begin() + mark_delete);

        hkparam.text()                   = printVector(objs).c_str();
        hkparam.attribute("numelements") = objs.size();

        ImGui::EndTable();
    }
}

pugi::xml_node refLiveEditList(
    pugi::xml_node                       hkparam,
    const std::vector<std::string_view>& classes,
    Hkx::BehaviourFile&                  file,
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
    if (ImGui::BeginTable("states", 3, table_flag, ImVec2(-FLT_MIN, -FLT_MIN)))
    {
        size_t mark_delete = UINT64_MAX;
        bool   do_delete   = false;
        for (size_t i = 0; i < objs.size(); ++i)
        {
            auto obj = Hkx::HkxFileManager::getSingleton()->getCurrentFile().getObj(objs[i]);

            ImGui::PushID(i);

            ImGui::TableNextColumn();
            if (ImGui::Button(ICON_FA_PEN))
            {
                ImGui::PopID();
                ImGui::EndTable();
                return obj;
            }
            addTooltip("Edit here");

            if (refEdit(objs[i], classes, parent, file,
                        obj.getByName(hint_attribute.data()).text().as_string(),
                        obj.getByName(name_attribute.data()).text().as_string()))
            {
                do_delete   = true;
                mark_delete = i;
            }

            ImGui::PopID();
        }
        if (do_delete)
        {
            objs.erase(objs.begin() + mark_delete);
            hkparam.attribute("numelements") = hkparam.attribute("numelements").as_int() - 1;
        }

        hkparam.text() = printVector(objs).c_str();

        ImGui::EndTable();
    }
    return {};
}

void objLiveEditList(
    pugi::xml_node                             hkparam,
    pugi::xml_node&                            edit_item,
    const char*                                def_str,
    Hkx::BehaviourFile&                        file,
    std::string_view                           str_id,
    std::function<std::string(pugi::xml_node)> disp_name_func)
{
    ImGui::AlignTextToFramePadding();
    ImGui::BulletText(str_id.data()), ImGui::SameLine();
    if (ImGui::Button(ICON_FA_PLUS_CIRCLE))
        appendXmlString(hkparam, def_str);
    addTooltip("Add new item");
    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_MINUS_CIRCLE) && edit_item)
        if (hkparam.remove_child(edit_item))
        {
            edit_item                        = {};
            hkparam.attribute("numelements") = hkparam.attribute("numelements").as_int() - 1;
        }
    addTooltip("Remove currently editing item.");
    ImGui::SameLine();
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(hkparam.attribute("numelements").as_string());

    if (ImGui::BeginListBox(fmt::format("##{}", str_id).c_str(), ImVec2(-FLT_MIN, -FLT_MIN)))
    {
        for (auto item : hkparam.children())
        {
            const bool is_selected = edit_item == item;
            auto       disp_name   = disp_name_func(item);
            if (ImGui::Selectable(disp_name.c_str(), is_selected))
                edit_item = item;
        }
        ImGui::EndListBox();
    }
}

void stateEdit(pugi::xml_node      hkparam,
               Hkx::BehaviourFile& file,
               pugi::xml_node      state_machine,
               std::string_view    hint,
               std::string_view    manual_name)
{
    ImGui::PushID(manual_name.empty() ? hkparam.attribute("name").as_string() : manual_name.data());

    intScalarEdit(hkparam, file, 4, hint, manual_name);
    if (getParentObj(hkparam).getByName("variableBindingSet"))
        ImGui::SameLine();

    if (ImGui::Button(ICON_FA_SEARCH))
        ImGui::OpenPopup("select state");
    addTooltip("Select");

    if (state_machine)
    {
        statePickerPopup("select state", hkparam, state_machine, file);
        if (auto state = getStateById(state_machine, hkparam.text().as_int(), file); state)
        {
            ImGui::SameLine();
            ImGui::TextUnformatted(state.getByName("name").text().as_string());
        }
    }

    ImGui::PopID();
}

void boneEdit(pugi::xml_node      hkparam,
              Hkx::BehaviourFile& file,
              Hkx::SkeletonFile&  skel_file,
              std::string_view    hint,
              std::string_view    manual_name)
{
    ImGui::PushID(manual_name.empty() ? hkparam.attribute("name").as_string() : manual_name.data());

    intScalarEdit(hkparam, file, ImGuiDataType_S16, hint, manual_name);
    if (getParentObj(hkparam).getByName("variableBindingSet"))
        ImGui::SameLine();

    auto picked_bone = bonePickerButton(skel_file, hkparam.text().as_int());
    if (picked_bone.has_value())
        hkparam.text() = picked_bone.value();

    ImGui::PopID();
}

////////////////    Linked Prop Edits

void varEditPopup(const char* str_id, Hkx::Variable& var, Hkx::BehaviourFile& file)
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
                    boolEdit(var_value_node, file);
                    break;
                case Hkx::VARIABLE_TYPE_INT8:
                    intScalarEdit(var_value_node, file, ImGuiDataType_S8);
                    break;
                case Hkx::VARIABLE_TYPE_INT16:
                    intScalarEdit(var_value_node, file, ImGuiDataType_S16);
                    break;
                case Hkx::VARIABLE_TYPE_INT32:
                    intScalarEdit(var_value_node, file, ImGuiDataType_S32);
                    break;
                case Hkx::VARIABLE_TYPE_REAL:
                    convertFloatEdit(var_value_node);
                    break;
                case Hkx::VARIABLE_TYPE_POINTER:
                    ImGui::TableNextColumn();
                    if (ImGui::InputText("value", &file.m_var_manager.getPointerValue(var_value_node.text().as_uint())))
                        file.buildRefList(getParentObj(var.get<Hkx::PropWordValue>()).attribute("name").as_string());
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

void evtEditPopup(const char* str_id, Hkx::AnimationEvent& evt, Hkx::BehaviourFile& file)
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

void propEditPopup(const char* str_id, Hkx::CharacterProperty& prop, Hkx::BehaviourFile& file)
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