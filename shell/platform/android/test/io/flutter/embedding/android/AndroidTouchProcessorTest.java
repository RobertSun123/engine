package io.flutter.embedding.android;

import static junit.framework.TestCase.assertEquals;
import static org.mockito.Mockito.inOrder;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.annotation.TargetApi;
import android.content.Context;
import android.view.InputDevice;
import android.view.MotionEvent;
import android.view.ViewConfiguration;
import androidx.test.core.app.ApplicationProvider;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import io.flutter.embedding.engine.renderer.FlutterRenderer;
import java.nio.ByteBuffer;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Captor;
import org.mockito.InOrder;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.robolectric.annotation.Config;

@Config(manifest = Config.NONE)
@RunWith(AndroidJUnit4.class)
@TargetApi(28)
public class AndroidTouchProcessorTest {
  @Mock FlutterRenderer mockRenderer;
  AndroidTouchProcessor touchProcessor;
  @Captor ArgumentCaptor<ByteBuffer> packetCaptor;
  @Captor ArgumentCaptor<Integer> packetSizeCaptor;

  @Before
  public void setUp() {
    MockitoAnnotations.openMocks(this);
    touchProcessor = new AndroidTouchProcessor(mockRenderer, false);
  }

  private long readPointerChange(ByteBuffer buffer) {
    return buffer.getLong(2 * AndroidTouchProcessor.BYTES_PER_FIELD);
  }

  private long readPointerDeviceKind(ByteBuffer buffer) {
    return buffer.getLong(3 * AndroidTouchProcessor.BYTES_PER_FIELD);
  }

  private long readPointerSignalKind(ByteBuffer buffer) {
    return buffer.getLong(4 * AndroidTouchProcessor.BYTES_PER_FIELD);
  }

  private double readPointerPhysicalX(ByteBuffer buffer) {
    return buffer.getDouble(7 * AndroidTouchProcessor.BYTES_PER_FIELD);
  }

  private double readPointerPhysicalY(ByteBuffer buffer) {
    return buffer.getDouble(8 * AndroidTouchProcessor.BYTES_PER_FIELD);
  }

  private double readScrollDeltaX(ByteBuffer buffer) {
    return buffer.getDouble(27 * AndroidTouchProcessor.BYTES_PER_FIELD);
  }

  private double readScrollDeltaY(ByteBuffer buffer) {
    return buffer.getDouble(28 * AndroidTouchProcessor.BYTES_PER_FIELD);
  }

  private double readPointerPanX(ByteBuffer buffer) {
    return buffer.getDouble(29 * AndroidTouchProcessor.BYTES_PER_FIELD);
  }

  private double readPointerPanY(ByteBuffer buffer) {
    return buffer.getDouble(30 * AndroidTouchProcessor.BYTES_PER_FIELD);
  }

  /// Utility method when trying to write a new test. Prefer named readPointerXXX.
  private double readOffset(int offset, ByteBuffer buffer) {
    return buffer.getDouble(offset * AndroidTouchProcessor.BYTES_PER_FIELD);
  }

  private class MotionEventMocker {
    int pointerId;
    int source;
    int toolType;

    MotionEventMocker(int pointerId, int source, int toolType) {
      this.pointerId = pointerId;
      this.source = source;
      this.toolType = toolType;
    }

