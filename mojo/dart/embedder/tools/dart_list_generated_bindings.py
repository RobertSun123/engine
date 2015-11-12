#!/usr/bin/python
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This script scans a directory tree for any .mojom files and outputs a list of
# .mojom.dart files.

import argparse
import os
import sys

def main(args):
  parser = argparse.ArgumentParser(
      description='Prints list of generated dart binding source files.')
  parser.add_argument('source_directory',
                      metavar='source_directory',
                      help='Path to source directory tree containing .mojom'
                           ' files')
  parser.add_argument('relative_directory_root',
                      metavar='relative_directory_root',
                      help='Path to directory which all outputted paths will'
                           'be relative to.')
  args = parser.parse_args()
  # Directory to start searching for .mojom files.
  source_directory = args.source_directory
  # Prefix to chop off output.
  root_prefix = os.path.abspath(args.relative_directory_root)
  for dirname, _, filenames in os.walk(source_directory):
    # filter for .mojom.dart files.
    filenames = [f for f in filenames if f.endswith('.mojom')]
    for f in filenames:
      # Ignore tests.
      if dirname.endswith('tests'):
        continue;
      path = os.path.abspath(os.path.join(dirname, f))
      path = os.path.relpath(path, root_prefix)
      # Append .dart.
      path += '.dart'
      print(path)

if __name__ == '__main__':
  sys.exit(main(sys.argv[1:]))

