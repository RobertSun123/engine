// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "key_mapping.h"

#include <glib.h>
#include <map>

#include "flutter/shell/platform/linux/fl_key_embedder_responder_private.h"

// DO NOT EDIT -- DO NOT EDIT -- DO NOT EDIT
// This file is generated by
// flutter/flutter@dev/tools/gen_keycodes/bin/gen_keycodes.dart and should not
// be edited directly.
//
// Edit the template dev/tools/gen_keycodes/data/gtk_key_mapping_cc.tmpl
// instead. See dev/tools/gen_keycodes/README.md for more information.

// Insert a new entry into a hashtable from uint64 to uint64.
//
// Returns whether the newly added value was already in the hash table or not.
static bool insert_record(GHashTable* table, guint64 xkb, guint64 fl_key) {
  return g_hash_table_insert(table, uint64_to_gpointer(xkb),
                             uint64_to_gpointer(fl_key));
}

void initialize_xkb_to_physical_key(GHashTable* table) {
  insert_record(table, 0x00000009, 0x00070029);  // escape
  insert_record(table, 0x0000000a, 0x0007001e);  // digit1
  insert_record(table, 0x0000000b, 0x0007001f);  // digit2
  insert_record(table, 0x0000000c, 0x00070020);  // digit3
  insert_record(table, 0x0000000d, 0x00070021);  // digit4
  insert_record(table, 0x0000000e, 0x00070022);  // digit5
  insert_record(table, 0x0000000f, 0x00070023);  // digit6
  insert_record(table, 0x00000010, 0x00070024);  // digit7
  insert_record(table, 0x00000011, 0x00070025);  // digit8
  insert_record(table, 0x00000012, 0x00070026);  // digit9
  insert_record(table, 0x00000013, 0x00070027);  // digit0
  insert_record(table, 0x00000014, 0x0007002d);  // minus
  insert_record(table, 0x00000015, 0x0007002e);  // equal
  insert_record(table, 0x00000016, 0x0007002a);  // backspace
  insert_record(table, 0x00000017, 0x0007002b);  // tab
  insert_record(table, 0x00000018, 0x00070014);  // keyQ
  insert_record(table, 0x00000019, 0x0007001a);  // keyW
  insert_record(table, 0x0000001a, 0x00070008);  // keyE
  insert_record(table, 0x0000001b, 0x00070015);  // keyR
  insert_record(table, 0x0000001c, 0x00070017);  // keyT
  insert_record(table, 0x0000001d, 0x0007001c);  // keyY
  insert_record(table, 0x0000001e, 0x00070018);  // keyU
  insert_record(table, 0x0000001f, 0x0007000c);  // keyI
  insert_record(table, 0x00000020, 0x00070012);  // keyO
  insert_record(table, 0x00000021, 0x00070013);  // keyP
  insert_record(table, 0x00000022, 0x0007002f);  // bracketLeft
  insert_record(table, 0x00000023, 0x00070030);  // bracketRight
  insert_record(table, 0x00000024, 0x00070028);  // enter
  insert_record(table, 0x00000025, 0x000700e0);  // controlLeft
  insert_record(table, 0x00000026, 0x00070004);  // keyA
  insert_record(table, 0x00000027, 0x00070016);  // keyS
  insert_record(table, 0x00000028, 0x00070007);  // keyD
  insert_record(table, 0x00000029, 0x00070009);  // keyF
  insert_record(table, 0x0000002a, 0x0007000a);  // keyG
  insert_record(table, 0x0000002b, 0x0007000b);  // keyH
  insert_record(table, 0x0000002c, 0x0007000d);  // keyJ
  insert_record(table, 0x0000002d, 0x0007000e);  // keyK
  insert_record(table, 0x0000002e, 0x0007000f);  // keyL
  insert_record(table, 0x0000002f, 0x00070033);  // semicolon
  insert_record(table, 0x00000030, 0x00070034);  // quote
  insert_record(table, 0x00000031, 0x00070035);  // backquote
  insert_record(table, 0x00000032, 0x000700e1);  // shiftLeft
  insert_record(table, 0x00000033, 0x00070031);  // backslash
  insert_record(table, 0x00000034, 0x0007001d);  // keyZ
  insert_record(table, 0x00000035, 0x0007001b);  // keyX
  insert_record(table, 0x00000036, 0x00070006);  // keyC
  insert_record(table, 0x00000037, 0x00070019);  // keyV
  insert_record(table, 0x00000038, 0x00070005);  // keyB
  insert_record(table, 0x00000039, 0x00070011);  // keyN
  insert_record(table, 0x0000003a, 0x00070010);  // keyM
  insert_record(table, 0x0000003b, 0x00070036);  // comma
  insert_record(table, 0x0000003c, 0x00070037);  // period
  insert_record(table, 0x0000003d, 0x00070038);  // slash
  insert_record(table, 0x0000003e, 0x000700e5);  // shiftRight
  insert_record(table, 0x0000003f, 0x00070055);  // numpadMultiply
  insert_record(table, 0x00000040, 0x000700e2);  // altLeft
  insert_record(table, 0x00000041, 0x0007002c);  // space
  insert_record(table, 0x00000042, 0x00070039);  // capsLock
  insert_record(table, 0x00000043, 0x0007003a);  // f1
  insert_record(table, 0x00000044, 0x0007003b);  // f2
  insert_record(table, 0x00000045, 0x0007003c);  // f3
  insert_record(table, 0x00000046, 0x0007003d);  // f4
  insert_record(table, 0x00000047, 0x0007003e);  // f5
  insert_record(table, 0x00000048, 0x0007003f);  // f6
  insert_record(table, 0x00000049, 0x00070040);  // f7
  insert_record(table, 0x0000004a, 0x00070041);  // f8
  insert_record(table, 0x0000004b, 0x00070042);  // f9
  insert_record(table, 0x0000004c, 0x00070043);  // f10
  insert_record(table, 0x0000004d, 0x00070053);  // numLock
  insert_record(table, 0x0000004e, 0x00070047);  // scrollLock
  insert_record(table, 0x0000004f, 0x0007005f);  // numpad7
  insert_record(table, 0x00000050, 0x00070060);  // numpad8
  insert_record(table, 0x00000051, 0x00070061);  // numpad9
  insert_record(table, 0x00000052, 0x00070056);  // numpadSubtract
  insert_record(table, 0x00000053, 0x0007005c);  // numpad4
  insert_record(table, 0x00000054, 0x0007005d);  // numpad5
  insert_record(table, 0x00000055, 0x0007005e);  // numpad6
  insert_record(table, 0x00000056, 0x00070057);  // numpadAdd
  insert_record(table, 0x00000057, 0x00070059);  // numpad1
  insert_record(table, 0x00000058, 0x0007005a);  // numpad2
  insert_record(table, 0x00000059, 0x0007005b);  // numpad3
  insert_record(table, 0x0000005a, 0x00070062);  // numpad0
  insert_record(table, 0x0000005b, 0x00070063);  // numpadDecimal
  insert_record(table, 0x0000005d, 0x00070094);  // lang5
  insert_record(table, 0x0000005e, 0x00070064);  // intlBackslash
  insert_record(table, 0x0000005f, 0x00070044);  // f11
  insert_record(table, 0x00000060, 0x00070045);  // f12
  insert_record(table, 0x00000061, 0x00070087);  // intlRo
  insert_record(table, 0x00000062, 0x00070092);  // lang3
  insert_record(table, 0x00000063, 0x00070093);  // lang4
  insert_record(table, 0x00000064, 0x0007008a);  // convert
  insert_record(table, 0x00000065, 0x00070088);  // kanaMode
  insert_record(table, 0x00000066, 0x0007008b);  // nonConvert
  insert_record(table, 0x00000068, 0x00070058);  // numpadEnter
  insert_record(table, 0x00000069, 0x000700e4);  // controlRight
  insert_record(table, 0x0000006a, 0x00070054);  // numpadDivide
  insert_record(table, 0x0000006b, 0x00070046);  // printScreen
  insert_record(table, 0x0000006c, 0x000700e6);  // altRight
  insert_record(table, 0x0000006e, 0x0007004a);  // home
  insert_record(table, 0x0000006f, 0x00070052);  // arrowUp
  insert_record(table, 0x00000070, 0x0007004b);  // pageUp
  insert_record(table, 0x00000071, 0x00070050);  // arrowLeft
  insert_record(table, 0x00000072, 0x0007004f);  // arrowRight
  insert_record(table, 0x00000073, 0x0007004d);  // end
  insert_record(table, 0x00000074, 0x00070051);  // arrowDown
  insert_record(table, 0x00000075, 0x0007004e);  // pageDown
  insert_record(table, 0x00000076, 0x00070049);  // insert
  insert_record(table, 0x00000077, 0x0007004c);  // delete
  insert_record(table, 0x00000079, 0x0007007f);  // audioVolumeMute
  insert_record(table, 0x0000007a, 0x00070081);  // audioVolumeDown
  insert_record(table, 0x0000007b, 0x00070080);  // audioVolumeUp
  insert_record(table, 0x0000007c, 0x00070066);  // power
  insert_record(table, 0x0000007d, 0x00070067);  // numpadEqual
  insert_record(table, 0x0000007e, 0x000700d7);  // numpadSignChange
  insert_record(table, 0x0000007f, 0x00070048);  // pause
  insert_record(table, 0x00000080, 0x000c029f);  // showAllWindows
  insert_record(table, 0x00000081, 0x00070085);  // numpadComma
  insert_record(table, 0x00000082, 0x00070090);  // lang1
  insert_record(table, 0x00000083, 0x00070091);  // lang2
  insert_record(table, 0x00000084, 0x00070089);  // intlYen
  insert_record(table, 0x00000085, 0x000700e3);  // metaLeft
  insert_record(table, 0x00000086, 0x000700e7);  // metaRight
  insert_record(table, 0x00000087, 0x00070065);  // contextMenu
  insert_record(table, 0x00000088, 0x000c0226);  // browserStop
  insert_record(table, 0x00000089, 0x00070079);  // again
  insert_record(table, 0x0000008b, 0x0007007a);  // undo
  insert_record(table, 0x0000008c, 0x00070077);  // select
  insert_record(table, 0x0000008d, 0x0007007c);  // copy
  insert_record(table, 0x0000008e, 0x00070074);  // open
  insert_record(table, 0x0000008f, 0x0007007d);  // paste
  insert_record(table, 0x00000090, 0x0007007e);  // find
  insert_record(table, 0x00000091, 0x0007007b);  // cut
  insert_record(table, 0x00000092, 0x00070075);  // help
  insert_record(table, 0x00000094, 0x000c0192);  // launchApp2
  insert_record(table, 0x00000096, 0x00010082);  // sleep
  insert_record(table, 0x00000097, 0x00010083);  // wakeUp
  insert_record(table, 0x00000098, 0x000c0194);  // launchApp1
  insert_record(table, 0x0000009e, 0x000c0196);  // launchInternetBrowser
  insert_record(table, 0x000000a0, 0x000c019e);  // lockScreen
  insert_record(table, 0x000000a3, 0x000c018a);  // launchMail
  insert_record(table, 0x000000a4, 0x000c022a);  // browserFavorites
  insert_record(table, 0x000000a6, 0x000c0224);  // browserBack
  insert_record(table, 0x000000a7, 0x000c0225);  // browserForward
  insert_record(table, 0x000000a9, 0x000c00b8);  // eject
  insert_record(table, 0x000000ab, 0x000c00b5);  // mediaTrackNext
  insert_record(table, 0x000000ac, 0x000c00cd);  // mediaPlayPause
  insert_record(table, 0x000000ad, 0x000c00b6);  // mediaTrackPrevious
  insert_record(table, 0x000000ae, 0x000c00b7);  // mediaStop
  insert_record(table, 0x000000af, 0x000c00b2);  // mediaRecord
  insert_record(table, 0x000000b0, 0x000c00b4);  // mediaRewind
  insert_record(table, 0x000000b1, 0x000c008c);  // launchPhone
  insert_record(table, 0x000000b3, 0x000c0183);  // mediaSelect
  insert_record(table, 0x000000b4, 0x000c0223);  // browserHome
  insert_record(table, 0x000000b5, 0x000c0227);  // browserRefresh
  insert_record(table, 0x000000b6, 0x000c0094);  // exit
  insert_record(table, 0x000000bb, 0x000700b6);  // numpadParenLeft
  insert_record(table, 0x000000bc, 0x000700b7);  // numpadParenRight
  insert_record(table, 0x000000bd, 0x000c0201);  // newKey
  insert_record(table, 0x000000be, 0x000c0279);  // redo
  insert_record(table, 0x000000bf, 0x00070068);  // f13
  insert_record(table, 0x000000c0, 0x00070069);  // f14
  insert_record(table, 0x000000c1, 0x0007006a);  // f15
  insert_record(table, 0x000000c2, 0x0007006b);  // f16
  insert_record(table, 0x000000c3, 0x0007006c);  // f17
  insert_record(table, 0x000000c4, 0x0007006d);  // f18
  insert_record(table, 0x000000c5, 0x0007006e);  // f19
  insert_record(table, 0x000000c6, 0x0007006f);  // f20
  insert_record(table, 0x000000c7, 0x00070070);  // f21
  insert_record(table, 0x000000c8, 0x00070071);  // f22
  insert_record(table, 0x000000c9, 0x00070072);  // f23
  insert_record(table, 0x000000ca, 0x00070073);  // f24
  insert_record(table, 0x000000d1, 0x000c00b1);  // mediaPause
  insert_record(table, 0x000000d6, 0x000c0203);  // close
  insert_record(table, 0x000000d7, 0x000c00b0);  // mediaPlay
  insert_record(table, 0x000000d8, 0x000c00b3);  // mediaFastForward
  insert_record(table, 0x000000d9, 0x000c00e5);  // bassBoost
  insert_record(table, 0x000000da, 0x000c0208);  // print
  insert_record(table, 0x000000e1, 0x000c0221);  // browserSearch
  insert_record(table, 0x000000e8, 0x000c0070);  // brightnessDown
  insert_record(table, 0x000000e9, 0x000c006f);  // brightnessUp
  insert_record(table, 0x000000eb, 0x000100b5);  // displayToggleIntExt
  insert_record(table, 0x000000ed, 0x000c007a);  // kbdIllumDown
  insert_record(table, 0x000000ee, 0x000c0079);  // kbdIllumUp
  insert_record(table, 0x000000ef, 0x000c028c);  // mailSend
  insert_record(table, 0x000000f0, 0x000c0289);  // mailReply
  insert_record(table, 0x000000f1, 0x000c028b);  // mailForward
  insert_record(table, 0x000000f2, 0x000c0207);  // save
  insert_record(table, 0x000000f3, 0x000c01a7);  // launchDocuments
  insert_record(table, 0x000000fc, 0x000c0075);  // brightnessAuto
  insert_record(table, 0x0000016e, 0x000c0060);  // info
  insert_record(table, 0x00000172, 0x000c008d);  // programGuide
  insert_record(table, 0x0000017a, 0x000c0061);  // closedCaptionToggle
  insert_record(table, 0x0000017c, 0x000c0232);  // zoomToggle
  insert_record(table, 0x0000017e, 0x000c01ae);  // launchKeyboardLayout
  insert_record(table, 0x00000190, 0x000c01b7);  // launchAudioBrowser
  insert_record(table, 0x00000195, 0x000c018e);  // launchCalendar
  insert_record(table, 0x0000019d, 0x000c0083);  // mediaLast
  insert_record(table, 0x000001a2, 0x000c009c);  // channelUp
  insert_record(table, 0x000001a3, 0x000c009d);  // channelDown
  insert_record(table, 0x000001aa, 0x000c022d);  // zoomIn
  insert_record(table, 0x000001ab, 0x000c022e);  // zoomOut
  insert_record(table, 0x000001ad, 0x000c0184);  // launchWordProcessor
  insert_record(table, 0x000001af, 0x000c0186);  // launchSpreadsheet
  insert_record(table, 0x000001b5, 0x000c018d);  // launchContacts
  insert_record(table, 0x000001b7, 0x000c0072);  // brightnessToggle
  insert_record(table, 0x000001b8, 0x000c01ab);  // spellCheck
  insert_record(table, 0x000001b9, 0x000c019c);  // logOff
  insert_record(table, 0x0000024b, 0x000c019f);  // launchControlPanel
  insert_record(table, 0x0000024c, 0x000c01a2);  // selectTask
  insert_record(table, 0x0000024d, 0x000c01b1);  // launchScreenSaver
  insert_record(table, 0x0000024e, 0x000c00cf);  // speechInputToggle
  insert_record(table, 0x0000024f, 0x000c01cb);  // launchAssistant
  insert_record(table, 0x00000250, 0x000c029d);  // keyboardLayoutSelect
  insert_record(table, 0x00000258, 0x000c0073);  // brightnessMinimum
  insert_record(table, 0x00000259, 0x000c0074);  // brightnessMaximum
  insert_record(table, 0x00000281, 0x00000017);  // privacyScreenToggle
}

