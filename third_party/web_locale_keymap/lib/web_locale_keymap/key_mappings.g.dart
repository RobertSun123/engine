//---------------------------------------------------------------------------------------------
//  Copyright (c) 2022 Google LLC
//  Licensed under the MIT License. See License.txt in the project root for license information.
//--------------------------------------------------------------------------------------------*/

// DO NOT EDIT -- DO NOT EDIT -- DO NOT EDIT
//
// This file is auto generated by flutter/engine:flutter/tools/gen_web_keyboard_layouts based on
// https://github.com/microsoft/vscode/tree/ab7ccc9e872dfcdfc429f8f2815109ec0ca926e3/src/vs/workbench/services/keybinding/browser/keyboardLayouts
//
// Edit the following files instead:
//
//  - Script: lib/main.dart
//  - Templates: data/*.tmpl
//
// See flutter/engine:flutter/tools/gen_web_keyboard_layouts/README.md for more information.

/// Used in the final mapping indicating the logical key should be derived from
/// KeyboardEvent.keyCode.
///
/// This value is chosen because it's a printable character within EASCII that
/// will never be mapped to (checked in the marshalling algorithm).
const int kUseKeyCode = 0xFF;

/// Used in the final mapping indicating the event key is 'Dead', the dead key.
final String _kUseDead = String.fromCharCode(0xFE);

/// The KeyboardEvent.key for a dead key.
const String _kEventKeyDead = 'Dead';

/// A map of all goals from the scan codes to their mapped value in US layout.
const Map<String, String> kLayoutGoals = <String, String>{
  'KeyA': 'a',
  'KeyB': 'b',
  'KeyC': 'c',
  'KeyD': 'd',
  'KeyE': 'e',
  'KeyF': 'f',
  'KeyG': 'g',
  'KeyH': 'h',
  'KeyI': 'i',
  'KeyJ': 'j',
  'KeyK': 'k',
  'KeyL': 'l',
  'KeyM': 'm',
  'KeyN': 'n',
  'KeyO': 'o',
  'KeyP': 'p',
  'KeyQ': 'q',
  'KeyR': 'r',
  'KeyS': 's',
  'KeyT': 't',
  'KeyU': 'u',
  'KeyV': 'v',
  'KeyW': 'w',
  'KeyX': 'x',
  'KeyY': 'y',
  'KeyZ': 'z',
  'Digit1': '1',
  'Digit2': '2',
  'Digit3': '3',
  'Digit4': '4',
  'Digit5': '5',
  'Digit6': '6',
  'Digit7': '7',
  'Digit8': '8',
  'Digit9': '9',
  'Digit0': '0',
  'Minus': '-',
  'Equal': '=',
  'BracketLeft': '[',
  'BracketRight': ']',
  'Backslash': r'\',
  'Semicolon': ';',
  'Quote': "'",
  'Backquote': '`',
  'Comma': ',',
  'Period': '.',
  'Slash': '/',
};

final int _kLowerA = 'a'.codeUnitAt(0);
final int _kUpperA = 'A'.codeUnitAt(0);
final int _kLowerZ = 'z'.codeUnitAt(0);
final int _kUpperZ = 'Z'.codeUnitAt(0);
final int _k0 = '0'.codeUnitAt(0);
final int _k9 = '9'.codeUnitAt(0);

bool _isAscii(String key) {
  if (key.length != 1) {
    return false;
  }
  // 0x20 is the first printable character in ASCII.
  return key.codeUnitAt(0) >= 0x20 && key.codeUnitAt(0) <= 0x7F;
}

/// Returns whether the `char` is a single character of a letter or a digit.
bool isAlnum(String char) {
  if (char.length != 1) {
    return false;
  }
  final int charCode = char.codeUnitAt(0);
  return (charCode >= _kLowerA && charCode <= _kLowerZ)
      || (charCode >= _kUpperA && charCode <= _kUpperZ)
      || (charCode >= _k0 && charCode <= _k9);
}

/// A set of rules that can derive a large number of logical keys simply from
/// the event's code and key.
///
/// This greatly reduces the entries needed in the final mapping.
int? heuristicMapper(String code, String key) {
  if (isAlnum(key)) {
    return key.toLowerCase().codeUnitAt(0);
  }
  if (!_isAscii(key)) {
    return kLayoutGoals[code]!.codeUnitAt(0);
  }
  return null;
}

// Maps an integer to a printable EASCII character by adding it to this value.
//
// We could've chosen 0x20, the first printable character, for a slightly bigger
// range, but it's prettier this way and sufficient.
final int _kMarshallIntBase = '0'.codeUnitAt(0);

class _StringStream {
  _StringStream(this._data) : _offset = 0;

  final String _data;
  final Map<int, String> _goalToEventCode = Map<int, String>.fromEntries(
    kLayoutGoals
      .entries
      .map((MapEntry<String, String> beforeEntry) =>
          MapEntry<int, String>(beforeEntry.value.codeUnitAt(0), beforeEntry.key))
  );

  int get offest => _offset;
  int _offset;

  int readIntAsVerbatim() {
    final int result = _data.codeUnitAt(_offset);
    _offset += 1;
    assert(result >= _kMarshallIntBase);
    return result - _kMarshallIntBase;
  }

  int readIntAsChar() {
    final int result = _data.codeUnitAt(_offset);
    _offset += 1;
    return result;
  }

