#pragma once

// F4SE::InputMap. CommonLibF4RD does not provide it. The unified 0-281 keycode
// numbering it defines is also what MCM's Keybinds.json stores.
//
// Layout:
//   0-255    keyboard, Windows VK codes
//   256-263  mouse buttons
//   264-265  mouse wheel up/down
//   266-281  gamepad: DPad Up/Down/Left/Right, Start, Back, LThumb, RThumb,
//            LB, RB, A, B, X, Y, LT, RT

#include <RE/Fallout.h>

#include <cstdint>

namespace F4SE
{
	namespace InputMap
	{
		enum
		{
			// 256 keyboard, then 8 mouse buttons, then 2 wheel directions, then 16 gamepad
			kMacro_KeyboardOffset = 0,
			kMacro_NumKeyboardKeys = 256,

			kMacro_MouseButtonOffset = kMacro_NumKeyboardKeys,  // 256
			kMacro_NumMouseButtons = 8,

			kMacro_MouseWheelOffset = kMacro_MouseButtonOffset + kMacro_NumMouseButtons,  // 264
			kMacro_MouseWheelDirections = 2,

			kMacro_GamepadOffset = kMacro_MouseWheelOffset + kMacro_MouseWheelDirections,  // 266
			kMacro_NumGamepadButtons = 16,

			kMaxMacros = kMacro_GamepadOffset + kMacro_NumGamepadButtons  // 282
		};

		enum
		{
			kGamepadButtonOffset_DPAD_UP = kMacro_GamepadOffset,  // 266
			kGamepadButtonOffset_DPAD_DOWN,
			kGamepadButtonOffset_DPAD_LEFT,
			kGamepadButtonOffset_DPAD_RIGHT,
			kGamepadButtonOffset_START,
			kGamepadButtonOffset_BACK,
			kGamepadButtonOffset_LEFT_THUMB,
			kGamepadButtonOffset_RIGHT_THUMB,
			kGamepadButtonOffset_LEFT_SHOULDER,
			kGamepadButtonOffset_RIGHT_SHOULDER,
			kGamepadButtonOffset_A,
			kGamepadButtonOffset_B,
			kGamepadButtonOffset_X,
			kGamepadButtonOffset_Y,
			kGamepadButtonOffset_LT,
			kGamepadButtonOffset_RT  // 281
		};

		namespace detail
		{
			// XInput button masks (xinput.h)
			inline constexpr std::uint32_t XI_DPAD_UP = 0x0001;
			inline constexpr std::uint32_t XI_DPAD_DOWN = 0x0002;
			inline constexpr std::uint32_t XI_DPAD_LEFT = 0x0004;
			inline constexpr std::uint32_t XI_DPAD_RIGHT = 0x0008;
			inline constexpr std::uint32_t XI_START = 0x0010;
			inline constexpr std::uint32_t XI_BACK = 0x0020;
			inline constexpr std::uint32_t XI_LEFT_THUMB = 0x0040;
			inline constexpr std::uint32_t XI_RIGHT_THUMB = 0x0080;
			inline constexpr std::uint32_t XI_LEFT_SHOULDER = 0x0100;
			inline constexpr std::uint32_t XI_RIGHT_SHOULDER = 0x0200;
			inline constexpr std::uint32_t XI_A = 0x1000;
			inline constexpr std::uint32_t XI_B = 0x2000;
			inline constexpr std::uint32_t XI_X = 0x4000;
			inline constexpr std::uint32_t XI_Y = 0x8000;

			// Triggers are not XInput button bits - these are the game's own IDs.
			inline constexpr std::uint32_t GAME_LT = 0x9;
			inline constexpr std::uint32_t GAME_RT = 0xA;

			// ScePad button masks (PS4-style pads, when pcGamePadMapType == kOrbis)
			inline constexpr std::uint32_t SCE_L3 = 0x00000002;
			inline constexpr std::uint32_t SCE_R3 = 0x00000004;
			inline constexpr std::uint32_t SCE_OPTIONS = 0x00000008;
			inline constexpr std::uint32_t SCE_UP = 0x00000010;
			inline constexpr std::uint32_t SCE_RIGHT = 0x00000020;
			inline constexpr std::uint32_t SCE_DOWN = 0x00000040;
			inline constexpr std::uint32_t SCE_LEFT = 0x00000080;
			inline constexpr std::uint32_t SCE_L1 = 0x00000400;
			inline constexpr std::uint32_t SCE_R1 = 0x00000800;
			inline constexpr std::uint32_t SCE_TRIANGLE = 0x00001000;
			inline constexpr std::uint32_t SCE_CIRCLE = 0x00002000;
			inline constexpr std::uint32_t SCE_CROSS = 0x00004000;
			inline constexpr std::uint32_t SCE_SQUARE = 0x00008000;
			inline constexpr std::uint32_t SCE_TOUCH_PAD = 0x00100000;
		}

		[[nodiscard]] inline std::uint32_t XInputToScePadOffset(std::uint32_t a_keyMask)
		{
			using namespace detail;
			switch (a_keyMask) {
			case XI_DPAD_UP:         return SCE_UP;
			case XI_DPAD_DOWN:       return SCE_DOWN;
			case XI_DPAD_LEFT:       return SCE_LEFT;
			case XI_DPAD_RIGHT:      return SCE_RIGHT;
			case XI_START:           return SCE_OPTIONS;
			case XI_BACK:            return SCE_TOUCH_PAD;
			case XI_LEFT_THUMB:      return SCE_L3;
			case XI_RIGHT_THUMB:     return SCE_R3;
			case XI_LEFT_SHOULDER:   return SCE_L1;
			case XI_RIGHT_SHOULDER:  return SCE_R1;
			case XI_A:               return SCE_CROSS;
			case XI_B:               return SCE_CIRCLE;
			case XI_X:               return SCE_SQUARE;
			case XI_Y:               return SCE_TRIANGLE;
			default:                 return a_keyMask;
			}
		}

