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
  bool CheckAllExpired();
  int x() {return x_;}
  int y() {return y_;}

  int   generation_;
  std::chrono::steady_clock::time_point time_point_;
  int   lived_;
  int   init_number_;

 private:
  void CheckLifeEx(int x, int y, int life_condition);
  void CheckLife(int x, int y, int life_condition);
  void LifeThread(bool print);
  bool  started_;
  int   life_condition_;
  int   same_count_;
  int   x_;
  int   y_;
  int** matrix_;
  butil::Lock lock_;
};

class LifeGameRunner {
 public:
  LifeGameRunner(LifeGame* game, int thread_num);
  ~LifeGameRunner();
  void LifeThread(bool print);

 private:
  LifeGame* game_;
  int thread_num_;
  butil::DelegateSimpleThreadPool* thread_pool_;
  std::vector<butil::DelegateSimpleThread::Delegate*> check_delegates_;
};

} // namespace YiNuo
#endif
