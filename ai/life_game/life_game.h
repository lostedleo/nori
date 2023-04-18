/*************************************************************************
  > File Name:    life_game.h
  > Author:       Zhu Zhenwei
  > Mail:         losted.leo@gmail.com
  > Created Time: 六  3/11 06:22:55 2017
 ************************************************************************/

#ifndef LIFE_GAME_H_
#define LIFE_GAME_H_

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

 private:
  void CheckLifeEx(int x, int y, int life_condition);
  void CheckLife(int x, int y, int life_condition);
  bool CheckAllExpired();
  void LifeThread(bool print);
  bool  started_;
  bool  printed_;
  int   init_number_;
  int   generation_;
  int   lived_;
  int   same_count_;
  int   x_;
  int   y_;
  int** matrix_;
  std::chrono::steady_clock::time_point time_point_;
};

} // namespace YiNuo
#endif
