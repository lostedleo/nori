#ifndef BUTIL_SYNCHRONIZATION_BLOCKING_QUEUE_H_
#define BUTIL_SYNCHRONIZATION_BLOCKING_QUEUE_H_

#include <stdlib.h>
#include <stdio.h>
#include <atomic>

#include <algorithm>
#include <deque>

#include "butil/synchronization/condition_variable.h"
#include "butil/time/time.h"

namespace butil {

// |BlockingQueue| 阻塞队列, 是线程安全的队列
//
// 主要支持如下操作：
//
// - Put       在队尾添加元素
// - Take      从队头取出元素, 若队列空，则一直等待，直到别的线程添加新元素
// - TryTake   同上 Take，但不等待，立即返回
// - TimedTake 同上 Take，但设置了超时
// - MultiTake    一次取出所有元素, 若队列空，则一直等待, 直到别的线程添加新元素
// - TryMultiTake 同上 MultiTake, 但不等待，立刻返回
// - Close     关闭队列，之后不再允许添加新元素，但允许取出之前已经添加的元素
//   ...
//
// 该类的所有方法都是线程安全的
template <class E>
class BlockingQueue {
 public:
  BlockingQueue();

  // 添加元素 |e| 到队列尾部
  //
  // 如果队列已经关闭，则返回 false；否则返回 true
  bool Put(E e);

  // 添加元素 |e| 到队列尾部
  //
  // 如果队列已经关闭，则返回 -1；
  // 否则返回 Put 之前, 队列中元素个数
  int PutEx(E e);

  // 从队列头部取出一个元素。
  //
  // 如果队列非空，则立即取出元素返回
  // 如果队列为空，且队列未被关闭，则一直等待，直到其他线程添加元素到队列 或 关闭队列
  // 如果队列为空，且队列已经关闭，则立即返回空元素，等价于 return E()
  E Take();
  // 如果队列非空，则立即取出元素返回 true
  // 如果队列为空，且队列未被关闭，则一直等待，直到其他线程添加元素到队列 或 关闭队列
  // 如果队列为空, 且队列已经关闭, 则返回 false
  bool Take(E* e);

  // 尝试从队列头部取出一个元素
  // 返回是否成功取得元素
  //
  // 如果队列非空，则立即取出队头元素，返回 1;
  // 如果队列为空，且未被关闭，则返回 0
  // 如果队列为空，且已被关闭，则返回 -1
  int TryTake(E *e);

  // 从队列头部取出一个元素, 最多阻塞 |ms| 毫秒;
  //
  // 如果成功取得元素, 则返回 1
  // 如果队列被关闭, 则返回 -1
  // 如果阻塞超过 |ms| 毫秒, 则返回 0
  //
  // NOTE: 超时时间的控制精度在 5~10 ms 左右, 设置太小的值没有意义
  int TimedTake(int ms, E *e);

  // 取出队列中已有的所有元素;
  // 会阻塞直到取出至少一个元素 或 队列被关闭;
  // 取出的元素会添加到 result 尾部.
  //
  // 如果队列非空，则返回取出的元素个数
  // 如果队列为空, 且已被关闭，则返回 -1
  // 如果队列为空, 且未被关闭，则一直等待，直到其他线程添加元素到队列 或 关闭队列, 并返回取出的元素个数
  int MultiTake(std::deque<E> *result);

  template <typename OutputIterator>
  int MultiTake(OutputIterator out);

  // 尝试取出队列中已有的所有元素;
  // 无论取出元素与否, 立即返回;
  // 取出的元素会添加到 result 尾部.
  //
  // 如果成功取得元素, 则返回取出的元素个数
  // 如果队列为空, 且已被关闭，则返回 -1
  // 如果队列为空, 但未被关闭，返回 0
  int TryMultiTake(std::deque<E> *result);

  template <typename OutputIterator>
  int TryMultiTake(OutputIterator out);

  int TakeElements(E events[], int size);

