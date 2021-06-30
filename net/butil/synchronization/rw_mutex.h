// Copyright (c) 2018 nori, Inc All Rights Reserverd
// Author: Zhenwei Zhu losted.leo@gmail.com

#ifndef BUTIL_SYNCHRONIZATION_RW_MUTEX_H
#define BUTIL_SYNCHRONIZATION_RW_MUTEX_H

#include <sys/time.h>
#include <pthread.h>
#include <iostream>

#include "butil/macros.h"

namespace butil {

class RWMutex {
 public:
  RWMutex();
  ~RWMutex();

  void Lock();
  bool TryLock();
  void Unlock();

  void ReaderLock();
  bool ReaderTryLock();
  void ReaderUnlock() { Unlock(); }

  void WriterLock() { Lock(); }
  bool WriterTryLock() { return TryLock(); }
  void WriterUnlock() { Unlock(); }

 private:
  pthread_rwlock_t raw_rwlock_;
  pthread_rwlockattr_t raw_rwlock_attr_;

  DISALLOW_COPY_AND_ASSIGN(RWMutex);
};

class ReaderAutoLock {
 public:
  explicit ReaderAutoLock(RWMutex *mutex)
    : mutex_(mutex) {
  mutex_->ReaderLock();
  }
  ~ReaderAutoLock() {
  mutex_->ReaderUnlock();
  }

 private:
  RWMutex *mutex_;

  DISALLOW_COPY_AND_ASSIGN(ReaderAutoLock);
};

class WriterAutoLock {
 public:
  explicit WriterAutoLock(RWMutex *mutex)
    : mutex_(mutex) {
  mutex_->Lock();
  }
  ~WriterAutoLock() {
  mutex_->Unlock();
  }

 private:
  RWMutex *mutex_;

  DISALLOW_COPY_AND_ASSIGN(WriterAutoLock);
};

}  // namespace butil
#endif