    MotionEvent mockEvent(int action, float x, float y, int buttonState) {
      MotionEvent event = mock(MotionEvent.class);
      when(event.getDevice()).thenReturn(null);
      when(event.getSource()).thenReturn(source);
      // Ensure that isFromSource does not auto default to false when source is passed in.
      when(event.isFromSource(InputDevice.SOURCE_CLASS_POINTER))
          .thenReturn(source == InputDevice.SOURCE_CLASS_POINTER);
      when(event.isFromSource(InputDevice.SOURCE_MOUSE))
          .thenReturn(source == InputDevice.SOURCE_MOUSE);
      when(event.isFromSource(InputDevice.SOURCE_STYLUS))
          .thenReturn(source == InputDevice.SOURCE_STYLUS);
      when(event.isFromSource(InputDevice.SOURCE_TOUCHSCREEN))
          .thenReturn(source == InputDevice.SOURCE_TOUCHSCREEN);
      when(event.getPointerCount()).thenReturn(1);
      when(event.getActionMasked()).thenReturn(action);
      when(event.getActionIndex()).thenReturn(0);
      when(event.getButtonState()).thenReturn(buttonState);
      when(event.getPointerId(0)).thenReturn(pointerId);
      when(event.getX(0)).thenReturn(x);
      when(event.getY(0)).thenReturn(y);
      when(event.getToolType(0)).thenReturn(toolType);
      when(event.getAxisValue(MotionEvent.AXIS_HSCROLL, pointerId)).thenReturn(x);
      when(event.getAxisValue(MotionEvent.AXIS_VSCROLL, pointerId)).thenReturn(y);
      return event;
    }
  }

  @Test
  public void normalTouch() {
    MotionEventMocker mocker =
        new MotionEventMocker(0, InputDevice.SOURCE_TOUCHSCREEN, MotionEvent.TOOL_TYPE_FINGER);
    touchProcessor.onTouchEvent(mocker.mockEvent(MotionEvent.ACTION_DOWN, 0.0f, 0.0f, 0));
    InOrder inOrder = inOrder(mockRenderer);
    inOrder
        .verify(mockRenderer)
        .dispatchPointerDataPacket(packetCaptor.capture(), packetSizeCaptor.capture());
    ByteBuffer packet = packetCaptor.getValue();
    assertEquals(AndroidTouchProcessor.PointerChange.DOWN, readPointerChange(packet));
    assertEquals(AndroidTouchProcessor.PointerDeviceKind.TOUCH, readPointerDeviceKind(packet));
    assertEquals(AndroidTouchProcessor.PointerSignalKind.NONE, readPointerSignalKind(packet));
    assertEquals(0.0, readPointerPhysicalX(packet));
    assertEquals(0.0, readPointerPhysicalY(packet));
    touchProcessor.onTouchEvent(mocker.mockEvent(MotionEvent.ACTION_MOVE, 10.0f, 5.0f, 0));
    inOrder
        .verify(mockRenderer)
        .dispatchPointerDataPacket(packetCaptor.capture(), packetSizeCaptor.capture());
    packet = packetCaptor.getValue();
    assertEquals(AndroidTouchProcessor.PointerChange.MOVE, readPointerChange(packet));
    assertEquals(AndroidTouchProcessor.PointerDeviceKind.TOUCH, readPointerDeviceKind(packet));
    assertEquals(AndroidTouchProcessor.PointerSignalKind.NONE, readPointerSignalKind(packet));
    assertEquals(10.0, readPointerPhysicalX(packet));
    assertEquals(5.0, readPointerPhysicalY(packet));
    touchProcessor.onTouchEvent(mocker.mockEvent(MotionEvent.ACTION_UP, 10.0f, 5.0f, 0));
    inOrder
        .verify(mockRenderer)
        .dispatchPointerDataPacket(packetCaptor.capture(), packetSizeCaptor.capture());
    packet = packetCaptor.getValue();
    assertEquals(AndroidTouchProcessor.PointerChange.UP, readPointerChange(packet));
    assertEquals(AndroidTouchProcessor.PointerDeviceKind.TOUCH, readPointerDeviceKind(packet));
    assertEquals(AndroidTouchProcessor.PointerSignalKind.NONE, readPointerSignalKind(packet));
    assertEquals(10.0, readPointerPhysicalX(packet));
    assertEquals(5.0, readPointerPhysicalY(packet));
    inOrder.verifyNoMoreInteractions();
  }

