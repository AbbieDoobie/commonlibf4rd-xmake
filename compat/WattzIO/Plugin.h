#pragma once

// Plugin entry point. CommonLibF4RD generates neither F4SEPlugin_Version nor the log
// sink, so this is the shared declaration of both.
//
//     WIO_PLUGIN_VERSION("WIO-QuickTurn", "Abbie Doobie", 0, 0, 1);
//
//     F4SE_PLUGIN_LOAD(const F4SE::LoadInterface* a_f4se)
//     {
//         WIO::Init(a_f4se, "WIO-QuickTurn"sv);
//         ...
//     }

#include <F4SE/F4SE.h>

#include <spdlog/sinks/basic_file_sink.h>
#ifndef NDEBUG
#	include <spdlog/sinks/msvc_sink.h>
#endif

#include <memory>
#include <string>
#include <string_view>

// Preserved spelling, so main.cpp's entry point is unchanged.
#define F4SE_PLUGIN_LOAD(...) extern "C" [[maybe_unused]] __declspec(dllexport) bool F4SEAPI F4SEPlugin_Load(__VA_ARGS__)

// Emits F4SEPlugin_Version with signature address-independence and both
// structure-independence layouts.
//
// F4RD relocates function starts, not interior offsets. A plugin that hardcodes byte
// offsets inside a function needs its own guard or REL::AUTO_CALLSITE.
#define WIO_PLUGIN_VERSION(a_name, a_author, a_major, a_minor, a_patch)                    \
	extern "C" [[maybe_unused]] __declspec(dllexport) constinit F4SE::PluginVersionData    \
		F4SEPlugin_Version = []() noexcept {                                               \
			F4SE::PluginVersionData v{};                                                   \
			v.dataVersion = F4SE::PluginVersionData::kVersion;                             \
			v.pluginVersion = WIO::detail::PackVersion(a_major, a_minor, a_patch);         \
			WIO::detail::CopyField(v.name, a_name);                                        \
			WIO::detail::CopyField(v.author, a_author);                                    \
			v.addressIndependence =                                                        \
				F4SE::PluginVersionData::kAddressIndependence_Signatures;                  \
			v.structureIndependence =                                                      \
				F4SE::PluginVersionData::kStructureIndependence_1_10_980Layout |           \
				F4SE::PluginVersionData::kStructureIndependence_1_11_137Layout;            \
			return v;                                                                      \
		}()

namespace WIO
{
	namespace detail
	{
		[[nodiscard]] constexpr std::uint32_t PackVersion(
			std::uint32_t a_major, std::uint32_t a_minor, std::uint32_t a_patch) noexcept
		{
			return ((a_major & 0xFF) << 24) | ((a_minor & 0xFF) << 16) | ((a_patch & 0xFFF) << 4);
		}

		template <std::size_t N>
		constexpr void CopyField(char (&a_dst)[N], std::string_view a_src) noexcept
		{
			const auto count = a_src.size() < N - 1 ? a_src.size() : N - 1;
			for (std::size_t i = 0; i < count; ++i) {
				a_dst[i] = a_src[i];
			}
			a_dst[count] = '\0';
		}
	}

	// Sets the live log level. REX::DEBUG is spdlog::debug, so a release build at Info drops
	// every REX::DEBUG line until this raises the level.
	//
	// Call from the settings loader rather than at startup only, so it follows a settings
	// reload as well as the first read.
	//
	// No effect in a debug build, which stays at trace.
	inline void SetVerbose([[maybe_unused]] bool a_verbose)
	{
#ifdef NDEBUG
		const auto level = a_verbose ? spdlog::level::debug : spdlog::level::info;
		spdlog::set_level(level);
		spdlog::flush_on(level);
#endif
	}

	// Sets up Documents\My Games\Fallout4\F4SE\<a_name>.log, then F4SE::Init.
	//
	// a_verbose false sets the level to Info, which drops every REX::DEBUG line. A plugin whose
	// verbose switch lives in its settings file calls SetVerbose from its settings loader.
	inline bool Init(const F4SE::LoadInterface* a_f4se, std::string_view a_name,
		bool a_verbose = false)
	{
#ifndef NDEBUG
		auto sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
#else
		auto path = F4SE::log::log_directory();
		if (!path) {
			return false;
		}
		*path /= std::string{ a_name } + ".log";
		auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
#endif

		auto log = std::make_shared<spdlog::logger>(std::string{ a_name }, std::move(sink));
		log->set_level(spdlog::level::trace);
		log->flush_on(spdlog::level::trace);
		spdlog::set_default_logger(std::move(log));
		spdlog::set_pattern("[%H:%M:%S.%e] [%^%l%$] %v");

		// No-op in a debug build, which stays at trace.
		SetVerbose(a_verbose);

		F4SE::Init(a_f4se);
		return true;
	}
}
