// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

library flutter_frontend_server;

import 'dart:async';
import 'dart:io' hide FileSystemEntity;

import 'package:args/args.dart';
import 'package:kernel/ast.dart';
import 'package:path/path.dart' as path;

import 'package:vm/incremental_compiler.dart';
import 'package:frontend_server/frontend_server.dart' as frontend
    show
        FrontendCompiler,
        CompilerInterface,
        listenAndCompile,
        argParser,
        usage,
        ProgramTransformer;

/// Wrapper around [FrontendCompiler] that adds [widgetCreatorTracker] kernel
/// transformation to the compilation.
class _FlutterFrontendCompiler implements frontend.CompilerInterface {
  final frontend.CompilerInterface _compiler;

  _FlutterFrontendCompiler(StringSink output,
      {bool unsafePackageSerialization,
      bool useDebuggerModuleNames,
      frontend.ProgramTransformer transformer})
      : _compiler = frontend.FrontendCompiler(output,
            transformer: transformer,
            useDebuggerModuleNames: useDebuggerModuleNames,
            unsafePackageSerialization: unsafePackageSerialization);

  @override
  Future<bool> compile(String filename, ArgResults options,
      {IncrementalCompiler generator}) async {
    return _compiler.compile(filename, options, generator: generator);
  }

  @override
  Future<Null> recompileDelta({String entryPoint}) async {
    return _compiler.recompileDelta(entryPoint: entryPoint);
  }

  @override
  void acceptLastDelta() {
    _compiler.acceptLastDelta();
  }

  @override
  Future<void> rejectLastDelta() async {
    return _compiler.rejectLastDelta();
  }

  @override
  void invalidate(Uri uri) {
    _compiler.invalidate(uri);
  }

  @override
  Future<Null> compileExpression(
      String expression,
      List<String> definitions,
      List<String> typeDefinitions,
      String libraryUri,
      String klass,
      bool isStatic) {
    return _compiler.compileExpression(
        expression, definitions, typeDefinitions, libraryUri, klass, isStatic);
  }

  // ignore: annotate_overrides
  Future<Null> compileExpressionToJs(
      String libraryUri,
      int line,
      int column,
      Map<String, String> jsModules,
      Map<String, String> jsFrameValues,
      String moduleName,
      String expression) {
    throw UnimplementedError('Compile expression to JS is not supported');
  }

  @override
  void reportError(String msg) {
    _compiler.reportError(msg);
  }

  @override
  void resetIncrementalCompiler() {
    _compiler.resetIncrementalCompiler();
  }
}

/// Entry point for this module, that creates `_FrontendCompiler` instance and
/// processes user input.
/// `compiler` is an optional parameter so it can be replaced with mocked
/// version for testing.
Future<int> starter(
  List<String> args, {
  frontend.CompilerInterface compiler,
  Stream<List<int>> input,
  StringSink output,
  frontend.ProgramTransformer transformer,
}) async {
  ArgResults options;
  try {
    options = frontend.argParser.parse(args);
  } catch (error) {
    print('ERROR: $error\n');
    print(frontend.usage);
    return 1;
  }

  if (options['train'] as bool) {
    if (!options.rest.isNotEmpty) {
      throw Exception('Must specify input.dart');
    }

    final String input = options.rest[0];
    final String sdkRoot = options['sdk-root'] as String;
    final Directory temp =
        Directory.systemTemp.createTempSync('train_frontend_server');
    try {
      for (int i = 0; i < 3; i++) {
        final String outputTrainingDill = path.join(temp.path, 'app.dill');
        options = frontend.argParser.parse(<String>[
          '--incremental',
          '--sdk-root=$sdkRoot',
          '--output-dill=$outputTrainingDill',
          '--target=flutter',
          '--track-widget-creation',
          '--enable-asserts',
          '--gen-bytecode',
          '--bytecode-options=source-positions,local-var-info,debugger-stops,instance-field-initializers,keep-unreachable-code,avoid-closure-call-instructions',
        ]);
        compiler ??= _FlutterFrontendCompiler(output, transformer: _ToStringTransformer(null));

        await compiler.compile(input, options);
        compiler.acceptLastDelta();
        await compiler.recompileDelta();
        compiler.acceptLastDelta();
        compiler.resetIncrementalCompiler();
        await compiler.recompileDelta();
        compiler.acceptLastDelta();
        await compiler.recompileDelta();
        compiler.acceptLastDelta();
      }
      return 0;
    } finally {
      temp.deleteSync(recursive: true);
    }
  }

  compiler ??= _FlutterFrontendCompiler(output,
      transformer: _ToStringTransformer(transformer),
      useDebuggerModuleNames: options['debugger-module-names'] as bool,
      unsafePackageSerialization:
          options['unsafe-package-serialization'] as bool);

  if (options.rest.isNotEmpty) {
    return await compiler.compile(options.rest[0], options) ? 0 : 254;
  }

  final Completer<int> completer = Completer<int>();
  frontend.listenAndCompile(compiler, input ?? stdin, options, completer);
  return completer.future;
}

class _ToStringVisitor extends RecursiveVisitor<void> {
  @override
  void visitProcedure(Procedure node) {
    if (node.name.name == 'toString' && node.enclosingLibrary != null) {
      assert(node.enclosingClass != null);
      final String importUri = node.enclosingLibrary.importUri.toString();
      if (importUri.startsWith('dart:ui') || importUri.startsWith('package:flutter/')) {
        node.function.replaceWith(
          FunctionNode(
            ReturnStatement(
              SuperMethodInvocation(
                node.name,
                Arguments(<Expression>[]),
              ),
            ),
          ),
        );
      }
    }
    super.visitProcedure(node);
  }
}

class _ToStringTransformer extends frontend.ProgramTransformer {
  _ToStringTransformer(this.child);

  final frontend.ProgramTransformer child;

  @override
  void transform(Component component) {
    assert(child is! _ToStringTransformer);
    component.visitChildren(_ToStringVisitor());
    child?.transform(component);
  }
}