  // 尝试取出队列中已有的所有元素;
  // 会阻塞直到取出至少一个元素, 最多阻塞 |ms| 毫秒;
  // 取出的元素会添加到 result 尾部.
  //
  // 如果成功取得元素, 则返回取出的元素个数
  // 如果队列被关闭, 则返回 -1
  // 如果阻塞超过 |ms| 毫秒, 则返回 0
  //
  // NOTE: 超时时间的控制精度在 5~10 ms 左右, 设置太小的值没有意义
  int TimedMultiTake(int ms, std::deque<E> *result);

  // 返回当下队列中元素个数
  //
  // 注意：如果有多个线程在操作该队列，元素个数随时会变
  int Size() const;

  // 判断队列是否为空
  //
  // 注意：如果有多个线程在操作该队列，元素个数随时会变
  bool Empty() const;

  // 关闭队列，不允许再添加新元素，但允许取出之前添加的元素
  // 可以多次调用该方法, 无副作用
  //
  // 注意：队列被关闭后，不能再被重新打开
  void Close();

  // 队列是否已经关闭
  bool Closed();

 private:
  // The queue of elements. Deque is used to provide O(1) time
  // for head elements removal.
  std::deque<E> queue_;
  std::atomic<bool> closed_;

  // The Lock used for queue synchronization.
  mutable Lock lock_;

  // The conditionial variable associated with the Lock above.
  mutable ConditionVariable cond_var_;

