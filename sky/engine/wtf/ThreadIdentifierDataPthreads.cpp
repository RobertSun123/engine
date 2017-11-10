/*
 * Copyright (C) 2009, 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "flutter/sky/engine/wtf/ThreadIdentifierDataPthreads.h"

#include "flutter/sky/engine/wtf/Assertions.h"
#include "flutter/sky/engine/wtf/WTF.h"

#include <limits.h>

#if USE(PTHREADS)

// PTHREAD_KEYS_MAX might be not defined (e.g. in certain bionic versions), so
// explicitly define it here.
#ifndef PTHREAD_KEYS_MAX
#define PTHREAD_KEYS_MAX 1024
#endif  // PTHREAD_KEYS_MAX

namespace WTF {

pthread_key_t ThreadIdentifierData::m_key = PTHREAD_KEYS_MAX;

void threadDidExit(ThreadIdentifier);

ThreadIdentifierData::~ThreadIdentifierData() {
  threadDidExit(m_identifier);
}

void ThreadIdentifierData::initializeOnce() {
  if (pthread_key_create(&m_key, destruct))
    CRASH();
}

ThreadIdentifier ThreadIdentifierData::identifier() {
  ASSERT(m_key != PTHREAD_KEYS_MAX);
  ThreadIdentifierData* threadIdentifierData =
      static_cast<ThreadIdentifierData*>(pthread_getspecific(m_key));

  return threadIdentifierData ? threadIdentifierData->m_identifier : 0;
}

void ThreadIdentifierData::initialize(ThreadIdentifier id) {
  ASSERT(!identifier());
  pthread_setspecific(m_key, new ThreadIdentifierData(id));
}

void ThreadIdentifierData::destruct(void* data) {
  if (isShutdown())
    return;

  ThreadIdentifierData* threadIdentifierData =
      static_cast<ThreadIdentifierData*>(data);
  ASSERT(threadIdentifierData);

  if (threadIdentifierData->m_isDestroyedOnce) {
    delete threadIdentifierData;
    return;
  }

  threadIdentifierData->m_isDestroyedOnce = true;
  // Re-setting the value for key causes another destruct() call after all other
  // thread-specific destructors were called.
  pthread_setspecific(m_key, threadIdentifierData);
}

}  // namespace WTF

#endif  // USE(PTHREADS)