  @Test
  public void trackpadGesture() {
    MotionEventMocker mocker =
        new MotionEventMocker(1, InputDevice.SOURCE_MOUSE, MotionEvent.TOOL_TYPE_MOUSE);
    touchProcessor.onTouchEvent(mocker.mockEvent(MotionEvent.ACTION_DOWN, 0.0f, 0.0f, 0));
    InOrder inOrder = inOrder(mockRenderer);
    inOrder
        .verify(mockRenderer)
        .dispatchPointerDataPacket(packetCaptor.capture(), packetSizeCaptor.capture());
    ByteBuffer packet = packetCaptor.getValue();
    assertEquals(AndroidTouchProcessor.PointerChange.PAN_ZOOM_START, readPointerChange(packet));
    assertEquals(AndroidTouchProcessor.PointerDeviceKind.TRACKPAD, readPointerDeviceKind(packet));
    assertEquals(AndroidTouchProcessor.PointerSignalKind.NONE, readPointerSignalKind(packet));
    assertEquals(0.0, readPointerPhysicalX(packet));
    assertEquals(0.0, readPointerPhysicalY(packet));
    touchProcessor.onTouchEvent(mocker.mockEvent(MotionEvent.ACTION_MOVE, 10.0f, 5.0f, 0));
    inOrder
        .verify(mockRenderer)
        .dispatchPointerDataPacket(packetCaptor.capture(), packetSizeCaptor.capture());
    packet = packetCaptor.getValue();
    assertEquals(AndroidTouchProcessor.PointerChange.PAN_ZOOM_UPDATE, readPointerChange(packet));
    assertEquals(AndroidTouchProcessor.PointerDeviceKind.TRACKPAD, readPointerDeviceKind(packet));
    assertEquals(AndroidTouchProcessor.PointerSignalKind.NONE, readPointerSignalKind(packet));
    assertEquals(0.0, readPointerPhysicalX(packet));
    assertEquals(0.0, readPointerPhysicalY(packet));
    assertEquals(10.0, readPointerPanX(packet));
    assertEquals(5.0, readPointerPanY(packet));
    touchProcessor.onTouchEvent(mocker.mockEvent(MotionEvent.ACTION_UP, 10.0f, 5.0f, 0));
    inOrder
        .verify(mockRenderer)
        .dispatchPointerDataPacket(packetCaptor.capture(), packetSizeCaptor.capture());
    packet = packetCaptor.getValue();
    assertEquals(AndroidTouchProcessor.PointerChange.PAN_ZOOM_END, readPointerChange(packet));
    assertEquals(AndroidTouchProcessor.PointerDeviceKind.TRACKPAD, readPointerDeviceKind(packet));
    assertEquals(AndroidTouchProcessor.PointerSignalKind.NONE, readPointerSignalKind(packet));
    assertEquals(0.0, readPointerPhysicalX(packet));
    assertEquals(0.0, readPointerPhysicalY(packet));
    inOrder.verifyNoMoreInteractions();
  }