		[[nodiscard]] inline std::uint32_t ScePadOffsetToXInput(std::uint32_t a_keyMask)
		{
			using namespace detail;
			switch (a_keyMask) {
			case SCE_UP:         return XI_DPAD_UP;
			case SCE_DOWN:       return XI_DPAD_DOWN;
			case SCE_LEFT:       return XI_DPAD_LEFT;
			case SCE_RIGHT:      return XI_DPAD_RIGHT;
			case SCE_OPTIONS:    return XI_START;
			case SCE_TOUCH_PAD:  return XI_BACK;
			case SCE_L3:         return XI_LEFT_THUMB;
			case SCE_R3:         return XI_RIGHT_THUMB;
			case SCE_L1:         return XI_LEFT_SHOULDER;
			case SCE_R1:         return XI_RIGHT_SHOULDER;
			case SCE_CROSS:      return XI_A;
			case SCE_CIRCLE:     return XI_B;
			case SCE_SQUARE:     return XI_X;
			case SCE_TRIANGLE:   return XI_Y;
			default:             return a_keyMask;
			}
		}

		[[nodiscard]] inline std::uint32_t GamepadMaskToKeycode(std::uint32_t a_keyMask)
		{
			using namespace detail;

			if (const auto controlMap = RE::ControlMap::GetSingleton();
				controlMap && controlMap->pcGamePadMapType == RE::PC_GAMEPAD_TYPE::kOrbis) {
				a_keyMask = ScePadOffsetToXInput(a_keyMask);
			}

			switch (a_keyMask) {
			case XI_DPAD_UP:         return kGamepadButtonOffset_DPAD_UP;
			case XI_DPAD_DOWN:       return kGamepadButtonOffset_DPAD_DOWN;
			case XI_DPAD_LEFT:       return kGamepadButtonOffset_DPAD_LEFT;
			case XI_DPAD_RIGHT:      return kGamepadButtonOffset_DPAD_RIGHT;
			case XI_START:           return kGamepadButtonOffset_START;
			case XI_BACK:            return kGamepadButtonOffset_BACK;
			case XI_LEFT_THUMB:      return kGamepadButtonOffset_LEFT_THUMB;
			case XI_RIGHT_THUMB:     return kGamepadButtonOffset_RIGHT_THUMB;
			case XI_LEFT_SHOULDER:   return kGamepadButtonOffset_LEFT_SHOULDER;
			case XI_RIGHT_SHOULDER:  return kGamepadButtonOffset_RIGHT_SHOULDER;
			case XI_A:               return kGamepadButtonOffset_A;
			case XI_B:               return kGamepadButtonOffset_B;
			case XI_X:               return kGamepadButtonOffset_X;
			case XI_Y:               return kGamepadButtonOffset_Y;
			case GAME_LT:            return kGamepadButtonOffset_LT;
			case GAME_RT:            return kGamepadButtonOffset_RT;
			default:                 return kMaxMacros;  // invalid
			}
		}

		[[nodiscard]] inline std::uint32_t GamepadKeycodeToMask(std::uint32_t a_keyCode)
		{
			using namespace detail;

			std::uint32_t mask{};
			switch (a_keyCode) {
			case kGamepadButtonOffset_DPAD_UP:         mask = XI_DPAD_UP; break;
			case kGamepadButtonOffset_DPAD_DOWN:       mask = XI_DPAD_DOWN; break;
			case kGamepadButtonOffset_DPAD_LEFT:       mask = XI_DPAD_LEFT; break;
			case kGamepadButtonOffset_DPAD_RIGHT:      mask = XI_DPAD_RIGHT; break;
			case kGamepadButtonOffset_START:           mask = XI_START; break;
			case kGamepadButtonOffset_BACK:            mask = XI_BACK; break;
			case kGamepadButtonOffset_LEFT_THUMB:      mask = XI_LEFT_THUMB; break;
			case kGamepadButtonOffset_RIGHT_THUMB:     mask = XI_RIGHT_THUMB; break;
			case kGamepadButtonOffset_LEFT_SHOULDER:   mask = XI_LEFT_SHOULDER; break;
			case kGamepadButtonOffset_RIGHT_SHOULDER:  mask = XI_RIGHT_SHOULDER; break;
			case kGamepadButtonOffset_A:               mask = XI_A; break;
			case kGamepadButtonOffset_B:               mask = XI_B; break;
			case kGamepadButtonOffset_X:               mask = XI_X; break;
			case kGamepadButtonOffset_Y:               mask = XI_Y; break;
			case kGamepadButtonOffset_LT:              return GAME_LT;
			case kGamepadButtonOffset_RT:              return GAME_RT;
			default:                                   return 0;
			}

			if (const auto controlMap = RE::ControlMap::GetSingleton();
				controlMap && controlMap->pcGamePadMapType == RE::PC_GAMEPAD_TYPE::kOrbis) {
				mask = XInputToScePadOffset(mask);
			}
			return mask;
		}
	}
}
