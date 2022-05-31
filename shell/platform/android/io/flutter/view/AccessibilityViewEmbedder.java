// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package io.flutter.view;

import android.annotation.SuppressLint;
import android.graphics.Rect;
import android.os.Build;
import android.os.Bundle;
import android.os.Parcel;
import android.util.SparseArray;
import android.view.MotionEvent;
import android.view.View;
import android.view.accessibility.AccessibilityEvent;
import android.view.accessibility.AccessibilityNodeInfo;
import android.view.accessibility.AccessibilityNodeProvider;
import android.view.accessibility.AccessibilityRecord;
import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import io.flutter.Log;
import java.lang.reflect.Field;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.HashMap;
import java.util.Map;

/**
 * Facilitates embedding of platform views in the accessibility tree generated by the accessibility
 * bridge.
 *
 * <p>Embedding is done by mirroring the accessibility tree of the platform view as a subtree of the
 * flutter accessibility tree.
 *
 * <p>This class relies on hidden system APIs to extract the accessibility information and does not
 * work starting Android P; If the reflection accessors are not available we fail silently by
 * embedding a null node, the app continues working but the accessibility information for the
 * platform view will not be embedded.
 *
 * <p>We use the term `flutterId` for virtual accessibility node IDs in the FlutterView tree, and
 * the term `originId` for the virtual accessibility node IDs in the platform view's tree.
 * Internally this class maintains a bidirectional mapping between `flutterId`s and the
 * corresponding platform view and `originId`.
 */
@Keep
class AccessibilityViewEmbedder {
  private static final String TAG = "AccessibilityBridge";

  private final ReflectionAccessors reflectionAccessors;

  // The view to which the platform view is embedded, this is typically FlutterView.
  private final View rootAccessibilityView;

  // Maps a flutterId to the corresponding platform view and originId.
  private final SparseArray<ViewAndId> flutterIdToOrigin;

  // Maps a platform view and originId to a corresponding flutterID.
  private final Map<ViewAndId, Integer> originToFlutterId;

  // Maps an embedded view to it's screen bounds.
  // This is used to translate the coordinates of the accessibility node subtree to the main
  // display's coordinate
  // system.
  private final Map<View, Rect> embeddedViewToDisplayBounds;

  private int nextFlutterId;

  AccessibilityViewEmbedder(@NonNull View rootAccessibiiltyView, int firstVirtualNodeId) {
    reflectionAccessors = new ReflectionAccessors();
    flutterIdToOrigin = new SparseArray<>();
    this.rootAccessibilityView = rootAccessibiiltyView;
    nextFlutterId = firstVirtualNodeId;
    originToFlutterId = new HashMap<>();
    embeddedViewToDisplayBounds = new HashMap<>();
  }

  /**
   * Returns the root accessibility node for an embedded platform view.
   *
   * @param flutterId the virtual accessibility ID for the node in flutter accessibility tree
   * @param displayBounds the display bounds for the node in screen coordinates
   */
  public AccessibilityNodeInfo getRootNode(
      @NonNull View embeddedView, int flutterId, @NonNull Rect displayBounds) {
    AccessibilityNodeInfo originNode = embeddedView.createAccessibilityNodeInfo();
    Long originPackedId = reflectionAccessors.getSourceNodeId(originNode);
    if (originPackedId == null) {
      return null;
    }
    embeddedViewToDisplayBounds.put(embeddedView, displayBounds);
    int originId = ReflectionAccessors.getVirtualNodeId(originPackedId);
    cacheVirtualIdMappings(embeddedView, originId, flutterId);
    return convertToFlutterNode(originNode, flutterId, embeddedView);
  }

  /** Creates the accessibility node info for the node identified with `flutterId`. */
  @Nullable
  public AccessibilityNodeInfo createAccessibilityNodeInfo(int flutterId) {
    ViewAndId origin = flutterIdToOrigin.get(flutterId);
    if (origin == null) {
      return null;
    }
    if (!embeddedViewToDisplayBounds.containsKey(origin.view)) {
      // This might happen if the embedded view is sending accessibility event before the first
      // Flutter semantics
      // tree was sent to the accessibility bridge. In this case we don't return a node as we do not
      // know the
      // bounds yet.
      // https://github.com/flutter/flutter/issues/30068
      return null;
    }
    AccessibilityNodeProvider provider = origin.view.getAccessibilityNodeProvider();
    if (provider == null) {
      // The provider is null for views that don't have a virtual accessibility tree.
      // We currently only support embedding virtual hierarchies in the Flutter tree.
      // TODO(amirh): support embedding non virtual hierarchies.
      // https://github.com/flutter/flutter/issues/29717
      return null;
    }
    AccessibilityNodeInfo originNode =
        origin.view.getAccessibilityNodeProvider().createAccessibilityNodeInfo(origin.id);
    if (originNode == null) {
      return null;
    }
    return convertToFlutterNode(originNode, flutterId, origin.view);
  }

