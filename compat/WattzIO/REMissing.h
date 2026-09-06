#pragma once

// === F4RD RELOCATIONS ========================================================
// Every id here needs a real OG number or an F4RD known RVA. WIO::Reloc::Address
// resolves through ID::id(), which on OG reaches resolve_impl in kNormal mode ->
// the runtime database's kKnownOnly, returning before any pattern scan.
//
// 'data' rows name a static object or a pointer variable rather than a function.
// The two are not interchangeable: a static object must not be loaded through.
//
// kind  what                                          OG        NG/AE
// id    PlayerCharacter::GetDifficultyLevel           922962    2233056
// data  BSTimer singleton (a static object)           1256126   2703179
// id    TESLoadGameEvent::GetEventSource              823570    2201848
// id    TESObjectREFR::GetDisplayFullName             1212056   2201126
// id    FavoritesManager::UseQuickkeyItem             303130    2248744
// data  BSAudioManager singleton (a pointer var)      1321158   2703058
// id    BSAudioManager::GetSoundHandle                1419045   2267105
// id    BSSoundHandle::Play                           384073    2267042
// id    MenuControls::RegisterHandler                 827678    2249387
// id    BGSInventoryList::GetItemCount                894081    2194163
// id    BGSInventoryList::GetQuestItemCount           800903    2194164
// id    PlayerCharacter::ShowPipboyLight              1304102   2233203
// data  PlayerCrosshairModeEvent source (pointer var) 1231665   4801808   (NG 2694517)
//
// To re-derive an OG id: match the AE function to its 1.10.163 twin, then
// reverse-map the OG address through the legacy Address Library table. Anchors
// are ids the F4RD runtime database knows on both runtimes, RTTI vtables (class
// by name, slot by index), and string literals referenced by exactly one
// function per image. Align an anchored caller's call sequence against its OG
// twin's (Needleman-Wunsch, scored through the anchors) and read off the call in
// the target's position; several callers must agree. With no anchored caller,
// align the adjacent run of functions or pair through a vtable slot, and confirm
// instruction by instruction.
// =============================================================================

// RE:: declarations CommonLibF4RD lacks.

#include <RE/Fallout.h>

#include <cstdint>
#include <string_view>

namespace WIO::Reloc
{
	// Resolves through a_id.id(), F4RD's per-family selector, rather than the REL::ID.
	// IDDatabase::resolve(const ID&) pattern-scans the AE id first on OG and consults
	// the OG id only if that fails, so a false positive outranks a correct OG id: on
	// 1.10.163 the AE radio-emitter id matches 0x235E80, which is not the radio emitter.
	//
	// Returns 0 rather than aborting: REL::Relocation's constructor calls
	// report_and_fail, taking the process down on the first unresolvable id.
	[[nodiscard]] inline std::uintptr_t Address(const REL::ID& a_id)
	{
		const auto result = REL::IDDatabase::get().resolve(a_id.id());
		return result.rva ? REL::Module::get().base() + *result.rva : 0;
	}
}

namespace WIO::CompatIDs
{
	// F4RD:id - (OG, AE). NG falls back to the AE value, which is correct for all of
	// these but one: every other id resolves on 1.10.984 under its AE number.
	//
	// The crosshair event source is the exception and takes the three-argument form:
	// its AE id is absent from the 1.10.984 table, so the NG fallback has nothing
	// to find.
	inline constexpr REL::ID kGetDifficultyLevel{ 922962, 2233056 };
	inline constexpr REL::ID kBSTimerSingleton{ 1256126, 2703179 };
	inline constexpr REL::ID kLoadGameEventSource{ 823570, 2201848 };
	inline constexpr REL::ID kGetDisplayFullName{ 1212056, 2201126 };
	inline constexpr REL::ID kUseQuickkeyItem{ 303130, 2248744 };
	inline constexpr REL::ID kAudioManagerSingleton{ 1321158, 2703058 };
	inline constexpr REL::ID kGetSoundHandle{ 1419045, 2267105 };
	inline constexpr REL::ID kPlaySoundHandle{ 384073, 2267042 };
	inline constexpr REL::ID kRegisterMenuHandler{ 827678, 2249387 };
	inline constexpr REL::ID kGetItemCount{ 894081, 2194163 };
	inline constexpr REL::ID kGetQuestItemCount{ 800903, 2194164 };
	inline constexpr REL::ID kShowPipboyLight{ 1304102, 2233203 };
	inline constexpr REL::ID kCrosshairEventSource{ 1231665, 2694517, 4801808 };
}