  @Test
  public void mouse() {
    MotionEventMocker mocker =
        new MotionEventMocker(2, InputDevice.SOURCE_MOUSE, MotionEvent.TOOL_TYPE_MOUSE);
    touchProcessor.onTouchEvent(mocker.mockEvent(MotionEvent.ACTION_DOWN, 0.0f, 0.0f, 1));
    InOrder inOrder = inOrder(mockRenderer);
    inOrder
        .verify(mockRenderer)
        .dispatchPointerDataPacket(packetCaptor.capture(), packetSizeCaptor.capture());
    ByteBuffer packet = packetCaptor.getValue();
    assertEquals(AndroidTouchProcessor.PointerChange.DOWN, readPointerChange(packet));
    assertEquals(AndroidTouchProcessor.PointerDeviceKind.MOUSE, readPointerDeviceKind(packet));
    assertEquals(AndroidTouchProcessor.PointerSignalKind.NONE, readPointerSignalKind(packet));
    assertEquals(0.0, readPointerPhysicalX(packet));
    assertEquals(0.0, readPointerPhysicalY(packet));
    touchProcessor.onTouchEvent(mocker.mockEvent(MotionEvent.ACTION_MOVE, 10.0f, 5.0f, 1));
    inOrder
        .verify(mockRenderer)
        .dispatchPointerDataPacket(packetCaptor.capture(), packetSizeCaptor.capture());
    packet = packetCaptor.getValue();
    assertEquals(AndroidTouchProcessor.PointerChange.MOVE, readPointerChange(packet));
    assertEquals(AndroidTouchProcessor.PointerDeviceKind.MOUSE, readPointerDeviceKind(packet));
    assertEquals(AndroidTouchProcessor.PointerSignalKind.NONE, readPointerSignalKind(packet));
    assertEquals(10.0, readPointerPhysicalX(packet));
    assertEquals(5.0, readPointerPhysicalY(packet));
    touchProcessor.onTouchEvent(mocker.mockEvent(MotionEvent.ACTION_UP, 10.0f, 5.0f, 1));
    inOrder
        .verify(mockRenderer)
        .dispatchPointerDataPacket(packetCaptor.capture(), packetSizeCaptor.capture());
    packet = packetCaptor.getValue();
    assertEquals(AndroidTouchProcessor.PointerChange.UP, readPointerChange(packet));
    assertEquals(AndroidTouchProcessor.PointerDeviceKind.MOUSE, readPointerDeviceKind(packet));
    assertEquals(AndroidTouchProcessor.PointerSignalKind.NONE, readPointerSignalKind(packet));
    assertEquals(10.0, readPointerPhysicalX(packet));
    assertEquals(5.0, readPointerPhysicalY(packet));
    inOrder.verifyNoMoreInteractions();
  }

  @Test
  public void unexpectedMaskedAction() {
    // Regression test for https://github.com/flutter/flutter/issues/111068
    MotionEventMocker mocker =
        new MotionEventMocker(1, InputDevice.SOURCE_STYLUS, MotionEvent.TOOL_TYPE_STYLUS);
    // ACTION_BUTTON_PRESS is not handled by AndroidTouchProcessor, nothing should be dispatched.
    touchProcessor.onTouchEvent(mocker.mockEvent(MotionEvent.ACTION_BUTTON_PRESS, 0.0f, 0.0f, 0));
    verify(mockRenderer, never()).dispatchPointerDataPacket(ByteBuffer.allocate(0), 0);
  }

  @Test
  public void scrollWheel() {
    // Pointer id must be zero to match actionIndex in mocked event.
    final int pointerId = 0;
    MotionEventMocker mocker =
        new MotionEventMocker(
            pointerId, InputDevice.SOURCE_CLASS_POINTER, MotionEvent.TOOL_TYPE_MOUSE);
    final float horizontalScrollValue = -1f;
    final float verticalScrollValue = .5f;
    final Context context = ApplicationProvider.getApplicationContext();
    final double horizontalScaleFactor =
        ViewConfiguration.get(context).getScaledHorizontalScrollFactor();
    final double verticalScaleFactor =
        ViewConfiguration.get(context).getScaledVerticalScrollFactor();
    // Zero verticalScaleFactor will cause this test to miss bugs.
    assertEquals("zero horizontal scale factor", true, horizontalScaleFactor != 0);
    assertEquals("zero vertical scale factor", true, verticalScaleFactor != 0);

    final MotionEvent event =
        mocker.mockEvent(MotionEvent.ACTION_SCROLL, horizontalScrollValue, verticalScrollValue, 1);
    boolean handled = touchProcessor.onGenericMotionEvent(event, context);

    InOrder inOrder = inOrder(mockRenderer);
    inOrder
        .verify(mockRenderer)
        .dispatchPointerDataPacket(packetCaptor.capture(), packetSizeCaptor.capture());
    ByteBuffer packet = packetCaptor.getValue();

    assertEquals(-horizontalScrollValue * horizontalScaleFactor, readScrollDeltaX(packet));
    assertEquals(-verticalScrollValue * verticalScaleFactor, readScrollDeltaY(packet));
    verify(event).getAxisValue(MotionEvent.AXIS_HSCROLL, pointerId);
    verify(event).getAxisValue(MotionEvent.AXIS_VSCROLL, pointerId);

    inOrder.verifyNoMoreInteractions();
  }
}
