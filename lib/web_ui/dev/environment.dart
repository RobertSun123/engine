// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// @dart = 2.6
import 'dart:io' as io;
import 'package:path/path.dart' as pathlib;

import 'exceptions.dart';

/// Contains various environment variables, such as common file paths and command-line options.
Environment get environment {
  _environment ??= Environment();
  return _environment;
}
Environment _environment;

/// Contains various environment variables, such as common file paths and command-line options.
class Environment {
  factory Environment() {
    final io.File self = io.File.fromUri(io.Platform.script);
    final io.Directory engineSrcDir = self.parent.parent.parent.parent.parent;
    final io.Directory outDir = io.Directory(pathlib.join(engineSrcDir.path, 'out'));
    final io.Directory hostDebugUnoptDir = io.Directory(pathlib.join(outDir.path, 'host_debug_unopt'));
    final io.Directory dartSdkDir = io.Directory(pathlib.join(hostDebugUnoptDir.path, 'dart-sdk'));
    final io.Directory webUiRootDir = io.Directory(pathlib.join(engineSrcDir.path, 'flutter', 'lib', 'web_ui'));
    final io.Directory integrationTestsDir = io.Directory(pathlib.join(engineSrcDir.path, 'flutter', 'e2etests', 'web'));

    for (io.Directory expectedDirectory in <io.Directory>[engineSrcDir, outDir, hostDebugUnoptDir, dartSdkDir, webUiRootDir]) {
      if (!expectedDirectory.existsSync()) {
        throw ToolException('$expectedDirectory does not exist.');
      }
    }

    return Environment._(
      self: self,
      webUiRootDir: webUiRootDir,
      engineSrcDir: engineSrcDir,
      integrationTestsDir: integrationTestsDir,
      outDir: outDir,
      hostDebugUnoptDir: hostDebugUnoptDir,
      dartSdkDir: dartSdkDir,
    );
  }

  Environment._({
    this.self,
    this.webUiRootDir,
    this.engineSrcDir,
    this.integrationTestsDir,
    this.outDir,
    this.hostDebugUnoptDir,
    this.dartSdkDir,
  });

  /// The Dart script that's currently running.
  final io.File self;

  /// Path to the "web_ui" package sources.
  final io.Directory webUiRootDir;

  /// Path to the engine's "src" directory.
  final io.Directory engineSrcDir;

  /// Path to the web integration tests.
  final io.Directory integrationTestsDir;

  /// Path to the engine's "out" directory.
  ///
  /// This is where you'll find the ninja output, such as the Dart SDK.
  final io.Directory outDir;

  /// The "host_debug_unopt" build of the Dart SDK.
  final io.Directory hostDebugUnoptDir;

  /// The root of the Dart SDK.
  final io.Directory dartSdkDir;

  /// The "dart" executable file.
  String get dartExecutable => pathlib.join(dartSdkDir.path, 'bin', 'dart');

  /// The "pub" executable file.
  String get pubExecutable => pathlib.join(dartSdkDir.path, 'bin', 'pub');

  /// The "dart2js" executable file.
  String get dart2jsExecutable => pathlib.join(dartSdkDir.path, 'bin', 'dart2js');

  /// Path to where github.com/flutter/engine is checked out inside the engine workspace.
  io.Directory get flutterDirectory => io.Directory(pathlib.join(engineSrcDir.path, 'flutter'));
  io.Directory get webSdkRootDir => io.Directory(pathlib.join(
    flutterDirectory.path,
    'web_sdk',
  ));

  /// Path to the "web_engine_tester" package.
  io.Directory get webEngineTesterRootDir => io.Directory(pathlib.join(
    webSdkRootDir.path,
    'web_engine_tester',
  ));

  /// Path to the "build" directory, generated by "package:build_runner".
  ///
  /// This is where compiled output goes.
  io.Directory get webUiBuildDir => io.Directory(pathlib.join(
    webUiRootDir.path,
    'build',
  ));

  /// Path to the ".dart_tool" directory, generated by various Dart tools.
  io.Directory get webUiDartToolDir => io.Directory(pathlib.join(
    webUiRootDir.path,
    '.dart_tool',
  ));

  /// Path to the "dev" directory containing engine developer tools and
  /// configuration files.
  io.Directory get webUiDevDir => io.Directory(pathlib.join(
    webUiRootDir.path,
    'dev',
  ));

  /// Path to the clone of the flutter/goldens repository.
  io.Directory get webUiGoldensRepositoryDirectory => io.Directory(pathlib.join(
    webUiDartToolDir.path,
    'goldens',
  ));
}
