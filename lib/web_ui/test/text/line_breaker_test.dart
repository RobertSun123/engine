// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'package:test/bootstrap/browser.dart';
import 'package:test/test.dart';

import 'package:ui/src/engine.dart';
import 'package:ui/ui.dart';

import '../html/paragraph/helper.dart';
import 'line_breaker_test_helper.dart';
import 'line_breaker_test_raw_data.dart';

final String placeholderChar = String.fromCharCode(0xFFFC);

void main() {
  internalBootstrapBrowserTest(() => testMain);
}

void testMain() {
  group('$LineBreakFragmenter', () {
    test('empty string', () {
      expect(split(''), <Line>[
        Line('', endOfText),
      ]);
    });

    test('whitespace', () {
      expect(split('foo bar'), <Line>[
        Line('foo ', opportunity, sp: 1),
        Line('bar', endOfText),
      ]);
      expect(split('  foo    bar  '), <Line>[
        Line('  ', opportunity, sp: 2),
        Line('foo    ', opportunity, sp: 4),
        Line('bar  ', endOfText, sp: 2),
      ]);
    });

    test('single-letter lines', () {
      expect(split('foo a bar'), <Line>[
        Line('foo ', opportunity, sp: 1),
        Line('a ', opportunity, sp: 1),
        Line('bar', endOfText),
      ]);
      expect(split('a b c'), <Line>[
        Line('a ', opportunity, sp: 1),
        Line('b ', opportunity, sp: 1),
        Line('c', endOfText),
      ]);
      expect(split(' a b '), <Line>[
        Line(' ', opportunity, sp: 1),
        Line('a ', opportunity, sp: 1),
        Line('b ', endOfText, sp: 1),
      ]);
    });

    test('new line characters', () {
      final String bk = String.fromCharCode(0x000B);
      // Can't have a line break between CR×LF.
      expect(split('foo\r\nbar'), <Line>[
        Line('foo\r\n', mandatory, nl: 2, sp: 2),
        Line('bar', endOfText),
      ]);

      // Any other new line is considered a line break on its own.

      expect(split('foo\n\nbar'), <Line>[
        Line('foo\n', mandatory, nl: 1, sp: 1),
        Line('\n', mandatory, nl: 1, sp: 1),
        Line('bar', endOfText),
      ]);
      expect(split('foo\r\rbar'), <Line>[
        Line('foo\r', mandatory, nl: 1, sp: 1),
        Line('\r', mandatory, nl: 1, sp: 1),
        Line('bar', endOfText),
      ]);
      expect(split('foo$bk${bk}bar'), <Line>[
        Line('foo$bk', mandatory, nl: 1, sp: 1),
        Line(bk, mandatory, nl: 1, sp: 1),
        Line('bar', endOfText),
      ]);

      expect(split('foo\n\rbar'), <Line>[
        Line('foo\n', mandatory, nl: 1, sp: 1),
        Line('\r', mandatory, nl: 1, sp: 1),
        Line('bar', endOfText),
      ]);
      expect(split('foo$bk\rbar'), <Line>[
        Line('foo$bk', mandatory, nl: 1, sp: 1),
        Line('\r', mandatory, nl: 1, sp: 1),
        Line('bar', endOfText),
      ]);
      expect(split('foo\r${bk}bar'), <Line>[
        Line('foo\r', mandatory, nl: 1, sp: 1),
        Line(bk, mandatory, nl: 1, sp: 1),
        Line('bar', endOfText),
      ]);
      expect(split('foo$bk\nbar'), <Line>[
        Line('foo$bk', mandatory, nl: 1, sp: 1),
        Line('\n', mandatory, nl: 1, sp: 1),
        Line('bar', endOfText),
      ]);
      expect(split('foo\n${bk}bar'), <Line>[
        Line('foo\n', mandatory, nl: 1, sp: 1),
        Line(bk, mandatory, nl: 1, sp: 1),
        Line('bar', endOfText),
      ]);

      // New lines at the beginning and end.

      expect(split('foo\n'), <Line>[
        Line('foo\n', mandatory, nl: 1, sp: 1),
        Line('', endOfText),
      ]);
      expect(split('foo\r'), <Line>[
        Line('foo\r', mandatory, nl: 1, sp: 1),
        Line('', endOfText),
      ]);
      expect(split('foo$bk'), <Line>[
        Line('foo$bk', mandatory, nl: 1, sp: 1),
        Line('', endOfText),
      ]);

      expect(split('\nfoo'), <Line>[
        Line('\n', mandatory, nl: 1, sp: 1),
        Line('foo', endOfText),
      ]);
      expect(split('\rfoo'), <Line>[
        Line('\r', mandatory, nl: 1, sp: 1),
        Line('foo', endOfText),
      ]);
      expect(split('${bk}foo'), <Line>[
        Line(bk, mandatory, nl: 1, sp: 1),
        Line('foo', endOfText),
      ]);

      // Whitespace with new lines.

      expect(split('foo  \n'), <Line>[
        Line('foo  \n', mandatory, nl: 1, sp: 3),
        Line('', endOfText),
      ]);

      expect(split('foo  \n   '), <Line>[
        Line('foo  \n', mandatory, nl: 1, sp: 3),
        Line('   ', endOfText, sp: 3),
      ]);

      expect(split('foo  \n   bar'), <Line>[
        Line('foo  \n', mandatory, nl: 1, sp: 3),
        Line('   ', opportunity, sp: 3),
        Line('bar', endOfText),
      ]);

      expect(split('\n  foo'), <Line>[
        Line('\n', mandatory, nl: 1, sp: 1),
        Line('  ', opportunity, sp: 2),
        Line('foo', endOfText),
      ]);
      expect(split('   \n  foo'), <Line>[
        Line('   \n', mandatory, nl: 1, sp: 4),
        Line('  ', opportunity, sp: 2),
        Line('foo', endOfText),
      ]);
    });

    test('trailing spaces and new lines', () {
      expect(split('foo bar  '), <Line>[
          Line('foo ', opportunity, sp: 1),
          Line('bar  ', endOfText, sp: 2),
        ],
      );

      expect(split('foo  \nbar\nbaz   \n'), <Line>[
          Line('foo  \n', mandatory, nl: 1, sp: 3),
          Line('bar\n', mandatory, nl: 1, sp: 1),
          Line('baz   \n', mandatory, nl: 1, sp: 4),
          Line('', endOfText),
        ],
      );
    });

    test('leading spaces', () {
      expect(split(' foo'), <Line>[
          Line(' ', opportunity, sp: 1),
          Line('foo', endOfText),
        ],
      );

      expect(split('   foo'), <Line>[
          Line('   ', opportunity, sp: 3),
          Line('foo', endOfText),
        ],
      );

      expect(split('  foo   bar'), <Line>[
          Line('  ', opportunity, sp: 2),
          Line('foo   ', opportunity, sp: 3),
          Line('bar', endOfText),
        ],
      );

      expect(split('  \n   foo'), <Line>[
          Line('  \n', mandatory, nl: 1, sp: 3),
          Line('   ', opportunity, sp: 3),
          Line('foo', endOfText),
        ],
      );
    });

    test('whitespace before the last character', () {
      expect(split('Lorem sit .'), <Line>[
          Line('Lorem ', opportunity, sp: 1),
          Line('sit ', opportunity, sp: 1),
          Line('.', endOfText),
        ],
      );
    });

    test('placeholders', () {
      final CanvasParagraph paragraph = rich(
        EngineParagraphStyle(),
        (CanvasParagraphBuilder builder) {
          builder.addPlaceholder(100, 100, PlaceholderAlignment.top);
          builder.addText('Lorem');
          builder.addPlaceholder(100, 100, PlaceholderAlignment.top);
          builder.addText('ipsum\n');
          builder.addPlaceholder(100, 100, PlaceholderAlignment.top);
          builder.addText('dolor');
          builder.addPlaceholder(100, 100, PlaceholderAlignment.top);
          builder.addText('\nsit');
          builder.addPlaceholder(100, 100, PlaceholderAlignment.top);
        },
      );

      final String placeholderChar = String.fromCharCode(0xFFFC);

      expect(splitParagraph(paragraph), <Line>[
        Line(placeholderChar, opportunity),
        Line('Lorem', opportunity),
        Line(placeholderChar, opportunity),
        Line('ipsum\n', mandatory, nl: 1, sp: 1),
        Line(placeholderChar, opportunity),
        Line('dolor', opportunity),
        Line('$placeholderChar\n', mandatory, nl: 1, sp: 1),
        Line('sit', opportunity),
        Line(placeholderChar, endOfText),
      ]);
    });

    test('single placeholder', () {
      final CanvasParagraph paragraph = rich(
        EngineParagraphStyle(),
        (CanvasParagraphBuilder builder) {
          builder.addPlaceholder(100, 100, PlaceholderAlignment.top);
        },
      );

      final String placeholderChar = String.fromCharCode(0xFFFC);

      expect(splitParagraph(paragraph), <Line>[
        Line(placeholderChar, endOfText),
      ]);
    });

    test('placeholders surrounded by spaces and new lines', () {
      final CanvasParagraph paragraph = rich(
        EngineParagraphStyle(),
        (CanvasParagraphBuilder builder) {
          builder.addPlaceholder(100, 100, PlaceholderAlignment.top);
          builder.addText('  Lorem  ');
          builder.addPlaceholder(100, 100, PlaceholderAlignment.top);
          builder.addText('  \nipsum \n');
          builder.addPlaceholder(100, 100, PlaceholderAlignment.top);
          builder.addText('\n');
          builder.addPlaceholder(100, 100, PlaceholderAlignment.top);
        },
      );

      expect(splitParagraph(paragraph), <Line>[
          Line('$placeholderChar  ', opportunity, sp: 2),
          Line('Lorem  ', opportunity, sp: 2),
          Line('$placeholderChar  \n', mandatory, nl: 1, sp: 3),
          Line('ipsum \n', mandatory, nl: 1, sp: 2),
          Line('$placeholderChar\n', mandatory, nl: 1, sp: 1),
          Line(placeholderChar, endOfText),
        ],
      );
    });

    test('surrogates', () {
      expect(split('A\u{1F600}'), <Line>[
          Line('A', opportunity),
          Line('\u{1F600}', endOfText),
        ],
      );

      expect(split('\u{1F600}A'), <Line>[
          Line('\u{1F600}', opportunity),
          Line('A', endOfText),
        ],
      );

      expect(split('\u{1F600}\u{1F600}'), <Line>[
          Line('\u{1F600}', opportunity),
          Line('\u{1F600}', endOfText),
        ],
      );

      expect(split('A \u{1F600} \u{1F600}'), <Line>[
          Line('A ', opportunity, sp: 1),
          Line('\u{1F600} ', opportunity, sp: 1),
          Line('\u{1F600}', endOfText),
        ],
      );
    });

    test('comprehensive test', () {
      final List<TestCase> testCollection =
          parseRawTestData(rawLineBreakTestData);
      for (int t = 0; t < testCollection.length; t++) {
        final TestCase testCase = testCollection[t];

        final String text = testCase.toText();
        final List<LineBreakFragment> fragments = LineBreakFragmenter(text).fragment();

        // `f` is the index in the `fragments` list.
        int f = 0;
        LineBreakFragment currentFragment = fragments[f];

        int surrogateCount = 0;
        // `s` is the index in the `testCase.signs` list.
        for (int s = 0; s < testCase.signs.length - 1; s++) {
          // `i` is the index in the `text`.
          final int i = s + surrogateCount;
          final Sign sign = testCase.signs[s];

          if (sign.isBreakOpportunity) {
            expect(
              currentFragment.end,
              i,
              reason: 'Failed at test case number $t:\n'
                  '$testCase\n'
                  '"$text"\n'
                  '\nExpected fragment to end at {$i} but ended at {${currentFragment.end}}.',
            );
            currentFragment = fragments[++f];
          } else {
            expect(
              currentFragment.end,
              greaterThan(i),
              reason: 'Failed at test case number $t:\n'
                  '$testCase\n'
                  '"$text"\n'
                  '\nFragment ended in early at {${currentFragment.end}}.',
            );
          }

          if (s < testCase.chars.length && testCase.chars[s].isSurrogatePair) {
            surrogateCount++;
          }
        }

        // Now let's look at the last sign, which requires different handling.

        // The last line break is an endOfText (or a hard break followed by
        // endOfText if the last character is a hard line break).
        if (currentFragment.type == mandatory) {
          // When last character is a hard line break, there should be an
          // extra fragment to represent the empty line at the end.
          expect(
            fragments,
            hasLength(f + 2),
            reason: 'Failed at test case number $t:\n'
                '$testCase\n'
                '"$text"\n'
                "\nExpected an extra fragment for endOfText but there wasn't one.",
          );

          currentFragment = fragments[++f];
        }

        expect(
          currentFragment.type,
          endOfText,
          reason: 'Failed at test case number $t:\n'
              '$testCase\n'
              '"$text"\n\n'
              'Expected an endOfText fragment but found: $currentFragment',
        );
        expect(
          currentFragment.end,
          text.length,
          reason: 'Failed at test case number $t:\n'
              '$testCase\n'
              '"$text"\n\n'
              'Expected an endOfText fragment ending at {${text.length}} but found: $currentFragment',
        );
      }
    });
  });
}