namespace RE
{
	using TESFormID = std::uint32_t;

	enum class DifficultyLevel : std::int32_t
	{
		kVeryEasy = 0x0,
		kLow = 0x0,
		kEasy = 0x1,
		kNormal = 0x2,
		kHard = 0x3,
		kVeryHard = 0x4,
		kSurvival = 0x5,
		kTrueSurvival = 0x6,
		kHigh = 0x6,
	};

	// ACTOR_VALUE_MODIFIER enumerator aliases: Perm/Temp/Damage also spelled
	// kPermanent/kTemporary/kDamage. Same values, not a second enum.
	inline constexpr auto kPermanentModifier = ACTOR_VALUE_MODIFIER::Perm;
	inline constexpr auto kTemporaryModifier = ACTOR_VALUE_MODIFIER::Temp;
	inline constexpr auto kDamageModifier = ACTOR_VALUE_MODIFIER::Damage;

	// PlayerCharacter::GetDifficultyLevel.
	[[nodiscard]] inline DifficultyLevel GetDifficultyLevel(PlayerCharacter* a_player)
	{
		if (!a_player) {
			return DifficultyLevel::kNormal;
		}
		using func_t = DifficultyLevel (*)(PlayerCharacter*);
		static const auto func = reinterpret_cast<func_t>(
			WIO::Reloc::Address(WIO::CompatIDs::kGetDifficultyLevel));
		return func ? func(a_player) : DifficultyLevel::kNormal;
	}

	// Prefs first, then the base collection: a setting present in both must resolve
	// to the Prefs copy, which is what the game's Options menu writes.
	[[nodiscard]] inline Setting* GetINISetting(std::string_view a_name)
	{
		Setting* setting = nullptr;

		if (const auto iniPrefs = INIPrefSettingCollection::GetSingleton(); iniPrefs) {
			setting = iniPrefs->GetSetting(a_name);
		}
		if (!setting) {
			if (const auto ini = INISettingCollection::GetSingleton(); ini) {
				setting = ini->GetSetting(a_name);
			}
		}
		return setting;
	}

	// Members F4RD does not declare. They cannot be added to its classes from
	// outside, so they are free functions and the call sites differ in shape.

	// BSTimer::GetSingleton.
	//
	// The id names a static BSTimer object, not a pointer to one, so the resolved
	// address is the singleton itself and must not be loaded through. Dereferencing
	// it yields highPrecisionInitTime - a QueryPerformanceCounter reading, which is
	// non-null and so survives every caller's null check, then faults at first use.
	[[nodiscard]] inline BSTimer* GetBSTimer()
	{
		static const auto address = WIO::Reloc::Address(WIO::CompatIDs::kBSTimerSingleton);
		return reinterpret_cast<BSTimer*>(address);
	}

	// ButtonEvent::QPressed and the zero-argument QReleased. F4RD has QJustPressed,
	// QHeldDown(float), QReleased(float) and QAnalogValue, but not these.
	[[nodiscard]] inline bool QPressed(const ButtonEvent& a_event) noexcept
	{
		return a_event.value != 0.0F;
	}

	[[nodiscard]] inline bool QReleased(const ButtonEvent& a_event) noexcept
	{
		return a_event.value == 0.0F && a_event.heldDownSecs > 0.0F;
	}