void initialize_gtk_keyval_to_logical_key(GHashTable* table) {
  insert_record(table, 0x000000a5, 0x01100070089);  // yen
  insert_record(table, 0x0000fd06, 0x01000000405);  // 3270_EraseEOF
  insert_record(table, 0x0000fd0e, 0x01000000503);  // 3270_Attn
  insert_record(table, 0x0000fd15, 0x01000000402);  // 3270_Copy
  insert_record(table, 0x0000fd16, 0x01000000d2f);  // 3270_Play
  insert_record(table, 0x0000fd1b, 0x01000000406);  // 3270_ExSelect
  insert_record(table, 0x0000fd1d, 0x01000000608);  // 3270_PrintScreen
  insert_record(table, 0x0000fd1e, 0x0100000000d);  // 3270_Enter
  insert_record(table, 0x0000fe03, 0x40000000102);  // ISO_Level3_Shift
  insert_record(table, 0x0000fe08, 0x01000000709);  // ISO_Next_Group
  insert_record(table, 0x0000fe0a, 0x0100000070a);  // ISO_Prev_Group
  insert_record(table, 0x0000fe0c, 0x01000000707);  // ISO_First_Group
  insert_record(table, 0x0000fe0e, 0x01000000708);  // ISO_Last_Group
  insert_record(table, 0x0000fe20, 0x01000000009);  // ISO_Left_Tab
  insert_record(table, 0x0000fe34, 0x0100000000d);  // ISO_Enter
  insert_record(table, 0x0000ff08, 0x01000000008);  // BackSpace
  insert_record(table, 0x0000ff09, 0x01000000009);  // Tab
  insert_record(table, 0x0000ff0b, 0x01000000401);  // Clear
  insert_record(table, 0x0000ff0d, 0x0100000000d);  // Return
  insert_record(table, 0x0000ff13, 0x01000000509);  // Pause
  insert_record(table, 0x0000ff14, 0x0100000010c);  // Scroll_Lock
  insert_record(table, 0x0000ff1b, 0x0100000001b);  // Escape
  insert_record(table, 0x0000ff21, 0x01000000719);  // Kanji
  insert_record(table, 0x0000ff24, 0x0100000071b);  // Romaji
  insert_record(table, 0x0000ff25, 0x01000000716);  // Hiragana
  insert_record(table, 0x0000ff26, 0x0100000071a);  // Katakana
  insert_record(table, 0x0000ff27, 0x01000000717);  // Hiragana_Katakana
  insert_record(table, 0x0000ff28, 0x0100000071c);  // Zenkaku
  insert_record(table, 0x0000ff29, 0x01000000715);  // Hankaku
  insert_record(table, 0x0000ff2a, 0x0100000071d);  // Zenkaku_Hankaku
  insert_record(table, 0x0000ff2f, 0x01000000714);  // Eisu_Shift
  insert_record(table, 0x0000ff31, 0x01000000711);  // Hangul
  insert_record(table, 0x0000ff34, 0x01000000712);  // Hangul_Hanja
  insert_record(table, 0x0000ff37, 0x01000000703);  // Codeinput
  insert_record(table, 0x0000ff3c, 0x01000000710);  // SingleCandidate
  insert_record(table, 0x0000ff3e, 0x0100000070e);  // PreviousCandidate
  insert_record(table, 0x0000ff50, 0x01000000306);  // Home
  insert_record(table, 0x0000ff51, 0x01000000302);  // Left
  insert_record(table, 0x0000ff52, 0x01000000304);  // Up
  insert_record(table, 0x0000ff53, 0x01000000303);  // Right
  insert_record(table, 0x0000ff54, 0x01000000301);  // Down
  insert_record(table, 0x0000ff55, 0x01000000308);  // Page_Up
  insert_record(table, 0x0000ff56, 0x01000000307);  // Page_Down
  insert_record(table, 0x0000ff57, 0x01000000305);  // End
  insert_record(table, 0x0000ff60, 0x0100000050c);  // Select
  insert_record(table, 0x0000ff61, 0x01000000a0c);  // Print
  insert_record(table, 0x0000ff62, 0x01000000506);  // Execute
  insert_record(table, 0x0000ff63, 0x01000000407);  // Insert
  insert_record(table, 0x0000ff65, 0x0100000040a);  // Undo
  insert_record(table, 0x0000ff66, 0x01000000409);  // Redo
  insert_record(table, 0x0000ff67, 0x01000000505);  // Menu
  insert_record(table, 0x0000ff68, 0x01000000507);  // Find
  insert_record(table, 0x0000ff69, 0x01000000504);  // Cancel
  insert_record(table, 0x0000ff6a, 0x01000000508);  // Help
  insert_record(table, 0x0000ff7e, 0x0100000070b);  // Mode_switch
  insert_record(table, 0x0000ff7f, 0x0100000010a);  // Num_Lock
  insert_record(table, 0x0000ff80, 0x00000000020);  // KP_Space
  insert_record(table, 0x0000ff89, 0x01000000009);  // KP_Tab
  insert_record(table, 0x0000ff8d, 0x5000000000d);  // KP_Enter
  insert_record(table, 0x0000ff91, 0x01000000801);  // KP_F1
  insert_record(table, 0x0000ff92, 0x01000000802);  // KP_F2
  insert_record(table, 0x0000ff93, 0x01000000803);  // KP_F3
  insert_record(table, 0x0000ff94, 0x01000000804);  // KP_F4
  insert_record(table, 0x0000ff95, 0x50000000037);  // KP_Home
  insert_record(table, 0x0000ff96, 0x50000000034);  // KP_Left
  insert_record(table, 0x0000ff97, 0x50000000038);  // KP_Up
  insert_record(table, 0x0000ff98, 0x50000000036);  // KP_Right
  insert_record(table, 0x0000ff99, 0x50000000032);  // KP_Down
  insert_record(table, 0x0000ff9a, 0x50000000039);  // KP_Page_Up
  insert_record(table, 0x0000ff9b, 0x50000000033);  // KP_Page_Down
  insert_record(table, 0x0000ff9c, 0x50000000031);  // KP_End
  insert_record(table, 0x0000ff9e, 0x50000000030);  // KP_Insert
  insert_record(table, 0x0000ff9f, 0x5000000002e);  // KP_Delete
  insert_record(table, 0x0000ffaa, 0x5000000002a);  // KP_Multiply
  insert_record(table, 0x0000ffab, 0x5000000002b);  // KP_Add
  insert_record(table, 0x0000ffad, 0x5000000002d);  // KP_Subtract
  insert_record(table, 0x0000ffae, 0x0000000002e);  // KP_Decimal
  insert_record(table, 0x0000ffaf, 0x5000000002f);  // KP_Divide
  insert_record(table, 0x0000ffb0, 0x50000000030);  // KP_0
  insert_record(table, 0x0000ffb1, 0x50000000031);  // KP_1
  insert_record(table, 0x0000ffb2, 0x50000000032);  // KP_2
  insert_record(table, 0x0000ffb3, 0x50000000033);  // KP_3
  insert_record(table, 0x0000ffb4, 0x50000000034);  // KP_4
  insert_record(table, 0x0000ffb5, 0x50000000035);  // KP_5
  insert_record(table, 0x0000ffb6, 0x50000000036);  // KP_6
  insert_record(table, 0x0000ffb7, 0x50000000037);  // KP_7
  insert_record(table, 0x0000ffb8, 0x50000000038);  // KP_8
  insert_record(table, 0x0000ffb9, 0x50000000039);  // KP_9
  insert_record(table, 0x0000ffbd, 0x5000000003d);  // KP_Equal
  insert_record(table, 0x0000ffbe, 0x01000000801);  // F1
  insert_record(table, 0x0000ffbf, 0x01000000802);  // F2
  insert_record(table, 0x0000ffc0, 0x01000000803);  // F3
  insert_record(table, 0x0000ffc1, 0x01000000804);  // F4
  insert_record(table, 0x0000ffc2, 0x01000000805);  // F5
  insert_record(table, 0x0000ffc3, 0x01000000806);  // F6
  insert_record(table, 0x0000ffc4, 0x01000000807);  // F7
  insert_record(table, 0x0000ffc5, 0x01000000808);  // F8
  insert_record(table, 0x0000ffc6, 0x01000000809);  // F9
  insert_record(table, 0x0000ffc7, 0x0100000080a);  // F10
  insert_record(table, 0x0000ffc8, 0x0100000080b);  // F11
  insert_record(table, 0x0000ffc9, 0x0100000080c);  // F12
  insert_record(table, 0x0000ffca, 0x0100000080d);  // F13
  insert_record(table, 0x0000ffcb, 0x0100000080e);  // F14
  insert_record(table, 0x0000ffcc, 0x0100000080f);  // F15
  insert_record(table, 0x0000ffcd, 0x01000000810);  // F16
  insert_record(table, 0x0000ffce, 0x01000000811);  // F17
  insert_record(table, 0x0000ffcf, 0x01000000812);  // F18
  insert_record(table, 0x0000ffd0, 0x01000000813);  // F19
  insert_record(table, 0x0000ffd1, 0x01000000814);  // F20
  insert_record(table, 0x0000ffd2, 0x01000000815);  // F21
  insert_record(table, 0x0000ffd3, 0x01000000816);  // F22
  insert_record(table, 0x0000ffd4, 0x01000000817);  // F23
  insert_record(table, 0x0000ffd5, 0x01000000818);  // F24
  insert_record(table, 0x0000ffe1, 0x3000000010d);  // Shift_L
  insert_record(table, 0x0000ffe2, 0x4000000010d);  // Shift_R
  insert_record(table, 0x0000ffe3, 0x30000000105);  // Control_L
  insert_record(table, 0x0000ffe4, 0x40000000105);  // Control_R
  insert_record(table, 0x0000ffe5, 0x01000000104);  // Caps_Lock
  insert_record(table, 0x0000ffe7, 0x30000000109);  // Meta_L
  insert_record(table, 0x0000ffe8, 0x40000000109);  // Meta_R
  insert_record(table, 0x0000ffe9, 0x30000000102);  // Alt_L
  insert_record(table, 0x0000ffea, 0x40000000102);  // Alt_R
  insert_record(table, 0x0000ffeb, 0x0100000010e);  // Super_L
  insert_record(table, 0x0000ffec, 0x0100000010e);  // Super_R
  insert_record(table, 0x0000ffed, 0x01000000108);  // Hyper_L
  insert_record(table, 0x0000ffee, 0x01000000108);  // Hyper_R
  insert_record(table, 0x0000ffff, 0x0100000007f);  // Delete
  insert_record(table, 0x1008ff02, 0x01000000602);  // MonBrightnessUp
  insert_record(table, 0x1008ff03, 0x01000000601);  // MonBrightnessDown
  insert_record(table, 0x1008ff10, 0x0100000060a);  // Standby
  insert_record(table, 0x1008ff11, 0x01000000a0f);  // AudioLowerVolume
  insert_record(table, 0x1008ff12, 0x01000000a11);  // AudioMute
  insert_record(table, 0x1008ff13, 0x01000000a10);  // AudioRaiseVolume
  insert_record(table, 0x1008ff14, 0x01000000d2f);  // AudioPlay
  insert_record(table, 0x1008ff15, 0x01000000a07);  // AudioStop
  insert_record(table, 0x1008ff16, 0x01000000a09);  // AudioPrev
  insert_record(table, 0x1008ff17, 0x01000000a08);  // AudioNext
  insert_record(table, 0x1008ff18, 0x01000000c04);  // HomePage
  insert_record(table, 0x1008ff19, 0x01000000b03);  // Mail
  insert_record(table, 0x1008ff1b, 0x01000000c06);  // Search
  insert_record(table, 0x1008ff1c, 0x01000000d30);  // AudioRecord
  insert_record(table, 0x1008ff20, 0x01000000b02);  // Calendar
  insert_record(table, 0x1008ff26, 0x01000000c01);  // Back
  insert_record(table, 0x1008ff27, 0x01000000c03);  // Forward
  insert_record(table, 0x1008ff28, 0x01000000c07);  // Stop
  insert_record(table, 0x1008ff29, 0x01000000c05);  // Refresh
  insert_record(table, 0x1008ff2a, 0x01000000607);  // PowerOff
  insert_record(table, 0x1008ff2b, 0x0100000060b);  // WakeUp
  insert_record(table, 0x1008ff2c, 0x01000000604);  // Eject
  insert_record(table, 0x1008ff2d, 0x01000000b07);  // ScreenSaver
  insert_record(table, 0x1008ff2f, 0x01100010082);  // Sleep
  insert_record(table, 0x1008ff30, 0x01000000c02);  // Favorites
  insert_record(table, 0x1008ff31, 0x01000000d2e);  // AudioPause
  insert_record(table, 0x1008ff3e, 0x01000000d31);  // AudioRewind
  insert_record(table, 0x1008ff56, 0x01000000a01);  // Close
  insert_record(table, 0x1008ff57, 0x01000000402);  // Copy
  insert_record(table, 0x1008ff58, 0x01000000404);  // Cut
  insert_record(table, 0x1008ff61, 0x01000000605);  // LogOff
  insert_record(table, 0x1008ff68, 0x01000000a0a);  // New
  insert_record(table, 0x1008ff6b, 0x01000000a0b);  // Open
  insert_record(table, 0x1008ff6d, 0x01000000408);  // Paste
  insert_record(table, 0x1008ff6e, 0x01000000b0d);  // Phone
  insert_record(table, 0x1008ff72, 0x01000000a03);  // Reply
  insert_record(table, 0x1008ff77, 0x01000000a0d);  // Save
  insert_record(table, 0x1008ff7b, 0x01000000a04);  // Send
  insert_record(table, 0x1008ff7c, 0x01000000a0e);  // Spell
  insert_record(table, 0x1008ff8b, 0x0100000050d);  // ZoomIn
  insert_record(table, 0x1008ff8c, 0x0100000050e);  // ZoomOut
  insert_record(table, 0x1008ff90, 0x01000000a02);  // MailForward
  insert_record(table, 0x1008ff97, 0x01000000d2c);  // AudioForward
  insert_record(table, 0x1008ffa7, 0x01100000014);  // Suspend
}

