#pragma once

#include <string>

#include <imgui.h>

#include "hkx/hkxfile.h"

namespace Haviour
{
namespace Ui
{
class ListView
{
public:
    static ListView* getSingleton();
    void             show();

private:
    ListView();
    ~ListView();

    Hkx::HkxFile::Handle m_file_listener;

    int           m_current_class_idx = 0;
    Hkx::NodeList m_cache_list;

    enum ColumnId
    {
        kColAction,
        kColId,
        kColName,
        kColClass
    };
    ImGuiTableSortSpecs m_current_sort_spec;
    std::string         m_filter_string;
    bool                m_filter_id = false;

    void updateCache();
    void drawTable();
};
} // namespace Ui

} // namespace Haviour
