// Variable list, also event list / character property list etc.
#pragma once

#include "hkx/hkxfile.h"

namespace Haviour
{
namespace Ui
{
class VarList
{
public:
    static VarList* getSingleton();
    void            show();

private:
    VarList();
    ~VarList();

    Hkx::HkxFile::Handle m_file_listener;

    int m_show_var_charprop_attr = 0;

    std::string   m_var_filter = {}, m_evt_filter = {}, m_charprop_filter = {};
    Hkx::Variable m_editing_var = {};
    Hkx::NameInfo m_editing_evt = {}, m_editing_charprop = {};

    void showVarList();
    void showVarEditUi(Hkx::Variable var);

    void showNameInfoList(std::string_view title, Hkx::NameInfoManger& manager, Hkx::NameInfo& selection, std::string& filter, std::function<void()> popup);
    void showEventEditUi(Hkx::NameInfo ninfo);
    void showCharPropEditUi(Hkx::NameInfo ninfo);
};
} // namespace Ui
} // namespace Haviour