  String readEventKey() {
    final String char = String.fromCharCode(readIntAsChar());
    if (char == _kUseDead) {
      return _kEventKeyDead;
    } else {
      return char;
    }
  }

  String readEventCode() {
    final int charCode = _data.codeUnitAt(_offset);
    _offset += 1;
    return _goalToEventCode[charCode]!;
  }
}

Map<String, int> _unmarshallCodeMap(_StringStream stream) {
  final int entryNum = stream.readIntAsVerbatim();
  return Map<String, int>.fromEntries((() sync* {
    for (int entryIndex = 0; entryIndex < entryNum; entryIndex += 1) {
      yield MapEntry<String, int>(stream.readEventKey(), stream.readIntAsChar());
    }
  })());
}

/// Decode a key mapping data out of the string.
Map<String, Map<String, int>> unmarshallMappingData(String compressed) {
  final _StringStream stream = _StringStream(compressed);
  final int eventCodeNum = stream.readIntAsVerbatim();
  return Map<String, Map<String, int>>.fromEntries((() sync* {
    for (int eventCodeIndex = 0; eventCodeIndex < eventCodeNum; eventCodeIndex += 1) {
      yield MapEntry<String, Map<String, int>>(stream.readEventCode(), _unmarshallCodeMap(stream));
    }
  })());
}

/// Data for [LocaleKeymap] on Windows.
///
/// Do not use this value, but [LocaleKeymap.win] instead.
///
/// The keys are `KeyboardEvent.code` and then `KeyboardEvent.key`. The values
/// are logical keys or [kUseKeyCode]. Entries that can be derived using
/// heuristics have been omitted.
Map<String, Map<String, int>> getMappingDataWin() {
  return unmarshallMappingData(
    r'F'
    r'`1§0'
    r'04)0=0@0}0'
    r"17!1&1'1+1>1|1~1"
    '25"2\'2/2@2~2'
    r'36"3#3*3+3-3^3'
    r"47!4$4'4+4;4{4~4"
    r'53%5(5[5'
    r'66&6-6/6:6^6|6'
    r'77&7/7=7?7`7{7|7'
    r'86!8(8*8[8\8_8'
    r'95(9)9]9^9{9'
    r'b1{b'
    r'c1&c'
    r'f1[f'
    r'g1]g'
    r'm2<m?m'
    r'n1}n'
    r'q3/q@q\q'
    r'v1@v'
    r'w3"w?w|w'
    r'x2#x)x'
    r'z2(z>y'
  ); // 195 characters
}

/// Data for [LocaleKeymap] on Linux.
///
/// Do not use this value, but [LocaleKeymap.linux] instead.
///
/// The keys are `KeyboardEvent.code` and then `KeyboardEvent.key`. The values
/// are logical keys or [kUseKeyCode]. Entries that can be derived using
/// heuristics have been omitted.
Map<String, Map<String, int>> getMappingDataLinux() {
  return unmarshallMappingData(
    r'B'
    r'04)0=0@0}0'
    r'13!1&1|1'
    r'23"2@2~2'
    r'32"3#3'
    r"45$4'4;4{4~4"
    r'53%5(5[5'
    r'65&6-6:6^6|6'
    r'75&7/7?7`7{7'
    r'85(8*8[8\8_8'
    r'94(9)9]9^9'
    r'a2@qΩq'
    r'k1&k'
    r'q3@qÆaæa'
    r'w2<z«z'
    r'x1>x'
    r'y2¥ÿ←ÿ'
    r'z5<z»yŁwłw›y'
    r';2µmºm'
  ); // 151 characters
}

/// Data for [LocaleKeymap] on Darwin.
///
/// Do not use this value, but [LocaleKeymap.darwin] instead.
///
/// The keys are `KeyboardEvent.code` and then `KeyboardEvent.key`. The values
/// are logical keys or [kUseKeyCode]. Entries that can be derived using
/// heuristics have been omitted.
Map<String, Map<String, int>> getMappingDataDarwin() {
  return unmarshallMappingData(
    r'W'
    r',2„w∑w'
    r'04)0=0`0}0'
    r'13!1&1|1'
    r'22"2@2'
    r'32"3#3'
    r"43$4%4'4"
    r'56%5(5:5[5{5~5'
    r'65 6&6,6]6^6'
    r'75&7.7/7\7|7'
    r'86!8(8*8;8[8{8'
    r"97 9'9(9)9]9{9}9"
    r'a2Ωq‡q'
    r'b2˛x≈x'
    r'c3 cÔj∆j'
    r'd2þe´e'
    r'f2þu¨u'
    r'g2þÿˆi'
    r'h3 hÎÿ∂d'
    r'i3 iÇcçc'
    r'j2Óh˙h'
    r'k2ˇÿ†t'
    r'l5 l@lþÿ|l˜n'
    r'm1~m'
    r'n3 nıÿ∫b'
    r'o2®r‰r'
    r'p2¬lÒl'
    r'q2Æaæa'
    r'r3 rπp∏p'
    r's3 sØoøo'
    r't2¥yÁy'
    r'u3 u©g˝g'
    r'v2˚kk'
    r'w2ÂzÅz'
    r'x2Œqœq'
    r'y5 yÏfƒfˇzΩz'
    r'z5 z¥y‡y‹ÿ›w'
    r'.2√v◊v'
    r';4µmÍsÓmßs'
    r'/2¸zΩz'
  ); // 315 characters
}
