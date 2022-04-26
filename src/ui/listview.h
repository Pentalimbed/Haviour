#pragma once

#include "hkx/hkxfile.h"

#include <imgui.h>

namespace Haviour
{
namespace Ui
{
class ListView
{
public:
    static ListView* getSingleton();
    void             show();
    bool             m_show = true;

private:
    ListView();
    ~ListView();

    Hkx::HkxFileManager::Handle m_file_listener, m_obj_listener;

    size_t                   m_current_class_idx = 0;
    std::vector<std::string> m_cache_list;

    enum ColumnId
    {
        kColAction,
        kColId,
        kColName,
        kColClass
    };
    ImGuiTableSortSpecs m_current_sort_spec;

    std::string m_filter;
    bool        m_do_filter_id = false;

    void updateCache(bool sort_only = false);
    void drawTable();
};
} // namespace Ui
} // namespace Haviour