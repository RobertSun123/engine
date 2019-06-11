// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#define FML_USED_ON_EMBEDDER

#include "flutter/fml/message_loop_impl.h"

#include <algorithm>
#include <vector>

#include "flutter/fml/build_config.h"
#include "flutter/fml/logging.h"
#include "flutter/fml/trace_event.h"

#if OS_MACOSX
#include "flutter/fml/platform/darwin/message_loop_darwin.h"
#elif OS_ANDROID
#include "flutter/fml/platform/android/message_loop_android.h"
#elif OS_LINUX
#include "flutter/fml/platform/linux/message_loop_linux.h"
#elif OS_WIN
#include "flutter/fml/platform/win/message_loop_win.h"
#endif

namespace fml {

fml::RefPtr<MessageLoopImpl> MessageLoopImpl::Create() {
#if OS_MACOSX
  return fml::MakeRefCounted<MessageLoopDarwin>();
#elif OS_ANDROID
  return fml::MakeRefCounted<MessageLoopAndroid>();
#elif OS_LINUX
  return fml::MakeRefCounted<MessageLoopLinux>();
#elif OS_WIN
  return fml::MakeRefCounted<MessageLoopWin>();
#else
  return nullptr;
#endif
}

MessageLoopImpl::MessageLoopImpl()
    : terminated_(false),
      task_queue_(MessageLoopTaskQueue::GetInstance()),
      loop_id_(task_queue_->CreateMessageLoopId()) {}

MessageLoopImpl::~MessageLoopImpl() = default;

void MessageLoopImpl::PostTask(fml::closure task, fml::TimePoint target_time) {
  FML_DCHECK(task != nullptr);
  RegisterTask(task, target_time);
}

void MessageLoopImpl::AddTaskObserver(intptr_t key, fml::closure callback) {
  FML_DCHECK(callback != nullptr);
  FML_DCHECK(MessageLoop::GetCurrent().GetLoopImpl().get() == this)
      << "Message loop task observer must be added on the same thread as the "
         "loop.";
  task_queue_->AddTaskObserver(loop_id_, key, std::move(callback));
}

void MessageLoopImpl::RemoveTaskObserver(intptr_t key) {
  FML_DCHECK(MessageLoop::GetCurrent().GetLoopImpl().get() == this)
      << "Message loop task observer must be removed from the same thread as "
         "the loop.";
  task_queue_->RemoveTaskObserver(loop_id_, key);
}

void MessageLoopImpl::DoRun() {
  if (terminated_) {
    // Message loops may be run only once.
    return;
  }

  // Allow the implementation to do its thing.
  Run();

  // The loop may have been implicitly terminated. This can happen if the
  // implementation supports termination via platform specific APIs or just
  // error conditions. Set the terminated flag manually.
  terminated_ = true;

  // The message loop is shutting down. Check if there are expired tasks. This
  // is the last chance for expired tasks to be serviced. Make sure the
  // terminated flag is already set so we don't accrue additional tasks now.
  RunExpiredTasksNow();

  // When the message loop is in the process of shutting down, pending tasks
  // should be destructed on the message loop's thread. We have just returned
  // from the implementations |Run| method which we know is on the correct
  // thread. Drop all pending tasks on the floor.
  task_queue_->DisposeTasks(loop_id_);
}

void MessageLoopImpl::DoTerminate() {
  terminated_ = true;
  Terminate();
}

// Thread safety analysis disabled as it does not account for defered locks.
void MessageLoopImpl::InheritAllTasks(
    const fml::RefPtr<MessageLoopImpl>& other) {
  if (terminated_ || other->terminated_) {
    return;
  }

  if (this == other.get()) {
    return;
  }

  bool success = task_queue_->MergeQueues(loop_id_, other->loop_id_);
  FML_CHECK(success) << "failed to merge task queues for " << loop_id_
                     << " and " << other->loop_id_;
}

void MessageLoopImpl::Unmerge() {
  bool success = task_queue_->UnmergeQueues(loop_id_);
  FML_CHECK(success) << "unable to unmerge loop: " << loop_id_;
}

void MessageLoopImpl::RegisterTask(fml::closure task,
                                   fml::TimePoint target_time) {
  FML_DCHECK(task != nullptr);
  if (terminated_) {
    // If the message loop has already been terminated, PostTask should destruct
    // |task| synchronously within this function.
    return;
  }
  task_queue_->RegisterTask(loop_id_, task, target_time);
}

void MessageLoopImpl::FlushTasks(FlushType type) {
  FML_TRACE_EVENT0("fml", "MessageLoop::FlushTasks");

  // We are grabbing this lock here as a proxy to indicate
  // that we are running tasks and will invoke the
  // "right" observers, we are trying to avoid the scenario
  // where:
  // gather invocations -> Swap -> execute invocations
  // will lead us to run invocations on the wrong thread.
  std::mutex& flush_tasks_mutex = *task_queue_->flush_tasks_mutexes[loop_id_];
  std::lock_guard<std::mutex> task_flush_lock(flush_tasks_mutex);

  const MessageLoopTaskQueue::TasksToRun& tasks =
      task_queue_->GetTasksToRunNow(loop_id_, type);
  WakeUp(tasks.wake_up_time);

  task_queue_->InvokeAndNotifyObservers(loop_id_, tasks);
}

void MessageLoopImpl::RunExpiredTasksNow() {
  FlushTasks(FlushType::kAll);
}

void MessageLoopImpl::RunSingleExpiredTaskNow() {
  FlushTasks(FlushType::kSingle);
}

}  // namespace fml
