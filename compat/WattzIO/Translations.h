#pragma once

// === F4RD RELOCATIONS ========================================================
// kind  what                                                OG      NG/AE
// id    BSTScatterTable insert, as called by
//       BSScaleformTranslator::AddTranslations              266342  2299490
//
// Re-derive:
//   1. The thunk has exactly one caller, the translation-file parser, and that
//      parser has exactly one caller, which holds "Interface\Translate_%s.txt".
//   2. Find that literal in the target binary, follow the RIP-relative lea that
//      loads its address back to the enclosing function via the .pdata table,
//      then match the call sequence against the AE one - identical but for
//      addresses.
//   3. In the parser, the insert is the call immediately after the two
//      BSFixedStringWCS constructions, with rcx = translator + 0x20.
//   4. Reverse-map that function start through the legacy OG Address Library
//      table to get the OG id.
// =============================================================================

// Scaleform translation loading, which CommonLibF4RD provides none of.
//
// Parses the translation file directly - UTF-16LE + BOM, tab between key and value,
// CRLF - and inserts each pair through the game's own translation-map insert. Both
// strings are passed by hidden reference, the MSVC ABI for a class with a non-trivial
// destructor.
//
// This does not go through BSResource, so a BA2-packed translation will not load;
// translation files have to ship loose.
//
// Localize() reads that map back. Scaleform resolves a "$KEY" itself, but a HUD message
// or an MCM setting value is plain text and has to be resolved here first.

#include <RE/Fallout.h>

#include "WattzIO/Logging.h"
#include "WattzIO/REMissing.h"

#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <initializer_list>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

// Declared here rather than including <Windows.h>, which collides with
// F4SE/Impl/WinAPI.h. Matches the Win32 signature exactly.
extern "C" __declspec(dllimport) int __stdcall WideCharToMultiByte(
	unsigned int CodePage, unsigned long dwFlags, const wchar_t* lpWideCharStr,
	int cchWideChar, char* lpMultiByteStr, int cbMultiByte, const char* lpDefaultChar,
	int* lpUsedDefaultChar);

namespace WIO::Translations
{
	namespace detail
	{
		// F4RD:id - (OG, AE); NG falls back to the AE value, which is correct here.
		inline constexpr REL::ID kMapInsert{ 266342, 2299490 };

		// WIO::Reloc::Address resolves through kMapInsert.id() and returns 0 rather than
		// aborting; REMissing.h documents both. An id with no OG number resolves nowhere
		// on 1.10.163, so this returns 0 there instead of failing the load.
		[[nodiscard]] inline std::uintptr_t MapInsertAddress()
		{
			static const std::uintptr_t address = WIO::Reloc::Address(kMapInsert);
			return address;
		}

		// BSTScatterTable insert, as called by AddTranslations. Callers must check
		// MapInsertAddress() first.
		inline void MapInsert(
			void* a_map,
			const RE::BSFixedStringWCS& a_key,
			const RE::BSFixedStringWCS& a_value)
		{
			using func_t = void (*)(void*, const RE::BSFixedStringWCS*, const RE::BSFixedStringWCS*);
			const auto func = reinterpret_cast<func_t>(MapInsertAddress());
			func(a_map, &a_key, &a_value);
		}

		// 65001 is CP_UTF8. A char-by-char narrowing would only be correct for ASCII.
		[[nodiscard]] inline std::string Narrow(const wchar_t* a_wide)
		{
			const auto needed = ::WideCharToMultiByte(65001, 0, a_wide, -1, nullptr, 0, nullptr, nullptr);
			if (needed <= 1) {
				return {};
			}
			std::string out(static_cast<std::size_t>(needed) - 1, '\0');
			::WideCharToMultiByte(65001, 0, a_wide, -1, out.data(), needed, nullptr, nullptr);
			return out;
		}

		// UTF-16LE file (BOM required) into a wide string; wchar_t is 16-bit here.
		[[nodiscard]] inline bool ReadUTF16File(const std::filesystem::path& a_path, std::wstring& a_out)
		{
			std::error_code ec;
			const auto size = std::filesystem::file_size(a_path, ec);
			if (ec || size < 2 || (size % 2) != 0) {
				return false;
			}

			std::FILE* file = nullptr;
			if (_wfopen_s(&file, a_path.c_str(), L"rb") != 0 || !file) {
				return false;
			}

			std::wstring buffer(static_cast<std::size_t>(size) / 2, L'\0');
			const auto read = std::fread(buffer.data(), 2, buffer.size(), file);
			std::fclose(file);
			if (read != buffer.size()) {
				return false;
			}

			if (buffer.front() != 0xFEFF) {  // BOM
				return false;
			}
			buffer.erase(buffer.begin());
			a_out = std::move(buffer);
			return true;
		}
	}

