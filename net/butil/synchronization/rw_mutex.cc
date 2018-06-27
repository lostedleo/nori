// Copyright (c) 2018 nori, Inc All Rights Reserverd
// Author: Zhenwei Zhu losted.leo@gmail.com

#include "butil/synchronization/rw_mutex.h"

#include <sched.h>

#include "butil/logging.h"
#include "butil/safe_strerror_posix.h"

#define CHECK_PTHREAD_CALL(pthread_func_call) \
    do { \
      int ret = pthread_func_call; \
      LOG_IF(FATAL, ret != 0)  \
      << "Failed to call " # pthread_func_call \
      << ": " << safe_strerror(ret);   \
    } while (false)

namespace butil {

// RWMutex, by default it's recursive, i.e., a single thread, which already owns
// the lock, can aquaire the lock again without being blocked.
RWMutex::RWMutex() {
  CHECK_PTHREAD_CALL(pthread_rwlockattr_init(&raw_rwlock_attr_));
  CHECK_PTHREAD_CALL(
      pthread_rwlockattr_setkind_np(&raw_rwlock_attr_, PTHREAD_MUTEX_RECURSIVE_NP));

  CHECK_PTHREAD_CALL(pthread_rwlock_init(&raw_rwlock_, &raw_rwlock_attr_));
}

RWMutex::~RWMutex() {
  CHECK_PTHREAD_CALL(pthread_rwlock_destroy(&raw_rwlock_));
  CHECK_PTHREAD_CALL(pthread_rwlockattr_destroy(&raw_rwlock_attr_));
}

void RWMutex::Lock() {
  CHECK_PTHREAD_CALL(pthread_rwlock_wrlock(&raw_rwlock_));
}

bool RWMutex::TryLock() {
  int ret = pthread_rwlock_trywrlock(&raw_rwlock_);
  if (ret == EBUSY) {
    return false;
  }
  LOG_IF(FATAL, ret != 0) << "pthread_rwlock_trylock failed: "
                          << safe_strerror(ret);
  return true;
}

void RWMutex::Unlock() {
  CHECK_PTHREAD_CALL(pthread_rwlock_unlock(&raw_rwlock_));
}

void RWMutex::ReaderLock() {
  CHECK_PTHREAD_CALL(pthread_rwlock_rdlock(&raw_rwlock_));
}

bool RWMutex::ReaderTryLock() {
  int ret = pthread_rwlock_tryrdlock(&raw_rwlock_);
  if (ret == EBUSY) {
    return false;
  }
  LOG_IF(FATAL, ret != 0) << "pthread_rwlock_trylock failed: "
                          << safe_strerror(ret);
  return true;
}

}  // namespace butil

