// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file provides weak pointers and weak pointer factories that work like
// Chromium's |base::WeakPtr<T>| and |base::WeakPtrFactory<T>|.

#ifndef FLUTTER_FML_MEMORY_WEAK_PTR_H_
#define FLUTTER_FML_MEMORY_WEAK_PTR_H_

#include <utility>

#include "flutter/fml/logging.h"
#include "flutter/fml/memory/ref_counted.h"
#include "flutter/fml/memory/task_runner_checker.h"
#include "flutter/fml/memory/thread_checker.h"
#include "flutter/fml/memory/weak_ptr_internal.h"

namespace fml {

struct DebugThreadChecker {
  FML_DECLARE_THREAD_CHECKER(checker);
};

struct DebugTaskRunnerChecker {
  FML_DECLARE_TASK_RUNNER_CHECKER(checker);
};

// Forward declaration, so |WeakPtr<T>| can friend it.
template <typename T>
class WeakPtrFactory;

// Class for "weak pointers" that can be invalidated. Valid weak pointers can
// only originate from a |WeakPtrFactory| (see below), though weak pointers are
// copyable and movable.
//
// Weak pointers are not in general thread-safe. They may only be *used* on a
// single thread, namely the same thread as the "originating" |WeakPtrFactory|
// (which can invalidate the weak pointers that it generates).
//
// However, weak pointers may be passed to other threads, reset on other
// threads, or destroyed on other threads. They may also be reassigned on other
// threads (in which case they should then only be used on the thread
// corresponding to the new "originating" |WeakPtrFactory|).
template <typename T>
class WeakPtr {
 public:
  WeakPtr() : ptr_(nullptr) {}

  // Copy constructor.
  explicit WeakPtr(const WeakPtr<T>& r) = default;

  template <typename U>
  WeakPtr(const WeakPtr<U>& r)
      : ptr_(static_cast<T*>(r.ptr_)), flag_(r.flag_), checker_(r.checker_) {}

  // Move constructor.
  WeakPtr(WeakPtr<T>&& r) = default;

  template <typename U>
  WeakPtr(WeakPtr<U>&& r)
      : ptr_(static_cast<T*>(r.ptr_)),
        flag_(std::move(r.flag_)),
        checker_(r.checker_) {}

  virtual ~WeakPtr() = default;

  // The following methods are thread-friendly, in the sense that they may be
  // called subject to additional synchronization.

  // Copy assignment.
  WeakPtr<T>& operator=(const WeakPtr<T>& r) = default;

  // Move assignment.
  WeakPtr<T>& operator=(WeakPtr<T>&& r) = default;

  void reset() { flag_ = nullptr; }

  // The following methods should only be called on the same thread as the
  // "originating" |WeakPtrFactory|.

  explicit operator bool() const {
    CheckThreadSafety();
    return flag_ && flag_->is_valid();
  }

  T* get() const {
    CheckThreadSafety();
    return *this ? ptr_ : nullptr;
  }

  // TODO(gw280): Remove all remaining usages of getUnsafe().
  // No new usages of getUnsafe() are allowed.
  //
  // https://github.com/flutter/flutter/issues/42949
  T* getUnsafe() const {
    // This is an unsafe method to get access to the raw pointer.
    // We still check the flag_ to determine if the pointer is valid
    // but callees should note that this WeakPtr could have been
    // invalidated on another thread.
    return flag_ && flag_->is_valid() ? ptr_ : nullptr;
  }

  T& operator*() const {
    CheckThreadSafety();
    FML_DCHECK(*this);
    return *get();
  }

  T* operator->() const {
    CheckThreadSafety();
    FML_DCHECK(*this);
    return get();
  }

 protected:
  explicit WeakPtr(T* ptr, fml::RefPtr<fml::internal::WeakPtrFlag>&& flag)
      : ptr_(ptr), flag_(std::move(flag)) {}

  T* ptr_;
  fml::RefPtr<fml::internal::WeakPtrFlag> flag_;

  virtual void CheckThreadSafety() const {
    FML_DCHECK_CREATION_THREAD_IS_CURRENT(checker_.checker);
  }

 private:
  template <typename U>
  friend class WeakPtr;

  friend class WeakPtrFactory<T>;

  explicit WeakPtr(T* ptr,
                   fml::RefPtr<fml::internal::WeakPtrFlag>&& flag,
                   DebugThreadChecker checker)
      : ptr_(ptr), flag_(std::move(flag)), checker_(checker) {}

  DebugThreadChecker checker_;

  // Copy/move construction/assignment supported.
};

// A weak pointer that can be used in different threads as long as
// the threads are belong to the same |TaskRunner|.
//
// It is still not in general thread safe as |WeakPtr|.
template <typename T>
class TaskRunnerAffineWeakPtr : public WeakPtr<T> {
 public:
  TaskRunnerAffineWeakPtr() : WeakPtr<T>() {}

