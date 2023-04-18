/*************************************************************************
  > File Name:    life_game.cpp
  > Author:       Zhu Zhenwei
  > Mail:         losted.leo@gmail.com
  > Created Time: 六  3/11 06:22:35 2017
 ************************************************************************/

#include "life_game.h"

#include <time.h>
#include <iostream>
#include <unistd.h>
#include <map>
#include <sstream>
#include <string>

uint64_t RandGenerator(uint64_t min, uint64_t max) {
static bool first = true;
   if (first) {
      srand(time(NULL)); //seeding for the first time only!
      first = false;
   }
   return min + rand() % (( max + 1 ) - min);

}

namespace YiNuo {
LifeGame::LifeGame(int x, int y)
  : x_(x), y_(y) {
  started_ = false;
  init_number_ = 0;
  generation_ = 0;
  lived_ = 0;
  same_count_ = 0;
  matrix_ = new int*[y_];
  for (int i = 0; i < y_; ++i) {
    matrix_[i] = new int[x_];
    for (int j = 0; j < x_; ++j) {
      matrix_[i][j] = 0;
    }
  }
  time_point_ = std::chrono::steady_clock::now();
}

LifeGame::~LifeGame() {
  Stop();
  for (int i = 0; i < y_; ++i) {
    delete []matrix_[i];
  }
  delete []matrix_;
}

void LifeGame::Init(int number) {
  init_number_ = number;
  InitRandom(init_number_);
}

void LifeGame::InitRandom(int number) {
  int x, y;
  for (int i = 0; i < number; ++i) {
    x = RandGenerator(0, x_-1);
    y = RandGenerator(0, y_-1);
    matrix_[y][x] = 1;
  }
  generation_ = 0;
  lived_ = 0;
  same_count_ = 0;
}

void LifeGame::Start(bool print/*=fase*/) {
  if (started_) {
    return;
  }
  started_ = true;
  LifeThread(print);
}

void LifeGame::Stop() {
  if (started_) {
    started_ = false;
  }
}

void LifeGame::Print() {
  std::stringstream ss;
  for (int i = 0; i < x_ + 2; ++i) {
    ss << "-";
  }
  ss << "\n";
  for (int i = 0; i < y_; ++i) {
    for (int j = 0; j < x_; ++j) {
      if (j == 0) {
        ss << "|";
      }
      if (matrix_[i][j]) {
        ss << "x";
      } else {
        ss << " ";
      }
    }
    ss << "|\n";
  }

  std::stringstream ss_temp;
  ss_temp << " generation:" << generation_ << " lived:" << lived_ << " ";
  int length = ss_temp.str().length();
  int left = (x_ + 2 - length) / 2;
  for (int i = 0; i < left; ++i) {
    ss << "-";
  }
  ss << ss_temp.str();
  for (int i = left + length; i < x_ + 2; ++i) {
    ss << "-";
  }
  ss << "\n";
  printf("%s", ss.str().c_str());
  fflush(stdout);
}

void LifeGame::CheckLifeEx(int x, int y, int life_condition) {
  int count = 0;
  for (int i = std::max(x - 1, 0); i < std::min(x + 2, x_); ++i)
    for (int j = std::max(y - 1, 0); j < std::min(y + 2, y_); ++j) {
      count += matrix_[j][i] & 1;
    }

  if (count == life_condition || count - matrix_[y][x] == life_condition)
    matrix_[y][x] |= 2;
}

void LifeGame::CheckLife(int x, int y, int life_condition) {
  int count = 0;
  for (int i = x - 1; i < x + 2; ++i)
    for (int j = y - 1; j < y + 2; ++j) {
      count += matrix_[(j + y_) % y_][(i + x_) % x_] & 1;
    }

  if (count == life_condition || count - matrix_[y][x] == life_condition)
    matrix_[y][x] |= 2;
}

bool LifeGame::CheckAllExpired() {
  int lived = 0;
  for (int i = 0; i < y_; ++i)
    for (int j = 0; j < x_; j++) {
      if (matrix_[i][j])
        lived++;
    }
  if (lived_ == lived) {
    same_count_++;
  } else {
    lived_ = lived;
    same_count_ = 0;
  }
  if (same_count_ >= 10) {
    return true;
  }
  return false;
}

void LifeGame::LifeThread(bool print) {
  while (started_) {
    int life_condition = 3;
    if (CheckAllExpired()) {
      // life_condition = butil::RandInt(2, 8);
      auto end_point = std::chrono::steady_clock::now();
      double costed_time = std::chrono::duration<double>(end_point - time_point_).count();
      printf("LifeGame had experienced %d generation cost %.3fs\n", generation_, costed_time);
      usleep(1000 * 1000);
      time_point_ = end_point;
      InitRandom(init_number_);
    }
    for (int j = 0; j < y_; ++j) {
      for (int i = 0; i < x_; ++i) {
        CheckLife(i, j, life_condition);
      }
    }

    for (int j = 0; j < y_; ++j) {
      for (int i = 0; i < x_; ++i) {
        matrix_[j][i] >>= 1;
      }
    }
    generation_++;
    if (print) {
      Print();
    } else {
      std::cout << "generation:" << generation_ << " lived:" << lived_ << " \n";
    }
    usleep(10 * 1000);
  }
}
}

