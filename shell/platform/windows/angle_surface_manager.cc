// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/windows/angle_surface_manager.h"

#include <iostream>
#include <vector>

#ifdef WINUWP
#include <windows.ui.core.h>
#include <winrt/Windows.UI.Composition.h>
#endif

#if defined(WINUWP) && defined(USECOREWINDOW)
#include <winrt/Windows.UI.Core.h>
#endif

// Logs an EGL error to stderr. This automatically calls eglGetError()
// and logs the error code.
static void LogEglError(std::string message) {
  EGLint error = eglGetError();
  std::cerr << "EGL: " << message << std::endl;
  std::cerr << "EGL: eglGetError returned " << error << std::endl;
}

namespace flutter {

AngleSurfaceManager::AngleSurfaceManager()
    : egl_config_(nullptr),
      egl_display_(EGL_NO_DISPLAY),
      egl_context_(EGL_NO_CONTEXT) {
  initialize_succeeded_ = Initialize();
}

AngleSurfaceManager::~AngleSurfaceManager() {
  CleanUp();
}

bool AngleSurfaceManager::InitializeEGL(
    PFNEGLGETPLATFORMDISPLAYEXTPROC egl_get_platform_display_EXT,
    const EGLint* config,
    bool should_log) {
  egl_display_ = egl_get_platform_display_EXT(EGL_PLATFORM_ANGLE_ANGLE,
                                              EGL_DEFAULT_DISPLAY, config);

  if (egl_display_ == EGL_NO_DISPLAY) {
    if (should_log) {
      LogEglError("Failed to get a compatible EGLdisplay");
    }
    return false;
  }

  if (eglInitialize(egl_display_, nullptr, nullptr) == EGL_FALSE) {
    if (should_log) {
      LogEglError("Failed to initialize EGL via ANGLE");
    }
    return false;
  }

  return true;
}

bool AngleSurfaceManager::Initialize() {
  const EGLint config_attributes[] = {EGL_RED_SIZE,   8, EGL_GREEN_SIZE,   8,
                                      EGL_BLUE_SIZE,  8, EGL_ALPHA_SIZE,   8,
                                      EGL_DEPTH_SIZE, 8, EGL_STENCIL_SIZE, 8,
                                      EGL_NONE};

  const EGLint display_context_attributes[] = {EGL_CONTEXT_CLIENT_VERSION, 2,
                                               EGL_NONE};

  // These are prefered display attributes and request ANGLE's D3D11
  // renderer. eglInitialize will only succeed with these attributes if the
  // hardware supports D3D11 Feature Level 10_0+.
  const EGLint d3d11_display_attributes[] = {
      EGL_PLATFORM_ANGLE_TYPE_ANGLE,
      EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE,

      // EGL_PLATFORM_ANGLE_ENABLE_AUTOMATIC_TRIM_ANGLE is an option that will
      // enable ANGLE to automatically call the IDXGIDevice3::Trim method on
      // behalf of the application when it gets suspended.
      EGL_PLATFORM_ANGLE_ENABLE_AUTOMATIC_TRIM_ANGLE,
      EGL_TRUE,
      EGL_NONE,
  };

  // These are used to request ANGLE's D3D11 renderer, with D3D11 Feature
  // Level 9_3.
  const EGLint d3d11_fl_9_3_display_attributes[] = {
      EGL_PLATFORM_ANGLE_TYPE_ANGLE,
      EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE,
      EGL_PLATFORM_ANGLE_MAX_VERSION_MAJOR_ANGLE,
      9,
      EGL_PLATFORM_ANGLE_MAX_VERSION_MINOR_ANGLE,
      3,
      EGL_PLATFORM_ANGLE_ENABLE_AUTOMATIC_TRIM_ANGLE,
      EGL_TRUE,
      EGL_NONE,
  };

  // These attributes request D3D11 WARP (software rendering fallback) in case
  // hardware-backed D3D11 is unavailable.
  const EGLint d3d11_warp_display_attributes[] = {
      EGL_PLATFORM_ANGLE_TYPE_ANGLE,
      EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE,
      EGL_PLATFORM_ANGLE_ENABLE_AUTOMATIC_TRIM_ANGLE,
      EGL_TRUE,
      EGL_NONE,
  };

  // These are used to request ANGLE's D3D9 renderer as a fallback if D3D11
  // is not available.
  const EGLint d3d9_display_attributes[] = {
      EGL_PLATFORM_ANGLE_TYPE_ANGLE,
      EGL_PLATFORM_ANGLE_TYPE_D3D9_ANGLE,
      EGL_TRUE,
      EGL_NONE,
  };

  std::vector<const EGLint*> display_attributes_configs = {
      d3d11_display_attributes,
      d3d11_fl_9_3_display_attributes,
      d3d11_warp_display_attributes,
      d3d9_display_attributes,
  };

  PFNEGLGETPLATFORMDISPLAYEXTPROC egl_get_platform_display_EXT =
      reinterpret_cast<PFNEGLGETPLATFORMDISPLAYEXTPROC>(
          eglGetProcAddress("eglGetPlatformDisplayEXT"));
  if (!egl_get_platform_display_EXT) {
    LogEglError("eglGetPlatformDisplayEXT not available");
    return false;
  }

  // Attempt to initialize ANGLE's renderer in order of: D3D11, D3D11 Feature
  // Level 9_3, D3D11 WARP and finally D3D9.
  for (auto config : display_attributes_configs) {
    bool should_log = (config == display_attributes_configs.back());
    if (InitializeEGL(egl_get_platform_display_EXT, config, should_log)) {
      break;
    }
  }

  EGLint numConfigs = 0;
  if ((eglChooseConfig(egl_display_, config_attributes, &egl_config_, 1,
                       &numConfigs) == EGL_FALSE) ||
      (numConfigs == 0)) {
    LogEglError("Failed to choose first context");
    return false;
  }

  egl_context_ = eglCreateContext(egl_display_, egl_config_, EGL_NO_CONTEXT,
                                  display_context_attributes);
  if (egl_context_ == EGL_NO_CONTEXT) {
    LogEglError("Failed to create EGL context");
    return false;
  }

  egl_resource_context_ = eglCreateContext(
      egl_display_, egl_config_, egl_context_, display_context_attributes);

  if (egl_resource_context_ == EGL_NO_CONTEXT) {
    LogEglError("Failed to create EGL resource context");
    return false;
  }

  return true;
}

void AngleSurfaceManager::CleanUp() {
  EGLBoolean result = EGL_FALSE;

  if (egl_display_ != EGL_NO_DISPLAY && egl_context_ != EGL_NO_CONTEXT) {
    result = eglDestroyContext(egl_display_, egl_context_);
    egl_context_ = EGL_NO_CONTEXT;

    if (result == EGL_FALSE) {
      LogEglError("Failed to destroy context");
    }
  }

  if (egl_display_ != EGL_NO_DISPLAY &&
      egl_resource_context_ != EGL_NO_CONTEXT) {
    result = eglDestroyContext(egl_display_, egl_resource_context_);
    egl_resource_context_ = EGL_NO_CONTEXT;

    if (result == EGL_FALSE) {
      LogEglError("Failed to destroy resource context");
    }
  }

  if (egl_display_ != EGL_NO_DISPLAY) {
    eglTerminate(egl_display_);
    egl_display_ = EGL_NO_DISPLAY;
  }
}

bool AngleSurfaceManager::CreateSurface(WindowsRenderTarget* render_target,
                                        EGLint width,
                                        EGLint height) {
  if (!render_target || !initialize_succeeded_) {
    return false;
  }

  EGLSurface surface = EGL_NO_SURFACE;

  const EGLint surfaceAttributes[] = {EGL_NONE};

#ifdef WINUWP
#ifdef USECOREWINDOW
  auto target = std::get<winrt::Windows::UI::Core::CoreWindow>(*render_target);
#else
  auto target =
      std::get<winrt::Windows::UI::Composition::SpriteVisual>(*render_target);
#endif
  surface = eglCreateWindowSurface(
      egl_display_, egl_config_,
      static_cast<EGLNativeWindowType>(winrt::get_abi(target)),
      surfaceAttributes);
#else
  surface = eglCreateWindowSurface(
      egl_display_, egl_config_,
      static_cast<EGLNativeWindowType>(std::get<HWND>(*render_target)),
      surfaceAttributes);
#endif
  if (surface == EGL_NO_SURFACE) {
    LogEglError("Surface creation failed.");
  }

  render_surface_ = surface;
  return true;
}

void AngleSurfaceManager::ResizeSurface(WindowsRenderTarget* render_target,
                                        EGLint width,
                                        EGLint height) {
  EGLint existing_width, existing_height;
  GetSurfaceDimensions(&existing_width, &existing_height);
  if (width != existing_width || height != existing_height) {
    // Resize render_surface_.  Internaly this calls mSwapChain->ResizeBuffers
    // avoiding the need to destory and recreate the underlying SwapChain.
    eglPostSubBufferNV(egl_display_, render_surface_, 1, 1, width, height);
  }
}

void AngleSurfaceManager::GetSurfaceDimensions(EGLint* width, EGLint* height) {
  if (render_surface_ == EGL_NO_SURFACE || !initialize_succeeded_) {
    width = 0;
    height = 0;
    return;
  }

  eglQuerySurface(egl_display_, render_surface_, EGL_WIDTH, width);
  eglQuerySurface(egl_display_, render_surface_, EGL_HEIGHT, height);
}

void AngleSurfaceManager::DestroySurface() {
  if (egl_display_ != EGL_NO_DISPLAY && render_surface_ != EGL_NO_SURFACE) {
    eglDestroySurface(egl_display_, render_surface_);
  }
  render_surface_ = EGL_NO_SURFACE;
}

bool AngleSurfaceManager::MakeCurrent() {
  return (eglMakeCurrent(egl_display_, render_surface_, render_surface_,
                         egl_context_) == EGL_TRUE);
}

bool AngleSurfaceManager::ClearContext() {
  return (eglMakeCurrent(egl_display_, nullptr, nullptr, egl_context_) ==
          EGL_TRUE);
}

bool AngleSurfaceManager::MakeResourceCurrent() {
  return (eglMakeCurrent(egl_display_, EGL_NO_SURFACE, EGL_NO_SURFACE,
                         egl_resource_context_) == EGL_TRUE);
}

EGLBoolean AngleSurfaceManager::SwapBuffers() {
  return (eglSwapBuffers(egl_display_, render_surface_));
}

}  // namespace flutter
