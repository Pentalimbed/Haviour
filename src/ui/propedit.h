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

    bool m_show = true;

    void setObject(std::string_view obj_id);

private:
    PropEdit();
    ~PropEdit();

    Hkx::HkxFileManager::Handle m_file_listener;

    std::string m_edit_obj_id_input;
    std::string m_edit_obj_id;

    static constexpr size_t m_history_len = 30;
    std::deque<std::string> m_history;

    void showHistoryList();
    void showRefList();
};
} // namespace Ui
} // namespace Haviour
