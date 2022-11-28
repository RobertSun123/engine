// Copyright 2014 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'dart:convert' show utf8;

// The following segment is not only used in the generating script, but also
// copied to the generated package.
/*@@@ SHARED SEGMENT START @@@*/

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
  'KeyA': 'A',
  'KeyB': 'B',
  'KeyC': 'C',
  'KeyD': 'D',
  'KeyE': 'E',
  'KeyF': 'F',
  'KeyG': 'G',
  'KeyH': 'H',
  'KeyI': 'I',
  'KeyJ': 'J',
  'KeyK': 'K',
  'KeyL': 'L',
  'KeyM': 'M',
  'KeyN': 'N',
  'KeyO': 'O',
  'KeyP': 'P',
  'KeyQ': 'Q',
  'KeyR': 'R',
  'KeyS': 'S',
  'KeyT': 'T',
  'KeyU': 'U',
  'KeyV': 'V',
  'KeyW': 'W',
  'KeyX': 'X',
  'KeyY': 'Y',
  'KeyZ': 'Z',
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

  String readString() {
    final int length = readIntAsVerbatim();
    if (length == 0) {
      return '';
    }
    final String result = _data.substring(_offset, _offset + length);
    _offset += length;
    return result;
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
      yield MapEntry<String, Map<String, int>>(stream.readString(), _unmarshallCodeMap(stream));
    }
  })());
}

/*@@@ SHARED SEGMENT END @@@*/

/// Whether the given charCode is a ASCII letter.
bool isLetterChar(int charCode) {
  return (charCode >= _kLowerA && charCode <= _kLowerZ)
      || (charCode >= _kUpperA && charCode <= _kUpperZ);
}

bool _isPrintableEascii(int charCode) {
  return charCode >= 0x20 && charCode <= 0xFF;
}

typedef _ForEachAction<V> = void Function(String key, V value);
void _sortedForEach<V>(Map<String, V> map, _ForEachAction<V> action) {
  map
    .entries
    .toList()
    ..sort((MapEntry<String, V> a, MapEntry<String, V> b) => a.key.compareTo(b.key))
    ..forEach((MapEntry<String, V> entry) {
      action(entry.key, entry.value);
    });
}

// Encode a small integer as a character by its value.
//
// For example, 0x48 is encoded as '0'. This means that values within 0x0 - 0x19
// or greater than 0xFF are forbidden.
void _marshallIntAsChar(StringBuffer builder, int value) {
  assert(_isPrintableEascii(value), '$value');
  builder.writeCharCode(value);
}

const int _kMarshallIntEnd = 0xFF; // The last printable EASCII.
// Encode a small integer as a character based on a certain printable codepoint.
//
// For example, 0x0 is encoded as '0', and 0x1 is encoded as '1'. This function
// allows smaller values than _marshallIntAsChar.
void _marshallIntAsVerbatim(StringBuffer builder, int value) {
  final int result = value + _kMarshallIntBase;
  assert(result <= _kMarshallIntEnd);
  builder.writeCharCode(result);
}

// Encode a string, length first, then contents.
//
// The length of string is the amount of characters, not its byte length.
void _marshallString(StringBuffer builder, String value) {
  _marshallIntAsVerbatim(builder, value.length);
  for (final int char in utf8.encode(value)) {
    assert(_isPrintableEascii(char), '0x${char.toRadixString(16)}}');
  }
  builder.write(value);
}

void _marshallEventKey(StringBuffer builder, String value) {
  if (value == _kEventKeyDead) {
    builder.write(_kUseDead);
  } else {
    assert(value.length == 1);
    assert(value != _kUseDead);
    builder.write(value);
  }
}

/// Encode a key mapping data into a string.
///
/// The algorithm aims at encoding the map directly into a printable string
/// (instead of a binary stream converted by base64). Some characters in the
/// string can be multi-byte, which means the decoder should parse the string
/// using substr instead of as a binary stream.
String marshallMappingData(Map<String, Map<String, int>> mappingData) {
  final StringBuffer builder = StringBuffer();
  _marshallIntAsVerbatim(builder, mappingData.length);
  _sortedForEach(mappingData, (String eventCode, Map<String, int> codeMap) {
    _marshallString(builder, eventCode);
    _marshallIntAsVerbatim(builder, codeMap.length);
    _sortedForEach(codeMap, (String eventKey, int logicalKey) {
      _marshallEventKey(builder, eventKey);
      _marshallIntAsChar(builder, logicalKey);
    });
  });
  return builder.toString();
}