  TaskRunnerAffineWeakPtr(const TaskRunnerAffineWeakPtr<T>& r) = default;

  template <typename U>
  TaskRunnerAffineWeakPtr(const TaskRunnerAffineWeakPtr<U>& r)
      : WeakPtr<T>(static_cast<T*>(r.ptr_), r.flag_), checker_(r.checker_) {}

  TaskRunnerAffineWeakPtr(TaskRunnerAffineWeakPtr<T>&& r) = default;

  template <typename U>
  TaskRunnerAffineWeakPtr(TaskRunnerAffineWeakPtr<U>&& r)
      : WeakPtr<T>(static_cast<T*>(r.ptr_), std::move(r.flag_)),
        checker_(r.checker_) {}

  ~TaskRunnerAffineWeakPtr() = default;

  TaskRunnerAffineWeakPtr<T>& operator=(const TaskRunnerAffineWeakPtr<T>& r) =
      default;

  TaskRunnerAffineWeakPtr<T>& operator=(TaskRunnerAffineWeakPtr<T>&& r) =
      default;

 protected:
  void CheckThreadSafety() const override {
    FML_DCHECK_TASK_RUNNER_IS_CURRENT(checker_.checker);
  }

 private:
  friend class WeakPtrFactory<T>;

  explicit TaskRunnerAffineWeakPtr(
      T* ptr,
      fml::RefPtr<fml::internal::WeakPtrFlag>&& flag,
      DebugTaskRunnerChecker checker)
      : WeakPtr<T>(ptr, std::move(flag)), checker_(checker) {}

  DebugTaskRunnerChecker checker_;
};

// Class that produces (valid) |WeakPtr<T>|s. Typically, this is used as a
// member variable of |T| (preferably the last one -- see below), and |T|'s
// methods control how weak pointers to it are vended. This class is not
// thread-safe, and should only be created, destroyed and used on a single
// thread.
//
// Example:
//
//  class Controller {
//   public:
//    Controller() : ..., weak_factory_(this) {}
//    ...
//
//    void SpawnWorker() { Worker::StartNew(weak_factory_.GetWeakPtr()); }
//    void WorkComplete(const Result& result) { ... }
//
//   private:
//    ...
//
//    // Member variables should appear before the |WeakPtrFactory|, to ensure
//    // that any |WeakPtr|s to |Controller| are invalidated before its member
//    // variables' destructors are executed.
//    WeakPtrFactory<Controller> weak_factory_;
//  };
//
//  class Worker {
//   public:
//    static void StartNew(const WeakPtr<Controller>& controller) {
//      Worker* worker = new Worker(controller);
//      // Kick off asynchronous processing....
//    }
//
//   private:
//    Worker(const WeakPtr<Controller>& controller) : controller_(controller) {}
//
//    void DidCompleteAsynchronousProcessing(const Result& result) {
//      if (controller_)
//        controller_->WorkComplete(result);
//    }
//
//    WeakPtr<Controller> controller_;
//  };
template <typename T>
class WeakPtrFactory {
 public:
  explicit WeakPtrFactory(T* ptr)
      : ptr_(ptr), flag_(fml::MakeRefCounted<fml::internal::WeakPtrFlag>()) {
    FML_DCHECK(ptr_);
  }

  ~WeakPtrFactory() {
    CheckThreadSafety();
    flag_->Invalidate();
  }

  // Gets a new weak pointer, which will be valid until either
  // |InvalidateWeakPtrs()| is called or this object is destroyed.
  WeakPtr<T> GetWeakPtr() const {
    return WeakPtr<T>(ptr_, flag_.Clone(), checker_);
  }

  // Gets a new weak pointer, which will be valid until either
  // |InvalidateWeakPtrs()| is called or this object is destroyed.
  TaskRunnerAffineWeakPtr<T> GetTaskRunnerAffineWeakPtr() const {
    return TaskRunnerAffineWeakPtr<T>(ptr_, flag_.Clone(),
                                      task_runner_checker_);
  }

 protected:
  // Note: See weak_ptr_internal.h for an explanation of why we store the
  // pointer here, instead of in the "flag".
  T* const ptr_;
  fml::RefPtr<fml::internal::WeakPtrFlag> flag_;

  void CheckThreadSafety() const {
    FML_DCHECK_CREATION_THREAD_IS_CURRENT(checker_.checker);
  }

 private:
  DebugThreadChecker checker_;
  DebugTaskRunnerChecker task_runner_checker_;

  FML_DISALLOW_COPY_AND_ASSIGN(WeakPtrFactory);
};

}  // namespace fml

#endif  // FLUTTER_FML_MEMORY_WEAK_PTR_H_