	// Loads one translation file and registers every entry with the game's translator.
	// Returns the number of entries registered, or 0 on any failure (which is logged).
	inline std::size_t LoadFile(const std::filesystem::path& a_path)
	{
		if (detail::MapInsertAddress() == 0) {
			REX::WARN(
				"Translations: the translation-map insert does not resolve on this runtime - '{}' not loaded",
				a_path.filename().string());
			return 0;
		}

		const auto manager = RE::BSScaleformManager::GetSingleton();
		const auto translator = RE::GetScaleformTranslator(manager);
		if (!translator) {
			REX::WARN("Translations: no BSScaleformTranslator - '{}' not loaded", a_path.filename().string());
			return 0;
		}

		std::wstring text;
		if (!detail::ReadUTF16File(a_path, text)) {
			REX::WARN("Translations: could not read '{}' as UTF-16LE", a_path.string());
			return 0;
		}

		auto* const map = std::addressof(translator->translator.translationMap);

		std::size_t count = 0;
		std::size_t pos = 0;
		while (pos <= text.size()) {
			auto end = text.find(L'\n', pos);
			if (end == std::wstring::npos) {
				end = text.size();
			}

			auto line = std::wstring_view{ text }.substr(pos, end - pos);
			pos = end + 1;

			if (!line.empty() && line.back() == L'\r') {
				line.remove_suffix(1);
			}
			if (line.empty()) {
				continue;
			}

			const auto tab = line.find(L'\t');
			if (tab == std::wstring_view::npos) {
				continue;  // no key/value separator - not an entry
			}

			const auto key = line.substr(0, tab);
			const auto value = line.substr(tab + 1);
			if (key.empty()) {
				continue;
			}

			// BSFixedStringWCS requires null-terminated input.
			const std::wstring keyStr{ key };
			const std::wstring valueStr{ value };

			detail::MapInsert(
				map,
				RE::BSFixedStringWCS{ keyStr.c_str() },
				RE::BSFixedStringWCS{ valueStr.c_str() });
			++count;
		}

		REX::INFO("Translations: registered {} entries from '{}'", count, a_path.filename().string());
		return count;
	}

	// Loads Data/Interface/Translations/<a_modName>_<LANG>.txt for the configured
	// language. Load "_en" first so a partial translation falls back to English
	// rather than rendering a raw $KEY.
	inline std::size_t LoadForMod(std::string_view a_modName)
	{
		// Setting::GetString returns a string_view; copy before concatenating.
		std::string language = "en";
		if (const auto setting = RE::GetINISetting("sLanguage:General"); setting) {
			if (const auto value = setting->GetString(); !value.empty()) {
				language.assign(value);
			}
		}

		const auto path = std::filesystem::path{ "Data/Interface/Translations" } /
			(std::string{ a_modName } + "_" + language + ".txt");
		return LoadFile(path);
	}

	// Resolves one "$KEY" out of the map LoadFile populates.
	//
	// Returns a_fallback if the translator is not up or the key is missing, so a bad or
	// BA2-packed translation file degrades to English rather than a raw key on screen.
	[[nodiscard]] inline std::string Localize(std::string_view a_key, std::string_view a_fallback)
	{
		const auto manager = RE::BSScaleformManager::GetSingleton();
		if (!manager || !manager->loader) {
			return std::string{ a_fallback };
		}
		const auto translator = RE::GetScaleformTranslator(manager);
		if (!translator) {
			return std::string{ a_fallback };
		}

		// BSFixedStringWCS needs null-terminated input. Keys are ASCII, so a plain widen.
		const std::wstring         wide{ a_key.begin(), a_key.end() };
		const RE::BSFixedStringWCS key{ wide.c_str() };
		const auto&                map = translator->translator.translationMap;

		if (const auto it = map.find(key); it != map.end()) {
			if (const auto* value = it->second.c_str(); value && *value) {
				return detail::Narrow(value);
			}
		}
		return std::string{ a_fallback };
	}

	// The same lookup, then {TOKEN} substitution, so each language places its own tokens.
	//
	// Replacements are not translated - pass names, numbers, engine text. Keep prose in the
	// key: words translated in isolation do not fit other languages' grammar.
	//
	// Plain find-and-replace, not std::format, so a value may contain braces freely.
	[[nodiscard]] inline std::string Localize(
		std::string_view                                                   a_key,
		std::string_view                                                   a_fallback,
		std::initializer_list<std::pair<std::string_view, std::string_view>> a_replacements)
	{
		std::string result = Localize(a_key, a_fallback);
		for (const auto& [token, value] : a_replacements) {
			for (std::size_t pos = 0; (pos = result.find(token, pos)) != std::string::npos;) {
				result.replace(pos, token.size(), value);
				pos += value.size();
			}
		}
		return result;
	}
}
