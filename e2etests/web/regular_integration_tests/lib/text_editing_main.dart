// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'package:flutter/material.dart';

void main() => runApp(MyApp());

class MyApp extends StatelessWidget {
  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      key: const Key('mainapp'),
      title: 'Integration Test App',
      home: MyHomePage(title: 'Integration Test App'),
    );
  }
}

class MyHomePage extends StatefulWidget {
  MyHomePage({Key key, this.title}) : super(key: key);

  final String title;

  @override
  _MyHomePageState createState() => _MyHomePageState();
}

class _MyHomePageState extends State<MyHomePage> {
  String infoText = 'no-enter';

  final TextEditingController _controller =
      TextEditingController(text: 'Text1');

  final TextEditingController _controller2 =
      TextEditingController(text: 'Text2');

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Text(widget.title),
      ),
      body: Center(
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: <Widget>[
            const Text(
              'Text Editing Test',
            ),
            TextFormField(
              key: const Key('input'),
              enabled: true,
              controller: _controller,
              decoration: const InputDecoration(
                labelText: 'Text Input Field:',
              ),
            ),
            const Text(
              'Text Editing Test 2',
            ),
            TextFormField(
              key: const Key('input2'),
              enabled: true,
              controller: _controller2,
              decoration: const InputDecoration(
                labelText: 'Text Input Field 2:',
              ),
              onFieldSubmitted: (String str) {
                print('event received');
                setState(() => infoText = 'enter pressed');
              },
            ),
            Text(
              infoText,
              key: const Key('text'),
            ),
          ],
        ),
      ),
    );
  }
}