  /*
   * Creates an AccessibilityNodeInfo that can be attached to the Flutter accessibility tree and is equivalent to
   * originNode(which belongs to embeddedView). The virtual ID for the created node will be flutterId.
   */
  @NonNull
  private AccessibilityNodeInfo convertToFlutterNode(
      @NonNull AccessibilityNodeInfo originNode, int flutterId, @NonNull View embeddedView) {
    AccessibilityNodeInfo result = AccessibilityNodeInfo.obtain(rootAccessibilityView, flutterId);
    result.setPackageName(rootAccessibilityView.getContext().getPackageName());
    result.setSource(rootAccessibilityView, flutterId);
    result.setClassName(originNode.getClassName());

    Rect displayBounds = embeddedViewToDisplayBounds.get(embeddedView);

    copyAccessibilityFields(originNode, result);
    setFlutterNodesTranslateBounds(originNode, displayBounds, result);
    addChildrenToFlutterNode(originNode, embeddedView, result);
    setFlutterNodeParent(originNode, embeddedView, result);

    return result;
  }

  private void setFlutterNodeParent(
      @NonNull AccessibilityNodeInfo originNode,
      @NonNull View embeddedView,
      @NonNull AccessibilityNodeInfo result) {
    Long parentOriginPackedId = reflectionAccessors.getParentNodeId(originNode);
    if (parentOriginPackedId == null) {
      return;
    }
    int parentOriginId = ReflectionAccessors.getVirtualNodeId(parentOriginPackedId);
    Integer parentFlutterId = originToFlutterId.get(new ViewAndId(embeddedView, parentOriginId));
    if (parentFlutterId != null) {
      result.setParent(rootAccessibilityView, parentFlutterId);
    }
  }

  private void addChildrenToFlutterNode(
      @NonNull AccessibilityNodeInfo originNode,
      @NonNull View embeddedView,
      @NonNull AccessibilityNodeInfo resultNode) {
    for (int i = 0; i < originNode.getChildCount(); i++) {
      Long originPackedId = reflectionAccessors.getChildId(originNode, i);
      if (originPackedId == null) {
        continue;
      }
      int originId = ReflectionAccessors.getVirtualNodeId(originPackedId);
      ViewAndId origin = new ViewAndId(embeddedView, originId);
      int childFlutterId;
      if (originToFlutterId.containsKey(origin)) {
        childFlutterId = originToFlutterId.get(origin);
      } else {
        childFlutterId = nextFlutterId++;
        cacheVirtualIdMappings(embeddedView, originId, childFlutterId);
      }
      resultNode.addChild(rootAccessibilityView, childFlutterId);
    }
  }

  // Caches a bidirectional mapping of (embeddedView, originId)<-->flutterId.
  // Where originId is a virtual node ID in the embeddedView's tree, and flutterId is the ID
  // of the corresponding node in the Flutter virtual accessibility nodes tree.
  private void cacheVirtualIdMappings(@NonNull View embeddedView, int originId, int flutterId) {
    ViewAndId origin = new ViewAndId(embeddedView, originId);
    originToFlutterId.put(origin, flutterId);
    flutterIdToOrigin.put(flutterId, origin);
  }

  // Suppressing deprecation warning for AccessibilityNodeInfo#getBoundsinParent and
  // AccessibilityNodeInfo#getBoundsinParent as we are copying the platform view's
  // accessibility node and we should not lose any available bounds information.
  @SuppressWarnings("deprecation")
  private void setFlutterNodesTranslateBounds(
      @NonNull AccessibilityNodeInfo originNode,
      @NonNull Rect displayBounds,
      @NonNull AccessibilityNodeInfo resultNode) {
    Rect boundsInParent = new Rect();
    originNode.getBoundsInParent(boundsInParent);
    resultNode.setBoundsInParent(boundsInParent);

    Rect boundsInScreen = new Rect();
    originNode.getBoundsInScreen(boundsInScreen);
    boundsInScreen.offset(displayBounds.left, displayBounds.top);
    resultNode.setBoundsInScreen(boundsInScreen);
  }

