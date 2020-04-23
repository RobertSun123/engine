// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// @dart = 2.6
part of engine;

/// Maps AutofillHints from the framework to the autofill hints that is used for
/// browsers.
/// See: https://github.com/flutter/flutter/blob/master/packages/flutter/lib/src/services/autofill.dart
/// See: https://developer.mozilla.org/en-US/docs/Web/HTML/Attributes/autocomplete
class BrowserAutofillHints {
  static final BrowserAutofillHints _singletonInstance =
      BrowserAutofillHints._();

  /// The [BrowserAutofillHints] singleton.
  static BrowserAutofillHints get instance => _singletonInstance;

  Map<String, String> _flutterToEngineMap;

  Map<String, String> _engineToFlutterMap;

  BrowserAutofillHints._() {
    _flutterToEngineMap = {
      'birthday': 'bday',
      'birthdayDay': 'bday-day',
      'birthdayMonth': 'bday-month',
      'birthdayYear': 'bday-year',
      'countryCode': 'country',
      'countryName': 'country-name',
      'creditCardExpirationDate': 'cc-exp',
      'creditCardExpirationMonth': 'cc-exp-month',
      'creditCardExpirationYear': 'cc-exp-year',
      'creditCardFamilyName': 'cc-family-name',
      'creditCardGivenName': 'cc-given-name',
      'creditCardMiddleName': 'cc-additional-name',
      'creditCardName': 'cc-name',
      'creditCardNumber': 'cc-number',
      'creditCardSecurityCode': 'cc-csc',
      'creditCardType': 'cc-type',
      'email': 'email',
      'familyName': 'familyName',
      'fullStreetAddress': 'street-address',
      'gender': 'sex',
      'givenName': 'given-name',
      'impp': 'impp',
      'jobTitle': 'organization-title',
      'language': 'language',
      'middleName': 'middleName',
      'name': 'name',
      'namePrefix': 'honorific-prefix',
      'nameSuffix': 'honorific-suffix',
      'newPassword': 'new-password',
      'nickname': 'nickname',
      'oneTimeCode': 'one-time-code',
      'organizationName': 'organization',
      'password': 'current-password',
      'photo': 'photo',
      'postalCode': 'postal-code',
      'streetAddressLevel1': 'address-level1',
      'streetAddressLevel2': 'address-level2',
      'streetAddressLevel3': 'address-level3',
      'streetAddressLevel4': 'address-level4',
      'streetAddressLine1': 'address-line1',
      'streetAddressLine2': 'address-line2',
      'streetAddressLine3': 'address-line3',
      'telephoneNumber': 'tel',
      'telephoneNumberAreaCode': 'tel-area-code',
      'telephoneNumberCountryCode': 'tel-country-code',
      'telephoneNumberExtension': 'tel-extension',
      'telephoneNumberLocal': 'tel-local',
      'telephoneNumberLocalPrefix': 'tel-local-prefix',
      'telephoneNumberLocalSuffix': 'tel-local-suffix',
      'telephoneNumberNational': 'tel-national',
      'transactionAmount': 'transaction-amount',
      'transactionCurrency': 'transaction-currency',
      'url': 'url',
      'username': 'username',
    };

    _engineToFlutterMap = Map<String, String>();

    _flutterToEngineMap.keys.forEach((String key) {
      _engineToFlutterMap[_flutterToEngineMap[key]] = key;
    });
  }

  String flutterToEngine(String flutterAutofillHint) {
    if(_flutterToEngineMap.containsKey(flutterAutofillHint)) {
      return _flutterToEngineMap[flutterAutofillHint];
    } else {
      // Use the hints as it is.
      return flutterAutofillHint;
    }
  }
}