	// Mouse wheel idCodes. F4RD's BS_BUTTON_CODE covers keyboard VK codes only; the
	// wheel arrives as a sentinel rather than a sequential button index.
	inline constexpr std::int32_t kBSButtonCodeWheelUp = 0x800;
	inline constexpr std::int32_t kBSButtonCodeWheelDown = 0x900;

	// ControlMap::kInvalid.
	//
	// 255, not 0xFFFFFFFF: upstream declares it `kInvalid = static_cast<std::uint8_t>(-1)`
	// inside a `std::uint32_t` enum. The game stores an unbound control as inputKey 255,
	// so widening it makes every existing-but-unbound control read as bound.
	inline constexpr std::uint32_t kInvalidMappedKey = static_cast<std::uint8_t>(-1);

	// ControlMap::GetMappedKey
	[[nodiscard]] inline std::uint32_t GetMappedKey(
		const ControlMap* a_controlMap,
		std::string_view a_eventID,
		INPUT_DEVICE a_device,
		UserEvents::INPUT_CONTEXT_ID a_context = UserEvents::INPUT_CONTEXT_ID::kMainGameplay)
	{
		if (!a_controlMap ||
			a_device >= INPUT_DEVICE::kSupported ||
			a_context >= UserEvents::INPUT_CONTEXT_ID::kTotal) {
			return kInvalidMappedKey;
		}

		const auto context = a_controlMap->controlMaps[stl::to_underlying(a_context)];
		if (!context) {
			return kInvalidMappedKey;
		}

		const BSFixedString eventID(a_eventID);
		for (const auto& mapping : context->deviceMappings[stl::to_underlying(a_device)]) {
			if (mapping.eventID == eventID) {
				return static_cast<std::uint32_t>(mapping.inputKey);
			}
		}
		return kInvalidMappedKey;
	}

	// MeleeThrowHandler. F4RD forward-declares it as a struct
	// (PlayerControls::meleeThrowHandler at 0x240) but never defines it.
	struct MeleeThrowHandler :
		public HeldStateHandler
	{
		static constexpr auto RTTI{ RTTI::MeleeThrowHandler };

		bool buttonHoldDebounce;  // 28
		bool pressRegistered;     // 29
		bool queueThrow;          // 2A
	};
	static_assert(sizeof(MeleeThrowHandler) == 0x30);

	// TESLoadGameEvent. F4RD forward-declares it but never defines it.
	struct TESLoadGameEvent
	{
		[[nodiscard]] static BSTEventSource<TESLoadGameEvent>* GetEventSource()
		{
			using func_t = BSTEventSource<TESLoadGameEvent>* (*)();
			static const auto func = reinterpret_cast<func_t>(
				WIO::Reloc::Address(WIO::CompatIDs::kLoadGameEventSource));
			return func ? func() : nullptr;
		}
	};
	static_assert(sizeof(TESLoadGameEvent) == 0x1);

	// PipboyManager::QPipboyActive. F4RD exposes the underlying value source
	// (BSTValueEventSource<IsPipboyActiveEvent> pipboyActive) but no accessor.
	[[nodiscard]] inline bool QPipboyActive(const PipboyManager* a_manager)
	{
		if (!a_manager) {
			return false;
		}
		const auto& value = a_manager->pipboyActive.optionalValue;
		return value.has_value() && *value;
	}

	// TESForm::GetFormTypeString. F4RD declares GetFormEnumString but not this.
	[[nodiscard]] inline const char* GetFormTypeString(ENUM_FORM_ID a_formType)
	{
		return TESForm::GetFormEnumString()[stl::to_underlying(a_formType)].formString;
	}

	[[nodiscard]] inline const char* GetFormTypeString(const TESForm* a_form)
	{
		return a_form ? GetFormTypeString(a_form->GetFormType()) : nullptr;
	}

