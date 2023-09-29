#ifndef PLUGIN_SETTINGS_H
#define PLUGIN_SETTINGS_H

#include "cura/plugins/slots/broadcast/v0/broadcast.grpc.pb.h"
#include "cura/plugins/slots/handshake/v0/handshake.grpc.pb.h"
#include "plugin/metadata.h"

#include <range/v3/all.hpp>

#include <algorithm>
#include <cctype>
#include <ctre.hpp>
#include <locale>
#include <optional>
#include <semver.hpp>
#include <string>
#include <unordered_map>

namespace plugin
{

struct Settings
{
    std::shared_ptr<Metadata> metadata;
    std::vector<bool> onlyfans_enabled;

    explicit Settings(const cura::plugins::slots::broadcast::v0::BroadcastServiceSettingsRequest& request, const std::shared_ptr<Metadata>& metadata)
        : metadata{ metadata }
    {
        const auto onlyfans_enabled_setting = retrieveSettings("onlyfans_enabled", request.global_settings(), metadata);
        if (! onlyfans_enabled_setting.has_value())
        {
            spdlog::error("Global Settings: <onlyfans_enabled: {}>",
                    onlyfans_enabled_setting.has_value());
            throw std::runtime_error(fmt::format("Global Settings: <onlyfans_enabled: {}>",
                                                 onlyfans_enabled_setting.has_value()));
        }
    }

    [[maybe_unused]] static std::optional<std::string> retrieveSettings(const std::string& settings_key, const cura::plugins::slots::broadcast::v0::Settings& settings, const auto& metadata)
    {
        const auto settings_key_ = settingKey(settings_key, metadata->plugin_name, metadata->plugin_version);
        if (settings.settings().contains(settings_key_))
        {
            return settings.settings().at(settings_key_);
        }

        return std::nullopt;
    }

    [[maybe_unused]] static std::optional<std::string> retrieveSettings(
        const std::string& settings_key,
        const size_t extruder_nr,
        const cura::plugins::slots::broadcast::v0::BroadcastServiceSettingsRequest& request,
        const auto& metadata)
    {
        const auto& settings = request.extruder_settings().at(static_cast<int>(extruder_nr));
        return retrieveSettings(settings_key, settings, metadata);
    }

    [[maybe_unused]] static std::optional<std::string>
        retrieveSettings(const std::string& settings_key, const cura::plugins::slots::broadcast::v0::BroadcastServiceSettingsRequest& request, const auto& metadata)
    {
        const auto& global_settings = request.global_settings();
        return retrieveSettings(settings_key, global_settings, metadata);
    }

    static bool validatePlugin(const cura::plugins::slots::handshake::v0::CallRequest& request, const std::shared_ptr<Metadata>& metadata)
    {
        auto plugin_name = request.plugin_name();
        auto plugin_name_expect = metadata->plugin_name;
        return plugin_name == plugin_name_expect && request.plugin_version() == metadata->plugin_version;
    }

    static std::string settingKey(std::string_view short_key, std::string_view name, std::string_view version)
    {
        std::string lower_name{ name };
        auto semantic_version = semver::from_string(version);
        std::transform(
            lower_name.begin(),
            lower_name.end(),
            lower_name.begin(),
            [](const auto& c)
            {
                return std::tolower(c);
            });
        return fmt::format("_plugin__{}__{}_{}_{}__{}", lower_name, semantic_version.major, semantic_version.minor, semantic_version.patch, short_key);
    }
};

using settings_t = std::unordered_map<std::string, Settings>;

} // namespace plugin

#endif // PLUGIN_SETTINGS_H