  private void copyAccessibilityFields(
      @NonNull AccessibilityNodeInfo input, @NonNull AccessibilityNodeInfo output) {
    output.setAccessibilityFocused(input.isAccessibilityFocused());
    output.setCheckable(input.isCheckable());
    output.setChecked(input.isChecked());
    output.setContentDescription(input.getContentDescription());
    output.setEnabled(input.isEnabled());
    output.setClickable(input.isClickable());
    output.setFocusable(input.isFocusable());
    output.setFocused(input.isFocused());
    output.setLongClickable(input.isLongClickable());
    output.setMovementGranularities(input.getMovementGranularities());
    output.setPassword(input.isPassword());
    output.setScrollable(input.isScrollable());
    output.setSelected(input.isSelected());
    output.setText(input.getText());
    output.setVisibleToUser(input.isVisibleToUser());

    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR2) {
      output.setEditable(input.isEditable());
    }
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
      output.setCanOpenPopup(input.canOpenPopup());
      output.setCollectionInfo(input.getCollectionInfo());
      output.setCollectionItemInfo(input.getCollectionItemInfo());
      output.setContentInvalid(input.isContentInvalid());
      output.setDismissable(input.isDismissable());
      output.setInputType(input.getInputType());
      output.setLiveRegion(input.getLiveRegion());
      output.setMultiLine(input.isMultiLine());
      output.setRangeInfo(input.getRangeInfo());
    }
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
      output.setError(input.getError());
      output.setMaxTextLength(input.getMaxTextLength());
    }
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
      output.setContextClickable(input.isContextClickable());
      // TODO(amirh): copy traversal before and after.
      // https://github.com/flutter/flutter/issues/29718
    }
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
      output.setDrawingOrder(input.getDrawingOrder());
      output.setImportantForAccessibility(input.isImportantForAccessibility());
    }
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
      output.setAvailableExtraData(input.getAvailableExtraData());
      output.setHintText(input.getHintText());
      output.setShowingHintText(input.isShowingHintText());
    }
  }

  /**
   * Delegates an AccessibilityNodeProvider#requestSendAccessibilityEvent from the
   * AccessibilityBridge to the embedded view.
   *
   * @return True if the event was sent.
   */
  public boolean requestSendAccessibilityEvent(
      @NonNull View embeddedView, @NonNull View eventOrigin, @NonNull AccessibilityEvent event) {
    AccessibilityEvent translatedEvent = AccessibilityEvent.obtain(event);
    Long originPackedId = reflectionAccessors.getRecordSourceNodeId(event);
    if (originPackedId == null) {
      return false;
    }
    int originVirtualId = ReflectionAccessors.getVirtualNodeId(originPackedId);
    Integer flutterId = originToFlutterId.get(new ViewAndId(embeddedView, originVirtualId));
    if (flutterId == null) {
      flutterId = nextFlutterId++;
      cacheVirtualIdMappings(embeddedView, originVirtualId, flutterId);
    }
    translatedEvent.setSource(rootAccessibilityView, flutterId);
    translatedEvent.setClassName(event.getClassName());
    translatedEvent.setPackageName(event.getPackageName());

    for (int i = 0; i < translatedEvent.getRecordCount(); i++) {
      AccessibilityRecord record = translatedEvent.getRecord(i);
      Long recordOriginPackedId = reflectionAccessors.getRecordSourceNodeId(record);
      if (recordOriginPackedId == null) {
        return false;
      }
      int recordOriginVirtualID = ReflectionAccessors.getVirtualNodeId(recordOriginPackedId);
      ViewAndId originViewAndId = new ViewAndId(embeddedView, recordOriginVirtualID);
      if (!originToFlutterId.containsKey(originViewAndId)) {
        return false;
      }
      int recordFlutterId = originToFlutterId.get(originViewAndId);
      record.setSource(rootAccessibilityView, recordFlutterId);
    }

    return rootAccessibilityView
        .getParent()
        .requestSendAccessibilityEvent(eventOrigin, translatedEvent);
  }

  /**
   * Delegates an @{link AccessibilityNodeProvider#performAction} from the AccessibilityBridge to
   * the embedded view's accessibility node provider.
   *
   * @return True if the action was performed.
   */
  public boolean performAction(int flutterId, int accessibilityAction, @Nullable Bundle arguments) {
    ViewAndId origin = flutterIdToOrigin.get(flutterId);
    if (origin == null) {
      return false;
    }
    View embeddedView = origin.view;
    AccessibilityNodeProvider provider = embeddedView.getAccessibilityNodeProvider();
    if (provider == null) {
      return false;
    }
    return provider.performAction(origin.id, accessibilityAction, arguments);
  }

  /**
   * Returns a flutterID for an accessibility record, or null if no mapping exists.
   *
   * @param embeddedView the embedded view that the record is associated with.
   */
  @Nullable
  public Integer getRecordFlutterId(
      @NonNull View embeddedView, @NonNull AccessibilityRecord record) {
    Long originPackedId = reflectionAccessors.getRecordSourceNodeId(record);
    if (originPackedId == null) {
      return null;
    }
    int originVirtualId = ReflectionAccessors.getVirtualNodeId(originPackedId);
    return originToFlutterId.get(new ViewAndId(embeddedView, originVirtualId));
  }

  /**
   * Delegates a View#onHoverEvent event from the AccessibilityBridge to an embedded view.
   *
   * <p>The pointer coordinates are translated to the embedded view's coordinate system.
   */
  public boolean onAccessibilityHoverEvent(int rootFlutterId, @NonNull MotionEvent event) {
    ViewAndId origin = flutterIdToOrigin.get(rootFlutterId);
    if (origin == null) {
      return false;
    }
    Rect displayBounds = embeddedViewToDisplayBounds.get(origin.view);
    int pointerCount = event.getPointerCount();
    MotionEvent.PointerProperties[] pointerProperties =
        new MotionEvent.PointerProperties[pointerCount];
    MotionEvent.PointerCoords[] pointerCoords = new MotionEvent.PointerCoords[pointerCount];
    for (int i = 0; i < event.getPointerCount(); i++) {
      pointerProperties[i] = new MotionEvent.PointerProperties();
      event.getPointerProperties(i, pointerProperties[i]);

      MotionEvent.PointerCoords originCoords = new MotionEvent.PointerCoords();
      event.getPointerCoords(i, originCoords);

      pointerCoords[i] = new MotionEvent.PointerCoords(originCoords);
      pointerCoords[i].x -= displayBounds.left;
      pointerCoords[i].y -= displayBounds.top;
    }
    MotionEvent translatedEvent =
        MotionEvent.obtain(
            event.getDownTime(),
            event.getEventTime(),
            event.getAction(),
            event.getPointerCount(),
            pointerProperties,
            pointerCoords,
            event.getMetaState(),
            event.getButtonState(),
            event.getXPrecision(),
            event.getYPrecision(),
            event.getDeviceId(),
            event.getEdgeFlags(),
            event.getSource(),
            event.getFlags());
    return origin.view.dispatchGenericMotionEvent(translatedEvent);
  }

  /**
   * Returns the View that contains the accessibility node identified by the provided flutterId or
   * null if it doesn't belong to a view.
   */
  public View platformViewOfNode(int flutterId) {
    ViewAndId viewAndId = flutterIdToOrigin.get(flutterId);
    if (viewAndId == null) {
      return null;
    }
    return viewAndId.view;
  }

  private static class ViewAndId {
    final View view;
    final int id;

    private ViewAndId(View view, int id) {
      this.view = view;
      this.id = id;
    }

    @Override
    public boolean equals(Object o) {
      if (this == o) return true;
      if (!(o instanceof ViewAndId)) return false;
      ViewAndId viewAndId = (ViewAndId) o;
      return id == viewAndId.id && view.equals(viewAndId.view);
    }

    @Override
    public int hashCode() {
      final int prime = 31;
      int result = 1;
      result = prime * result + view.hashCode();
      result = prime * result + id;
      return result;
    }
  }

  private static class ReflectionAccessors {
    private @Nullable final Method getSourceNodeId;
    private @Nullable final Method getParentNodeId;
    private @Nullable final Method getRecordSourceNodeId;
    private @Nullable final Method getChildId;
    private @Nullable final Field childNodeIdsField;
    private @Nullable final Method longArrayGetIndex;

    @SuppressLint("DiscouragedPrivateApi,PrivateApi")
    private ReflectionAccessors() {
      Method getSourceNodeId = null;
      Method getParentNodeId = null;
      Method getRecordSourceNodeId = null;
      Method getChildId = null;
      Field childNodeIdsField = null;
      Method longArrayGetIndex = null;
      try {
        getSourceNodeId = AccessibilityNodeInfo.class.getMethod("getSourceNodeId");
      } catch (NoSuchMethodException e) {
        Log.w(TAG, "can't invoke AccessibilityNodeInfo#getSourceNodeId with reflection");
      }
      try {
        getRecordSourceNodeId = AccessibilityRecord.class.getMethod("getSourceNodeId");
      } catch (NoSuchMethodException e) {
        Log.w(TAG, "can't invoke AccessibiiltyRecord#getSourceNodeId with reflection");
      }
      // Reflection access is not allowed starting Android P on these methods.
      if (Build.VERSION.SDK_INT <= Build.VERSION_CODES.O) {
        try {
          getParentNodeId = AccessibilityNodeInfo.class.getMethod("getParentNodeId");
        } catch (NoSuchMethodException e) {
          Log.w(TAG, "can't invoke getParentNodeId with reflection");
        }
        // Starting P we extract the child id from the mChildNodeIds field (see getChildId
        // below).
        try {
          getChildId = AccessibilityNodeInfo.class.getMethod("getChildId", int.class);
        } catch (NoSuchMethodException e) {
          Log.w(TAG, "can't invoke getChildId with reflection");
        }
      } else {
        try {
          childNodeIdsField = AccessibilityNodeInfo.class.getDeclaredField("mChildNodeIds");
          childNodeIdsField.setAccessible(true);
          // The private member is a private utility class to Android. We need to use
          // reflection to actually handle the data too.
          longArrayGetIndex = Class.forName("android.util.LongArray").getMethod("get", int.class);
        } catch (NoSuchFieldException
            | ClassNotFoundException
            | NoSuchMethodException
            | NullPointerException e) {
          Log.w(TAG, "can't access childNodeIdsField with reflection");
          childNodeIdsField = null;
        }
      }
      this.getSourceNodeId = getSourceNodeId;
      this.getParentNodeId = getParentNodeId;
      this.getRecordSourceNodeId = getRecordSourceNodeId;
      this.getChildId = getChildId;
      this.childNodeIdsField = childNodeIdsField;
      this.longArrayGetIndex = longArrayGetIndex;
    }

    /** Returns virtual node ID given packed node ID used internally in accessibility API. */
    private static int getVirtualNodeId(long nodeId) {
      return (int) (nodeId >> 32);
    }

    @Nullable
    private Long getSourceNodeId(@NonNull AccessibilityNodeInfo node) {
      if (getSourceNodeId == null) {
        return null;
      }
      try {
        return (Long) getSourceNodeId.invoke(node);
      } catch (IllegalAccessException e) {
        Log.w(TAG, "Failed to access getSourceNodeId method.", e);
      } catch (InvocationTargetException e) {
        Log.w(TAG, "The getSourceNodeId method threw an exception when invoked.", e);
      }
      return null;
    }

    @Nullable
    private Long getChildId(@NonNull AccessibilityNodeInfo node, int child) {
      if (getChildId == null && (childNodeIdsField == null || longArrayGetIndex == null)) {
        return null;
      }
      if (getChildId != null) {
        try {
          return (Long) getChildId.invoke(node, child);
          // Using identical separate catch blocks to comply with the following lint:
          // Error: Multi-catch with these reflection exceptions requires API level 19
          // (current min is 16) because they get compiled to the common but new super
          // type ReflectiveOperationException. As a workaround either create individual
          // catch statements, or catch Exception. [NewApi]
        } catch (IllegalAccessException e) {
          Log.w(TAG, "Failed to access getChildId method.", e);
        } catch (InvocationTargetException e) {
          Log.w(TAG, "The getChildId method threw an exception when invoked.", e);
        }
      } else {
        try {
          return (long) longArrayGetIndex.invoke(childNodeIdsField.get(node), child);
          // Using identical separate catch blocks to comply with the following lint:
          // Error: Multi-catch with these reflection exceptions requires API level 19
          // (current min is 16) because they get compiled to the common but new super
          // type ReflectiveOperationException. As a workaround either create individual
          // catch statements, or catch Exception. [NewApi]
        } catch (IllegalAccessException e) {
          Log.w(TAG, "Failed to access longArrayGetIndex method or the childNodeId field.", e);
        } catch (InvocationTargetException | ArrayIndexOutOfBoundsException e) {
          Log.w(TAG, "The longArrayGetIndex method threw an exception when invoked.", e);
        }
      }
      return null;
    }

    @Nullable
    private Long getParentNodeId(@NonNull AccessibilityNodeInfo node) {
      if (getParentNodeId != null) {
        try {
          return (long) getParentNodeId.invoke(node);
          // Using identical separate catch blocks to comply with the following lint:
          // Error: Multi-catch with these reflection exceptions requires API level 19
          // (current min is 16) because they get compiled to the common but new super
          // type ReflectiveOperationException. As a workaround either create individual
          // catch statements, or catch Exception. [NewApi]
        } catch (IllegalAccessException e) {
          Log.w(TAG, "Failed to access getParentNodeId method.", e);
        } catch (InvocationTargetException e) {
          Log.w(TAG, "The getParentNodeId method threw an exception when invoked.", e);
        }
      }

      // Fall back on reading the ID from a serialized data if we absolutely have to.
      return yoinkParentIdFromParcel(node);
    }

    // If this looks like it's failing, that's because it probably is. This method is relying on
    // the implementation details of `AccessibilityNodeInfo#writeToParcel` in order to find the
    // particular bit in the opaque parcel that represents mParentNodeId. If the implementation
    // details change from our assumptions in this method, this will silently break.
    @Nullable
    private static Long yoinkParentIdFromParcel(AccessibilityNodeInfo node) {
      if (Build.VERSION.SDK_INT < Build.VERSION_CODES.O) {
        Log.w(TAG, "Unexpected Android version. Unable to find the parent ID.");
        return null;
      }

      // We're creating a copy here because writing a node to a parcel recycles it. Objects
      // are passed by reference in Java. So even though this method doesn't seem to use the
      // node again, it's really used in other methods that would throw exceptions if we
      // recycle it here.
      AccessibilityNodeInfo copy = AccessibilityNodeInfo.obtain(node);
      final Parcel parcel = Parcel.obtain();
      parcel.setDataPosition(0);
      copy.writeToParcel(parcel, /*flags=*/ 0);
      Long parentNodeId = null;
      // Match the internal logic that sets where mParentId actually ends up finally living.
      // This logic should match
      // https://android.googlesource.com/platform/frameworks/base/+/0b5ca24a4/core/java/android/view/accessibility/AccessibilityNodeInfo.java#3524.
      parcel.setDataPosition(0);
      long nonDefaultFields = parcel.readLong();
      int fieldIndex = 0;
      if (isBitSet(nonDefaultFields, fieldIndex++)) {
        parcel.readInt(); // mIsSealed
      }
      if (isBitSet(nonDefaultFields, fieldIndex++)) {
        parcel.readLong(); // mSourceNodeId
      }
      if (isBitSet(nonDefaultFields, fieldIndex++)) {
        parcel.readInt(); // mWindowId
      }
      if (isBitSet(nonDefaultFields, fieldIndex++)) {
        parentNodeId = parcel.readLong();
      }

      parcel.recycle();
      return parentNodeId;
    }

    private static boolean isBitSet(long flags, int bitIndex) {
      return (flags & (1L << bitIndex)) != 0;
    }

    @Nullable
    private Long getRecordSourceNodeId(@NonNull AccessibilityRecord node) {
      if (getRecordSourceNodeId == null) {
        return null;
      }
      try {
        return (Long) getRecordSourceNodeId.invoke(node);
      } catch (IllegalAccessException e) {
        Log.w(TAG, "Failed to access the getRecordSourceNodeId method.", e);
      } catch (InvocationTargetException e) {
        Log.w(TAG, "The getRecordSourceNodeId method threw an exception when invoked.", e);
      }
      return null;
    }
  }
}
