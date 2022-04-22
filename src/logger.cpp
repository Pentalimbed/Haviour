#include "logger.h"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/details/null_mutex.h>
#include <imgui.h>
#include <extern/imgui_notify.h>

namespace Haviour
{
template <typename Mutex>
class notify_sink : public spdlog::sinks::base_sink<Mutex>
{
protected:
    void sink_it_(const spdlog::details::log_msg& msg) override
    {
        if (!ImGui::GetCurrentContext())
            return;

        std::string str(msg.payload.begin(), msg.payload.end());

        switch (msg.level)
        {
            case spdlog::level::info:
                ImGui::InsertNotification({ImGuiToastType_Info, 5000, str.c_str()});
                break;
            case spdlog::level::warn:
                ImGui::InsertNotification({ImGuiToastType_Warning, 5000, str.c_str()});
                break;
            case spdlog::level::err:
            case spdlog::level::critical:
                ImGui::InsertNotification({ImGuiToastType_Error, 8000, str.c_str()});
                break;
            default:
                break;
        }
    }

    void flush_() override {}
};

void setupLogger()
{
    auto max_size  = (1 << 20) * 10;
    auto max_files = 3;
    auto logger    = spdlog::rotating_logger_mt("app", "log/Haviour.log", max_size, max_files);

    logger->set_level(spdlog::level::debug);
    logger->flush_on(spdlog::level::debug);
    logger->set_pattern("[%H:%M:%S:%e] [%n] [%l] %v");

    logger->sinks().push_back(std::make_shared<notify_sink<spdlog::details::null_mutex>>());

    spdlog::set_default_logger(logger);
}
} // namespace Haviour