/// Holds information about how a line was split from a string.
class Line {
  Line(this.text, this.breakType, {this.nl = 0, this.sp = 0});

  factory Line.fromLineBreakFragment(String text, LineBreakFragment fragment) {
    return Line(
      text.substring(fragment.start, fragment.end),
      fragment.type,
      nl: fragment.trailingNewlines,
      sp: fragment.trailingSpaces,
    );
  }

  final String text;
  final LineBreakType breakType;
  final int nl;
  final int sp;

  @override
  int get hashCode => Object.hash(text, breakType, nl, sp);

  @override
  bool operator ==(Object other) {
    return other is Line &&
        other.text == text &&
        other.breakType == breakType &&
        other.nl == nl &&
        other.sp == sp;
  }

  String get escapedText {
    final String bk = String.fromCharCode(0x000B);
    final String nl = String.fromCharCode(0x0085);
    return text
        .replaceAll('"', r'\"')
        .replaceAll('\n', r'\n')
        .replaceAll('\r', r'\r')
        .replaceAll(bk, '{BK}')
        .replaceAll(nl, '{NL}');
  }

  @override
  String toString() {
    return '"$escapedText" ($breakType, nl: $nl, sp: $sp)';
  }
}

List<Line> split(String text) {
  return <Line>[
    for (final LineBreakFragment fragment in LineBreakFragmenter(text).fragment())
      Line.fromLineBreakFragment(text, fragment)
  ];
}

List<Line> splitParagraph(CanvasParagraph paragraph) {
  return <Line>[
    for (final LineBreakFragment fragment in LineBreakFragmenter(paragraph.plainText).fragment())
      Line.fromLineBreakFragment(paragraph.toPlainText(), fragment)
  ];
}
