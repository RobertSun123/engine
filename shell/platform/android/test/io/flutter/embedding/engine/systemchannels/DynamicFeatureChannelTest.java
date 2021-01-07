package io.flutter.embedding.engine.systemchannels;

import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;

import android.content.res.AssetManager;
import io.flutter.embedding.engine.FlutterJNI;
import io.flutter.embedding.engine.dart.DartExecutor;
import io.flutter.embedding.engine.deferredcomponents.DeferredComponentManager;
import io.flutter.plugin.common.MethodCall;
import io.flutter.plugin.common.MethodChannel;
import java.util.HashMap;
import java.util.Map;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.annotation.Config;

class TestDeferredComponentManager implements DeferredComponentManager {
  DeferredComponentChannel channel;
  String moduleName;

  public void setJNI(FlutterJNI flutterJNI) {}

  public void setDeferredComponentChannel(DeferredComponentChannel channel) {
    this.channel = channel;
  }

  public void installDeferredComponent(int loadingUnitId, String moduleName) {
    this.moduleName = moduleName;
  }

  public void completeInstall() {
    channel.completeInstallSuccess(moduleName);
  }

  public String getDeferredComponentInstallState(int loadingUnitId, String moduleName) {
    return "installed";
  }

  public void loadAssets(int loadingUnitId, String moduleName) {}

  public void loadDartLibrary(int loadingUnitId, String moduleName) {}

  public void uninstallDeferredComponent(int loadingUnitId, String moduleName) {}

  public void destroy() {}
}

@Config(manifest = Config.NONE)
@RunWith(RobolectricTestRunner.class)
public class DeferredComponentChannelTest {
  @Test
  public void deferredComponentChannel_installCompletesResults() {
    MethodChannel rawChannel = mock(MethodChannel.class);
    FlutterJNI mockFlutterJNI = mock(FlutterJNI.class);
    DartExecutor dartExecutor = new DartExecutor(mockFlutterJNI, mock(AssetManager.class));
    TestDeferredComponentManager testDeferredComponentManager = new TestDeferredComponentManager();
    DeferredComponentChannel fakeDeferredComponentChannel = new DeferredComponentChannel(dartExecutor);
    fakeDeferredComponentChannel.setDeferredComponentManager(testDeferredComponentManager);

    Map<String, Object> args = new HashMap<>();
    args.put("loadingUnitId", -1);
    args.put("moduleName", "hello");
    MethodCall methodCall = new MethodCall("installDeferredComponent", args);
    MethodChannel.Result mockResult = mock(MethodChannel.Result.class);
    fakeDeferredComponentChannel.parsingMethodHandler.onMethodCall(methodCall, mockResult);

    testDeferredComponentManager.completeInstall();
    verify(mockResult).success(null);
  }

  @Test
  public void deferredComponentChannel_installCompletesMultipleResults() {
    MethodChannel rawChannel = mock(MethodChannel.class);
    FlutterJNI mockFlutterJNI = mock(FlutterJNI.class);
    DartExecutor dartExecutor = new DartExecutor(mockFlutterJNI, mock(AssetManager.class));
    TestDeferredComponentManager testDeferredComponentManager = new TestDeferredComponentManager();
    DeferredComponentChannel fakeDeferredComponentChannel = new DeferredComponentChannel(dartExecutor);
    fakeDeferredComponentChannel.setDeferredComponentManager(testDeferredComponentManager);

    Map<String, Object> args = new HashMap<>();
    args.put("loadingUnitId", -1);
    args.put("moduleName", "hello");
    MethodCall methodCall = new MethodCall("installDeferredComponent", args);
    MethodChannel.Result mockResult1 = mock(MethodChannel.Result.class);
    MethodChannel.Result mockResult2 = mock(MethodChannel.Result.class);
    fakeDeferredComponentChannel.parsingMethodHandler.onMethodCall(methodCall, mockResult1);
    fakeDeferredComponentChannel.parsingMethodHandler.onMethodCall(methodCall, mockResult2);

    testDeferredComponentManager.completeInstall();
    verify(mockResult1).success(null);
    verify(mockResult2).success(null);
  }

  @Test
  public void deferredComponentChannel_getInstallState() {
    MethodChannel rawChannel = mock(MethodChannel.class);
    FlutterJNI mockFlutterJNI = mock(FlutterJNI.class);
    DartExecutor dartExecutor = new DartExecutor(mockFlutterJNI, mock(AssetManager.class));
    TestDeferredComponentManager testDeferredComponentManager = new TestDeferredComponentManager();
    DeferredComponentChannel fakeDeferredComponentChannel = new DeferredComponentChannel(dartExecutor);
    fakeDeferredComponentChannel.setDeferredComponentManager(testDeferredComponentManager);

    Map<String, Object> args = new HashMap<>();
    args.put("loadingUnitId", -1);
    args.put("moduleName", "hello");
    MethodCall methodCall = new MethodCall("getDeferredComponentInstallState", args);
    MethodChannel.Result mockResult = mock(MethodChannel.Result.class);
    fakeDeferredComponentChannel.parsingMethodHandler.onMethodCall(methodCall, mockResult);

    testDeferredComponentManager.completeInstall();
    verify(mockResult).success("installed");
  }
}
