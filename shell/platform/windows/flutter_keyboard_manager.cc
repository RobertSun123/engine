// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/windows/flutter_keyboard_manager.h"

#include <assert.h>
#include <windows.h>

#include <chrono>
#include <codecvt>
#include <iostream>
#include <string>

#include "flutter/shell/platform/windows/string_conversion.h"

namespace flutter {

namespace {}  // namespace

/**
 * The code prefix for keys which do not have a Unicode representation.
 *
 * This is used by platform-specific code to generate Flutter key codes using
 * HID Usage codes.
 */
constexpr uint64_t kHidPlane = 0x00100000000;

/**
 * The code prefix for keys which have a Unicode representation.
 *
 * This is used by platform-specific code to generate Flutter key codes.
 */
constexpr uint64_t kUnicodePlane = 0x00000000000;

/**
 */
constexpr uint64_t kWindowsKeyIdPlane = 0x00700000000;

/**
 * Mask for the auto-generated bit portion of the key code.
 *
 * This is used by platform-specific code to generate new Flutter key codes
 * for keys which are not recognized.
 */
constexpr uint64_t kAutogeneratedMask = 0x10000000000;

FlutterKeyboardManager::FlutterKeyboardManager(
    std::function<void(const FlutterKeyEvent&)> onEvent)
    : onEvent_(onEvent) {}

FlutterKeyboardManager::~FlutterKeyboardManager() = default;

void FlutterKeyboardManager::TextHook(FlutterWindowsView* view,
                                      const std::u16string& code_point) {}

static bool isAsciiPrintable(int codeUnit) {
  return codeUnit <= 0x7f && codeUnit >= 0x20;
}

static bool isControlCharacter(int codeUnit) {
  return (codeUnit <= 0x1f && codeUnit >= 0x00) ||
         (codeUnit >= 0x7f && codeUnit <= 0x9f);
}

// Transform scancodes sent by windows to scancodes written in Chromium spec.
static uint16_t normalizeScancode(int windowsScanCode) {
  // Windows scancode is composed of 1 bit of "extended" and 8 bits of code,
  // concatenated (by Flutter embedding), i.e. PageUp is represented as 0x149.
  // In Chromium spec the extended bit is shown as 0xe000 bit, making PageUp
  // 0xe049.
  return (windowsScanCode & 0xff) | ((windowsScanCode & 0x100) ? 0xe000 : 0);
}

uint64_t FlutterKeyboardManager::getPhysicalKey(int scancode) {
  int chromiumScancode = normalizeScancode(scancode);
  auto resultIt = windowsToPhysicalMap_.find(chromiumScancode);
  if (resultIt != windowsToPhysicalMap_.end())
    return resultIt->second;
  return scancode | kHidPlane;
}

uint64_t FlutterKeyboardManager::getLogicalKey(int key, int scancode) {
  // Normally logical keys should only be derived from key codes, but since some
  // key codes are either 0 or ambiguous (multiple keys using the same key
  // code), these keys are resolved by scan codes.
  auto numpadIt = scanCodeToLogicalMap_.find(normalizeScancode(scancode));
  if (numpadIt != scanCodeToLogicalMap_.cend())
    return numpadIt->second;

  // Check if the keyCode is one we know about and have a mapping for.
  auto logicalIt = windowsToLogicalMap_.find(key);
  if (logicalIt != windowsToLogicalMap_.cend())
    return logicalIt->second;

  // Upper case letters should be normalized into lower case letters.
  if (isAsciiPrintable(key)) {
    if (isupper(key)) {
      return tolower(key);
    }
    return key;
  }

  // For keys that do not exist in the map, if it has a non-control-character
  // label, then construct a new Unicode-based key from it. Don't mark it as
  // autogenerated, since the label uniquely identifies an ID from the Unicode
  // plane.
  if (!isControlCharacter(key)) {
    return key | kUnicodePlane;
  } else {
    // This is a non-printable key that we don't know about, so we mint a new
    // code with the autogenerated bit set.
    return key | kWindowsKeyIdPlane | kAutogeneratedMask;
  }
}

void FlutterKeyboardManager::cacheUtf8String(char32_t character) {
  if (character == 0) {
    character_cache_[0] = '\0';
    return;
  }
  // TODO: Correctly handle UTF-32
  std::wstring text({static_cast<wchar_t>(character)});
  strcpy_s(character_cache_, kCharacterCacheSize, Utf8FromUtf16(text).c_str());
}

void FlutterKeyboardManager::KeyboardHook(FlutterWindowsView* view,
                                          int key,
                                          int scancode,
                                          int action,
                                          char32_t character,
                                          bool was_down) {
  const uint64_t physical_key = getPhysicalKey(scancode);
  const uint64_t logical_key = getLogicalKey(key, scancode);
  assert(action == WM_KEYDOWN || action == WM_KEYUP);
  const bool is_physical_down = action == WM_KEYDOWN;

  auto last_logical_record_iter = pressingRecords_.find(physical_key);
  const bool had_record = last_logical_record_iter != pressingRecords_.end();
  const uint64_t last_logical_record =
      had_record ? last_logical_record_iter->second : 0;

  // The resulting event's `change`.
  FlutterKeyEventKind change;
  // The resulting event's `logical_key`.
  uint64_t result_logical_key;
  // The next value of pressingRecords_[physical_key] (or to remove it).
  uint64_t next_logical_record;
  bool next_has_record = true;

  if (is_physical_down) {
    if (had_record) {
      if (was_down) {
        // A normal repeated key.
        change = kFlutterKeyEventKindRepeat;
        assert(had_record);
        cacheUtf8String(character);
        next_logical_record = last_logical_record;
        result_logical_key = last_logical_record;
      } else {
        // A non-repeated key has been pressed that has the exact physical key
        // as a currently pressed one, usually indicating multiple keyboards are
        // pressing keys with the same physical key, or the up event was lost
        // during a loss of focus. The down event is ignored.
        return;
      }
    } else {
      // A normal down event (whether the system event is a repeat or not).
      change = kFlutterKeyEventKindDown;
      assert(!had_record);
      cacheUtf8String(character);
      next_logical_record = logical_key;
      result_logical_key = logical_key;
    }
  } else {  // isPhysicalDown is false
    if (last_logical_record == 0) {
      // The physical key has been released before. It indicates multiple
      // keyboards pressed keys with the same physical key. Ignore the up event.
      return;
    } else {
      // A normal up event.
      change = kFlutterKeyEventKindUp;
      assert(had_record);
      // Up events never have character.
      character_cache_[0] = '\0';
      next_has_record = false;
      result_logical_key = last_logical_record;
    }
  }

  if (next_has_record) {
    pressingRecords_[physical_key] = next_logical_record;
  } else {
    pressingRecords_.erase(last_logical_record_iter);
  }

  if (result_logical_key == VK_PROCESSKEY) {
    // VK_PROCESSKEY means that the key press is used by an IME. These key
    // presses are considered handled and not sent to Flutter. These events must
    // be filtered by result_logical_key because the key up event of such
    // presses uses the "original" logical key.
    return;
  }

  FlutterKeyEvent key_data = {};
  key_data.struct_size = sizeof(FlutterKeyEvent);
  key_data.timestamp =
      std::chrono::duration_cast<std::chrono::microseconds>(
          std::chrono::high_resolution_clock::now().time_since_epoch())
          .count();
  key_data.kind = change;
  key_data.physical = physical_key;
  key_data.logical = result_logical_key;
  key_data.character = character_cache_;
  key_data.synthesized = false;
  onEvent_(key_data);
}

}  // namespace flutter
