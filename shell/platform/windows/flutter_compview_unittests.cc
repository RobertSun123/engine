#include "flutter/shell/platform/windows/testing/flutter_compview_test.h"
#include "gtest/gtest.h"

namespace flutter {
namespace testing {

TEST(FlutterCompViewTest, CreateDestroy) {
  FlutterCompViewTest window(800, 600);
  ASSERT_TRUE(TRUE);
}

//TODO MOVE to wrapper
//TEST(FlutterCompViewTest, CanFontChange) {
//  FlutterCompViewTest window(800, 600);
//  HWND hwnd = window.GetWindowHandle();
//  LRESULT result = SendMessage(hwnd, WM_FONTCHANGE, NULL, NULL);
//  ASSERT_EQ(result, 0);
//  ASSERT_TRUE(window.OnFontChangeWasCalled());
//}

}  // namespace testing
}  // namespace flutter
