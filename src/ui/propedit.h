#pragma once

#include "hkx/hkxfile.h"

#include <deque>

namespace Haviour
{
namespace Ui
{
class PropEdit
{
public:
    static PropEdit* getSingleton();
    void             show();

    void setObject(pugi::xml_node hkobject);
    void setObject(std::string obj_id);

private:
    PropEdit();
    ~PropEdit();

    Hkx::HkxFile::Handle m_file_listener;

    std::string    m_edit_obj_id;
    pugi::xml_node m_edit_obj;

    static constexpr size_t    m_history_len = 100;
    std::deque<pugi::xml_node> m_history;

    void showHistoryList();
    void showRefList();
};
} // namespace Ui
} // namespace Haviour