	// TESObjectREFR::GetDisplayFullName.
	[[nodiscard]] inline const char* GetDisplayFullName(TESObjectREFR* a_ref)
	{
		if (!a_ref) {
			return nullptr;
		}
		using func_t = const char* (*)(TESObjectREFR*);
		static const auto func = reinterpret_cast<func_t>(
			WIO::Reloc::Address(WIO::CompatIDs::kGetDisplayFullName));
		return func ? func(a_ref) : nullptr;
	}

	// FavoritesManager::UseQuickkeyItem.
	inline bool UseQuickkeyItem(FavoritesManager* a_mgr, std::uint32_t a_quickkeyIndex)
	{
		if (!a_mgr) {
			return false;
		}
		using func_t = bool (*)(FavoritesManager*, std::uint32_t);
		static const auto func = reinterpret_cast<func_t>(
			WIO::Reloc::Address(WIO::CompatIDs::kUseQuickkeyItem));
		return func && func(a_mgr, a_quickkeyIndex);
	}

	// ReadyWeaponHandler. F4RD forward-declares it (PlayerControls::readyWeaponHandler at
	// 0x1F8) but never defines it. Declared struct upstream, so this completes that type.
	// Virtuals come from BSInputEventUser via PlayerInputHandler.
	struct ReadyWeaponHandler :
		public PlayerInputHandler
	{
		static constexpr auto RTTI{ RTTI::ReadyWeaponHandler };

		bool actionTaken{ false };  // 20
	};
	static_assert(sizeof(ReadyWeaponHandler) == 0x28);

	// TESObjectWEAP::IsMeleeWeapon / IsThrownWeapon.
	[[nodiscard]] inline bool IsMeleeWeapon(const TESObjectWEAP* a_weap)
	{
		return a_weap && a_weap->weaponData.type.any(
			WEAPON_TYPE::kOneHandSword, WEAPON_TYPE::kOneHandDagger,
			WEAPON_TYPE::kOneHandAxe, WEAPON_TYPE::kOneHandMace,
			WEAPON_TYPE::kTwoHandSword, WEAPON_TYPE::kTwoHandAxe);
	}

	[[nodiscard]] inline bool IsThrownWeapon(const TESObjectWEAP* a_weap)
	{
		return a_weap && a_weap->weaponData.type.any(
			WEAPON_TYPE::kGrenade, WEAPON_TYPE::kMine);
	}

	// BSAudioManager. F4RD declares neither the class nor its methods.
	class BSAudioManager
	{
	public:
		// Unlike the BSTimer id, this one names a pointer variable, so it is loaded
		// through.
		[[nodiscard]] static BSAudioManager* GetSingleton()
		{
			static const auto address = WIO::Reloc::Address(WIO::CompatIDs::kAudioManagerSingleton);
			return address ? *reinterpret_cast<BSAudioManager**>(address) : nullptr;
		}

		bool GetSoundHandle(BSSoundHandle& a_handle, const BSISoundDescriptor* a_descriptor,
			float a_distance, std::uint32_t a_usageFlags, void* a_data = nullptr)
		{
			using func_t = bool (*)(BSAudioManager*, BSSoundHandle&, const BSISoundDescriptor*,
				float, std::uint32_t, void*);
			static const auto func = reinterpret_cast<func_t>(
				WIO::Reloc::Address(WIO::CompatIDs::kGetSoundHandle));
			return func && func(this, a_handle, a_descriptor, a_distance, a_usageFlags, a_data);
		}
	};

	// BSSoundHandle::Play. F4RD declares the struct's fields but none of its methods.
	// Named PlaySoundHandle because <mmsystem.h> defines PlaySound as a macro.
	inline bool PlaySoundHandle(BSSoundHandle& a_handle)
	{
		using func_t = bool (*)(BSSoundHandle*);
		static const auto func = reinterpret_cast<func_t>(
			WIO::Reloc::Address(WIO::CompatIDs::kPlaySoundHandle));
		return func && func(std::addressof(a_handle));
	}