  DISALLOW_COPY_AND_ASSIGN(BlockingQueue);
};

template<class E>
BlockingQueue<E>::BlockingQueue()
  : closed_(false),
    cond_var_(&lock_) {
}

template<class E>
int BlockingQueue<E>::Size() const {
  AutoLock lock(lock_);
  return queue_.size();
}

template<class E>
bool BlockingQueue<E>::Empty() const {
  AutoLock lock(lock_);
  return queue_.empty();
}

template<class E>
bool BlockingQueue<E>::Put(E e) {
  AutoLock lock(lock_);

  if (closed_) {
    return false;
  }

  queue_.push_back(e);
  cond_var_.Signal();
  return true;
}

template<class E>
int BlockingQueue<E>::PutEx(E e) {
  AutoLock lock(lock_);

  if (closed_) {
    return -1;
  }

  queue_.push_back(e);
  cond_var_.Signal();
  return queue_.size() - 1;
}

template<class E>
bool BlockingQueue<E>::Take(E* e) {
  AutoLock lock(lock_);

  while (queue_.empty()) {
    if (closed_) return false;
    cond_var_.Wait();
  }

  *e = queue_.front();
  queue_.pop_front();
  return true;
}

template<class E>
E BlockingQueue<E>::Take() {
  AutoLock lock(lock_);

  while (queue_.empty()) {
    if (closed_) return E();
    cond_var_.Wait();
  }

  E e = queue_.front();
  queue_.pop_front();
  return e;
}

template<class E>
int BlockingQueue<E>::TryTake(E *e) {
  AutoLock lock(lock_);

  if (queue_.empty()) {
    if (closed_) {
      return -1;
    } else {
      return 0;
    }
  }

  *e = queue_.front();
  queue_.pop_front();
  return 1;
}

// 从队列头部取出一个元素, 最多阻塞 |ms| 毫秒;
//
// 如果成功取得元素, 则返回 1
// 如果队列被关闭, 则返回 -1
// 如果阻塞超过 |ms| 毫秒, 则返回 0
//
// NOTE: 超时时间的控制精度在 5~10 ms 左右, 设置太小的值没有意义
template<class E>
int BlockingQueue<E>::TimedTake(int millisecond, E *e) {
  if (millisecond <= 0) {
    return this->TryTake(e);
  }

  AutoLock lock(lock_);

  while (queue_.empty()) {
    if (closed_) {
      // 队列关闭
      *e = E();
      return -1;
    }
    // NOTE: |millisecond| will be updated by TimedWait.
    if (!cond_var_.TimedWait(butil::TimeDelta::FromMilliseconds(millisecond))) {
      // 超时
      *e = E();
      return 0;
    }
  }

  // 成功取得元素
  *e = queue_.front();
  queue_.pop_front();
  return 1;
}

// 尝试取出队列中已有的所有元素;
// 无论取出元素与否, 立即返回;
// 取出的元素会添加到 result 尾部.
//
// 如果成功取得元素, 则返回取出的元素个数
// 如果队列为空, 且已被关闭，则返回 -1
// 如果队列为空, 但未被关闭，返回 0
template<class E>
int BlockingQueue<E>::TryMultiTake(std::deque<E> *result) {
  AutoLock lock(lock_);

  if (queue_.empty() && closed_) {
    return -1;
  }

  int ret = queue_.size();
  result->insert(result->end(), queue_.begin(), queue_.end());
  queue_.clear();

  return ret;
}

template <class E>
template <typename OutputIterator>
int BlockingQueue<E>::TryMultiTake(OutputIterator out) {
  AutoLock lock(lock_);

  if (queue_.empty() && closed_) {
    return -1;
  }

  int ret = queue_.size();
  std::copy(queue_.begin(), queue_.end(), out);
  queue_.clear();

  return ret;
}

template <class E>
template <typename OutputIterator>
int BlockingQueue<E>::MultiTake(OutputIterator out) {
  AutoLock lock(lock_);

  if (queue_.empty() && closed_) {
    return -1;
  }

  while (queue_.empty()) {
    if (closed_) return -1;
    cond_var_.Wait();
  }

  int ret = queue_.size();
  out->insert(out->end(), queue_.begin(), queue_.end());
  queue_.clear();

  return ret;
}

template<class E>
int BlockingQueue<E>::MultiTake(std::deque<E> *result) {
  AutoLock lock(lock_);

  if (queue_.empty() && closed_) {
    return -1;
  }

  while (queue_.empty()) {
    if (closed_) return -1;
    cond_var_.Wait();
  }

  int ret = queue_.size();
  result->insert(result->end(), queue_.begin(), queue_.end());
  queue_.clear();

  return ret;
}

template<class E>
int BlockingQueue<E>::TakeElements(E events[], int size) {
  AutoLock lock(lock_);

  if (queue_.empty() && closed_) {
    return -1;
  }

  while (queue_.empty()) {
    if (closed_) return -1;
    cond_var_.Wait();
  }

  int ret = static_cast<int>(queue_.size()) > size ? size : queue_.size();
  for (int i = 0; i < ret; ++i) {
    events[i] = queue_.front();
    queue_.pop_front();
  }

  return ret;
}

// 尝试取出队列中已有的所有元素;
// 会阻塞直到取出至少一个元素, 最多阻塞 |ms| 毫秒;
// 取出的元素会添加到 result 尾部.
//
// 如果成功取得元素, 则返回取出的元素个数
// 如果队列被关闭, 则返回 -1
// 如果阻塞超过 |ms| 毫秒, 则返回 0
//
// NOTE: 超时时间的控制精度在 5~10 ms 左右, 设置太小的值没有意义
template<class E>
int BlockingQueue<E>::TimedMultiTake(int millisecond, std::deque<E> *result) {
  if (millisecond <= 0) {
    return this->TryMultiTake(result);
  }

  AutoLock lock(lock_);

  while (queue_.empty()) {
    if (closed_) {
      // 队列关闭
      return -1;
    }
    // NOTE: |millisecond| will be updated by TimedWait.
    if (!cond_var_.TimedWait(butil::TimeDelta::FromMilliseconds(millisecond))) {
      // 超时
      return 0;
    }
  }

  // 成功取得元素
  int ret = queue_.size();
  result->insert(result->end(), queue_.begin(), queue_.end());
  queue_.clear();

  return ret;
}

template<class E>
void BlockingQueue<E>::Close() {
  AutoLock lock(lock_);

  if (closed_) {
    return;
  }

  closed_ = true;

  cond_var_.Broadcast();
}

template<class E>
bool BlockingQueue<E>::Closed() {
  AutoLock lock(lock_);
  return closed_;
}

}  // namespace extend
#endif  // BUTIL_SYNCHRONIZATION_BLOCKING_QUEUE_H_
