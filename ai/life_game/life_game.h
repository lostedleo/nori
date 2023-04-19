/*************************************************************************
  > File Name:    life_game.h
  > Author:       Zhu Zhenwei
  > Mail:         losted.leo@gmail.com
  > Created Time: 六  3/11 06:22:55 2017
 ************************************************************************/

#ifndef LIFE_GAME_H_
#define LIFE_GAME_H_

#include "butil/synchronization/lock.h"
#include "butil/threading/simple_thread.h"

#include <chrono>

namespace YiNuo {

typedef std::chrono::steady_clock::time_point time_point;

class LifeGame {
 public:

  explicit LifeGame(int x, int y);
  ~LifeGame();

  void Init(int number);
  void InitRandom(int number);
  void Start(bool print=false);
  void Stop();
  void Print();
  void CheckLifesByIndex(int start, int end);
  void TransformLifes(int start, int end);
  uint64_t CountLifes(int start, int end);

  int x() {return x_;}
  int y() {return y_;}
  int generation() {return generation_;}
  void inc_generation() {generation_++;}
  int init_number() {return init_number_;}
  uint64_t lived() {return lived_;}
  void set_lived(uint64_t lived) {lived_ = lived;}
  int same_count() {return same_count_;}
  void inc_same_count() {same_count_++;}
  void set_same_count(int val) {same_count_ = val;}
  time_point point() {return time_point_;}
  void set_point(time_point point) {time_point_ = point;}

 private:
  void CheckLifeEx(int x, int y, int life_condition);
  void CheckLife(int x, int y, int life_condition); void LifeThread(bool print);
  bool CheckAllExpired();

  bool  started_;
  int   life_condition_;
  int   init_number_;
  int   same_count_;
  uint64_t lived_;
  int   generation_;
  int   x_;
  int   y_;
  int** matrix_;
  time_point time_point_;
  butil::Lock lock_;
};

class LifeGameRunner {
 public:
  typedef butil::DelegateSimpleThread::Delegate Delegate;
  LifeGameRunner(LifeGame* game, int thread_num, int sleep_ms=0);
  ~LifeGameRunner();
  void Run(bool print);

 private:
  bool CheckAllExpired();

  LifeGame* game_;
  int sleep_time_;
  int thread_num_;
  butil::DelegateSimpleThreadPool* thread_pool_;
  std::vector<Delegate*> check_delegates_;
  std::vector<Delegate*> trans_delegates_;
  std::vector<Delegate*> count_delegates_;
  std::vector<uint64_t> counter_;
};

} // namespace YiNuo
#endif
