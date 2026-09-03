#pragma once

// Include from a mod's pch.h after <RE/Fallout.h> and <F4SE/F4SE.h>:
//
//     #include <WattzIO/Compat.h>

#include <F4SE/F4SE.h>
#include <RE/Fallout.h>

#include "WattzIO/Logging.h"       // REX::INFO / DEBUG / WARN / ERROR, UNRESTRICTED_CAST
#include "WattzIO/Relocation.h"    // REL::write_fill, REL::replace_func
#include "WattzIO/InputMap.h"      // F4SE::InputMap
#include "WattzIO/REMissing.h"     // RE:: declarations F4RD lacks
#include "WattzIO/Plugin.h"        // F4SE_PLUGIN_LOAD, WIO_PLUGIN_VERSION, WIO::Init
#include "WattzIO/Translations.h"  // WIO::Translations::LoadFile / LoadForMod / Localize
#include "WattzIO/Papyrus.h"       // WIO::Papyrus::DispatchStaticCall / DispatchMethodCall
#include "WattzIO/Settings.h"      // REX::TIniSetting, REX::FIniSettingStore, WIO::Ini::GetBool
