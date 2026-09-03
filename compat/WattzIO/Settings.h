#pragma once

// REX:: registered-setting store, which CommonLibF4RD has no equivalent of.
//
// Each TIniSetting registers with the store on construction; the store loads or saves
// all of them against the ini given to Init().
//
//     inline REX::TIniSetting<bool> bThing{ "Section", "bThing", true };
//     REX::FIniSettingStore::GetSingleton()->Init(path, path);
//     REX::FIniSettingStore::GetSingleton()->Load();
//     ... bThing.GetValue() ...
//
// Supports bool, integers, float/double and std::string. Anything else fails to
// compile rather than silently doing nothing.

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

// Declared here rather than including <Windows.h>, which collides with
// F4SE/Impl/WinAPI.h. These match the Win32 signatures exactly, so they are ordinary
// redeclarations if a translation unit does pull Windows.h in some other way.
extern "C" __declspec(dllimport) unsigned long __stdcall GetPrivateProfileStringA(
	const char* lpAppName, const char* lpKeyName, const char* lpDefault,
	char* lpReturnedString, unsigned long nSize, const char* lpFileName);

extern "C" __declspec(dllimport) int __stdcall WritePrivateProfileStringA(
	const char* lpAppName, const char* lpKeyName, const char* lpString,
	const char* lpFileName);

// Boolean parsing for MCM-managed ini files.
//
// MCM writes 1/0, but a file written by CSimpleIni holds true/false, so both forms are accepted,
// case-insensitively and ignoring surrounding whitespace. A value matching neither falls back to
// the caller's default rather than reading as false.
namespace WIO::Ini
{
	[[nodiscard]] inline bool ParseBool(std::string_view a_value, bool a_default)
	{
		const auto space = [](char a_c) {
			return a_c == ' ' || a_c == '\t' || a_c == '\r' || a_c == '\n';
		};
		while (!a_value.empty() && space(a_value.front())) {
			a_value.remove_prefix(1);
		}
		while (!a_value.empty() && space(a_value.back())) {
			a_value.remove_suffix(1);
		}
		if (a_value.empty()) {
			return a_default;
		}

		const auto matches = [a_value](std::string_view a_literal) {
			if (a_value.size() != a_literal.size()) {
				return false;
			}
			for (std::size_t i = 0; i < a_value.size(); ++i) {
				const auto c = (a_value[i] >= 'A' && a_value[i] <= 'Z') ?
				                   static_cast<char>(a_value[i] + ('a' - 'A')) :
				                   a_value[i];
				if (c != a_literal[i]) {
					return false;
				}
			}
			return true;
		};

		if (a_value == "1" || matches("true")) {
			return true;
		}
		if (a_value == "0" || matches("false")) {
			return false;
		}
		return a_default;
	}

	[[nodiscard]] inline bool GetBool(const char* a_section, const char* a_key, bool a_default,
		const char* a_file)
	{
		char buffer[32]{};
		::GetPrivateProfileStringA(a_section, a_key, "", buffer, sizeof(buffer), a_file);
		return ParseBool(buffer, a_default);
	}
}

namespace REX
{
	struct ISetting
	{
		virtual ~ISetting() = default;
		virtual void Load(const char* a_file) = 0;
		virtual void Save(const char* a_file) = 0;
	};

	class FIniSettingStore
	{
	public:
		[[nodiscard]] static FIniSettingStore* GetSingleton()
		{
			static FIniSettingStore singleton;
			return &singleton;
		}

		// Callers pass the same path twice; only a_file is used. A separate override file
		// would need a second Load pass.
		void Init(const char* a_file, const char* /*a_fileCustom*/) { _file = a_file ? a_file : ""; }

		void Add(ISetting* a_setting)
		{
			if (a_setting) {
				_settings.push_back(a_setting);
			}
		}

		void Load()
		{
			if (_file.empty()) {
				return;
			}
			for (auto* setting : _settings) {
				setting->Load(_file.c_str());
			}
		}

		void Save()
		{
			if (_file.empty()) {
				return;
			}
			for (auto* setting : _settings) {
				setting->Save(_file.c_str());
			}
		}

	private:
		std::string _file;
		std::vector<ISetting*> _settings;
	};

	template <class T>
	class TSetting
	{
	public:
		explicit TSetting(T a_default) :
			m_value(a_default),
			m_valueDefault(a_default)
		{}

		[[nodiscard]] T GetValue() const { return m_value; }
		[[nodiscard]] T GetValueDefault() const { return m_valueDefault; }
		void SetValue(T a_value) { m_value = a_value; }

	protected:
		T m_value;
		T m_valueDefault;
	};

	template <class T, class S = FIniSettingStore>
	class TIniSetting :
		public TSetting<T>,
		public ISetting
	{
	public:
		TIniSetting(std::string_view a_section, std::string_view a_key, T a_default) :
			TSetting<T>(a_default),
			m_section(a_section),
			m_key(a_key)
		{
			S::GetSingleton()->Add(this);
		}

		TIniSetting(std::string_view a_key, T a_default) :
			TSetting<T>(a_default),
			m_key(a_key)
		{
			S::GetSingleton()->Add(this);
		}

		void Load(const char* a_file) override
		{
			char buf[512]{};
			::GetPrivateProfileStringA(m_section.c_str(), m_key.c_str(), "", buf, sizeof(buf), a_file);
			if (buf[0] == 0) {
				this->m_value = this->m_valueDefault;
				return;
			}

			if constexpr (std::is_same_v<T, std::string>) {
				this->m_value = buf;
			} else if constexpr (std::is_same_v<T, bool>) {
				this->m_value = WIO::Ini::ParseBool(buf, this->m_valueDefault);
			} else if constexpr (std::is_floating_point_v<T>) {
				try {
					this->m_value = static_cast<T>(std::stod(buf));
				} catch (...) {
					this->m_value = this->m_valueDefault;
				}
			} else {
				try {
					this->m_value = static_cast<T>(std::stoll(buf));
				} catch (...) {
					this->m_value = this->m_valueDefault;
				}
			}
		}

		void Save(const char* a_file) override
		{
			std::string text;
			if constexpr (std::is_same_v<T, std::string>) {
				text = this->m_value;
			} else if constexpr (std::is_same_v<T, bool>) {
				text = this->m_value ? "1" : "0";
			} else {
				text = std::to_string(this->m_value);
			}
			::WritePrivateProfileStringA(m_section.c_str(), m_key.c_str(), text.c_str(), a_file);
		}

	private:
		// Owned rather than string_view: the profile API needs null-terminated strings, and
		// a caller is free to pass a temporary.
		std::string m_section;
		std::string m_key;
	};
}