	// MenuControls::RegisterHandler.
	inline void RegisterMenuHandler(MenuControls* a_controls, BSInputEventUser* a_handler)
	{
		if (!a_controls || !a_handler) {
			return;
		}
		using func_t = void (*)(MenuControls*, BSInputEventUser*);
		static const auto func = reinterpret_cast<func_t>(
			WIO::Reloc::Address(WIO::CompatIDs::kRegisterMenuHandler));
		if (func) {
			func(a_controls, a_handler);
		}
	}

	// BGSInventoryList::GetItemCount / GetQuestItemCount.
	[[nodiscard]] inline std::uint32_t GetItemCount(const BGSInventoryList* a_list, TESBoundObject* a_object)
	{
		if (!a_list || !a_object) {
			return 0;
		}
		using func_t = std::uint32_t (*)(const BGSInventoryList*, TESBoundObject*);
		static const auto func = reinterpret_cast<func_t>(
			WIO::Reloc::Address(WIO::CompatIDs::kGetItemCount));
		return func ? func(a_list, a_object) : 0;
	}

	[[nodiscard]] inline std::uint32_t GetQuestItemCount(const BGSInventoryList* a_list, TESBoundObject* a_object)
	{
		if (!a_list || !a_object) {
			return 0;
		}
		using func_t = std::uint32_t (*)(const BGSInventoryList*, TESBoundObject*);
		static const auto func = reinterpret_cast<func_t>(
			WIO::Reloc::Address(WIO::CompatIDs::kGetQuestItemCount));
		return func ? func(a_list, a_object) : 0;
	}

	// DialogueMenu is not declared upstream; only its name is needed.
	inline constexpr std::string_view kDialogueMenuName{ "DialogueMenu" };

	// PlayerCharacter::ShowPipboyLight.
	inline void ShowPipboyLight(PlayerCharacter* a_player, bool a_show, bool a_skipEffects)
	{
		if (!a_player) {
			return;
		}
		using func_t = void (*)(PlayerCharacter*, bool, bool);
		static const auto func = reinterpret_cast<func_t>(
			WIO::Reloc::Address(WIO::CompatIDs::kShowPipboyLight));
		if (func) {
			func(a_player, a_show, a_skipEffects);
		}
	}

	// BSScaleformManager::GetTranslator. F4RD's StateBag has only the non-template
	// GetStateAddRef(StateType), so the cast is explicit.
	[[nodiscard]] inline BSScaleformTranslator* GetScaleformTranslator(const BSScaleformManager* a_manager)
	{
		if (!a_manager || !a_manager->loader) {
			return nullptr;
		}
		return static_cast<BSScaleformTranslator*>(
			a_manager->loader->GetStateAddRef(Scaleform::GFx::State::StateType::kTranslator));
	}

	// CrosshairMode stays incomplete: no enumerator names have been recovered, so
	// consumers read BSTValueEvent<CrosshairMode>::optionalValue as an integer.
	enum class CrosshairMode;

	class PlayerCrosshairModeEvent :
		public BSTValueEvent<CrosshairMode>
	{
	private:
		using EventSource_t = BSTGlobalEvent::EventSource<PlayerCrosshairModeEvent>;

	public:
		[[nodiscard]] static EventSource_t* GetEventSource()
		{
			static const auto address = WIO::Reloc::Address(WIO::CompatIDs::kCrosshairEventSource);
			if (!address) {
				return nullptr;
			}
			// A pointer variable, like the BSAudioManager singleton and unlike the BSTimer
			// one: loaded through, and written back when the engine has not made it yet.
			auto* const singleton = reinterpret_cast<EventSource_t**>(address);
			if (!*singleton) {
				*singleton = new EventSource_t(&BSTGlobalEvent::GetSingleton()->eventSourceSDMKiller);
			}
			return *singleton;
		}
	};
	static_assert(sizeof(PlayerCrosshairModeEvent) == 0x08);
}
