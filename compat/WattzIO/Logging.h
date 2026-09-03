#pragma once

// REX:: logging on spdlog. CommonLibF4RD has no REX:: namespace.
//
// REX::DEBUG is always compiled in and gated at runtime by the logger's level, so a
// plugin can switch diagnostics on in a releasedbg build. spdlog checks the level
// before formatting, so a dropped line costs an argument evaluation and a comparison,
// not a format.

#include <spdlog/spdlog.h>

#include <utility>

namespace REX
{
	template <class... Args>
	struct [[maybe_unused]] INFO
	{
		INFO() = delete;
		explicit INFO(fmt::format_string<Args...> a_fmt, Args&&... a_args)
		{
			spdlog::info(a_fmt, std::forward<Args>(a_args)...);
		}
	};
	template <class... Args>
	INFO(fmt::format_string<Args...>, Args&&...) -> INFO<Args...>;

	template <class... Args>
	struct [[maybe_unused]] WARN
	{
		WARN() = delete;
		explicit WARN(fmt::format_string<Args...> a_fmt, Args&&... a_args)
		{
			spdlog::warn(a_fmt, std::forward<Args>(a_args)...);
		}
	};
	template <class... Args>
	WARN(fmt::format_string<Args...>, Args&&...) -> WARN<Args...>;

	template <class... Args>
	struct [[maybe_unused]] ERROR
	{
		ERROR() = delete;
		explicit ERROR(fmt::format_string<Args...> a_fmt, Args&&... a_args)
		{
			spdlog::error(a_fmt, std::forward<Args>(a_args)...);
		}
	};
	template <class... Args>
	ERROR(fmt::format_string<Args...>, Args&&...) -> ERROR<Args...>;

	template <class... Args>
	struct [[maybe_unused]] DEBUG
	{
		DEBUG() = delete;
		explicit DEBUG(fmt::format_string<Args...> a_fmt, Args&&... a_args)
		{
			spdlog::debug(a_fmt, std::forward<Args>(a_args)...);
		}
	};
	template <class... Args>
	DEBUG(fmt::format_string<Args...>, Args&&...) -> DEBUG<Args...>;

	// REX::W32, thin Win32 wrappers. Declared here rather than including <Windows.h>,
	// which would collide with F4SE/Impl/WinAPI.h.
	namespace W32
	{
		extern "C" __declspec(dllimport) unsigned long __stdcall GetPrivateProfileStringA(
			const char* lpAppName, const char* lpKeyName, const char* lpDefault,
			char* lpReturnedString, unsigned long nSize, const char* lpFileName);

		extern "C" __declspec(dllimport) int __stdcall WritePrivateProfileStringA(
			const char* lpAppName, const char* lpKeyName, const char* lpString,
			const char* lpFileName);
	}

	// REX::UNRESTRICTED_CAST
	template <class To, class From>
	[[nodiscard]] To UNRESTRICTED_CAST(From a_from)
	{
		return F4SE::stl::unrestricted_cast<To>(a_from);
	}
}
