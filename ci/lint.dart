/// Runs clang-tidy on files with changes.
///
/// usage:
/// dart lint.dart <path to compile_commands.json> <path to git repository> [clang-tidy checks]

import 'dart:io' show File, Process, ProcessResult, exit;
import 'dart:convert' show jsonDecode, utf8;

class Command {
  String directory;
  String command;
  String file;
}

Command parseCommand(Map<String, dynamic> map) {
  return Command()
    ..directory = map['directory']
    ..command = map['command']
    ..file = map['file'];
}

String calcTidyArgs(Command command) {
  String result = command.command;
  result = result.replaceAll(RegExp(r'\S*clang/bin/clang'), '');
  result = result.replaceAll(RegExp(r'-MF \S*'), '');
  return result;
}

String calcTidyPath(Command command) {
  final RegExp regex = RegExp(r'\S*clang/bin/clang');
  return regex
      .stringMatch(command.command)
      .replaceAll('clang/bin/clang', 'clang/bin/clang-tidy');
}

bool isNonEmptyString(String str) => str.length > 0;

bool containsAny(String str, List<String> queries) {
  for (String query in queries) {
    if (str.contains(query)) {
      return true;
    }
  }
  return false;
}

/// Returns a list of all files with current changes or differ from `master`.
List<String> getListOfChangedFiles(String repoPath) {
  final Set<String> result = Set<String>();
  final ProcessResult diffResult = Process.runSync(
      'git', ['diff', '--name-only'],
      workingDirectory: repoPath);
  final ProcessResult diffCachedResult = Process.runSync(
      'git', ['diff', '--cached', '--name-only'],
      workingDirectory: repoPath);
  final ProcessResult masterResult = Process.runSync(
      'git', ['diff', '--name-only', 'master'],
      workingDirectory: repoPath);
  result.addAll(diffResult.stdout.split('\n').where(isNonEmptyString));
  result.addAll(diffCachedResult.stdout.split('\n').where(isNonEmptyString));
  result.addAll(masterResult.stdout.split('\n').where(isNonEmptyString));
  return result.toList();
}

void main(List<String> arguments) async {
  final String buildCommandsPath = arguments[0];
  final String repoPath = arguments[1];
  final String checks = arguments.length >= 3 ? '--checks=${arguments[2]}' : '--config';
  final List<String> changedFiles = getListOfChangedFiles(repoPath);

  final List<dynamic> buildCommandMaps =
      jsonDecode(await new File(buildCommandsPath).readAsString());
  final List<Command> buildCommands =
      buildCommandMaps.map((x) => parseCommand(x)).toList();
  final Command firstCommand = buildCommands[0];
  final String tidyPath = calcTidyPath(firstCommand);
  final List<Command> changedFileBuildCommands =
      buildCommands.where((x) => containsAny(x.file, changedFiles)).toList();

  int exitCode = 0;
  for (Command command in changedFileBuildCommands) {
    final String tidyArgs = calcTidyArgs(command);
    print('# linting ${command.file}');
    final List<String> args = [command.file, '', '--'];
    args.addAll(tidyArgs.split(' '));
    final Process process = await Process.start(tidyPath, args,
        workingDirectory: command.directory, runInShell: false);
    process.stdout.transform(utf8.decoder).listen((data) {
      print(data);
      exitCode = 1;
    });
    await process.exitCode;
  }
  exit(exitCode);
}