void initialize_modifier_bit_to_checked_keys(GHashTable* table) {
  FlKeyEmbedderCheckedKey* data;

  data = g_new(FlKeyEmbedderCheckedKey, 1);
  g_hash_table_insert(table, GUINT_TO_POINTER(GDK_SHIFT_MASK), data);
  data->is_caps_lock = false;
  data->primary_logical_key = 0x3000000010d;   // shiftLeft
  data->primary_physical_key = 0x0000700e1;    // shiftLeft
  data->secondary_physical_key = 0x0000700e5;  // shiftRight

  data = g_new(FlKeyEmbedderCheckedKey, 1);
  g_hash_table_insert(table, GUINT_TO_POINTER(GDK_CONTROL_MASK), data);
  data->is_caps_lock = false;
  data->primary_logical_key = 0x30000000105;   // controlLeft
  data->primary_physical_key = 0x0000700e0;    // controlLeft
  data->secondary_physical_key = 0x0000700e4;  // controlRight

  data = g_new(FlKeyEmbedderCheckedKey, 1);
  g_hash_table_insert(table, GUINT_TO_POINTER(GDK_MOD1_MASK), data);
  data->is_caps_lock = false;
  data->primary_logical_key = 0x30000000102;   // altLeft
  data->primary_physical_key = 0x0000700e2;    // altLeft
  data->secondary_physical_key = 0x0000700e6;  // altRight

  data = g_new(FlKeyEmbedderCheckedKey, 1);
  g_hash_table_insert(table, GUINT_TO_POINTER(GDK_META_MASK), data);
  data->is_caps_lock = false;
  data->primary_logical_key = 0x30000000109;   // metaLeft
  data->primary_physical_key = 0x0000700e3;    // metaLeft
  data->secondary_physical_key = 0x0000700e7;  // metaRight
}

void initialize_lock_bit_to_checked_keys(GHashTable* table) {
  FlKeyEmbedderCheckedKey* data;

  data = g_new(FlKeyEmbedderCheckedKey, 1);
  g_hash_table_insert(table, GUINT_TO_POINTER(GDK_LOCK_MASK), data);
  data->is_caps_lock = true;
  data->primary_logical_key = 0x01000000104;  // capsLock
  data->primary_physical_key = 0x000070039;   // capsLock

  data = g_new(FlKeyEmbedderCheckedKey, 1);
  g_hash_table_insert(table, GUINT_TO_POINTER(GDK_MOD2_MASK), data);
  data->is_caps_lock = false;
  data->primary_logical_key = 0x0100000010a;  // numLock
  data->primary_physical_key = 0x000070053;   // numLock
}
