/*************************************************************************
  > File Name:    life_game.cpp
  > Author:       Zhu Zhenwei
  > Mail:         losted.leo@gmail.com
  > Created Time: 六  3/11 06:22:35 2017
 ************************************************************************/

#include "life_game.h"
#include "butil/threading/simple_thread.h"

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
class CheckRunner : public butil::DelegateSimpleThread::Delegate {
 public:
  explicit CheckRunner(LifeGame* game, int index, int thread_num) :
    game_(game), index_(index), thread_num_(thread_num) { }

  virtual void Run() OVERRIDE {
    int x = game_->x();
    int start = (x / thread_num_) * index_;
    int end;
    if (index_ != thread_num_ - 1) {
      end = (x / thread_num_) * (index_ + 1);
    } else {
      end = x;
    }

    game_->CheckLifesByIndex(start, end);
  }

 private:
  LifeGame* game_;
  int index_;
  int thread_num_;
};

LifeGame::LifeGame(int x, int y)
  : x_(x), y_(y) {
  started_ = false;
  init_number_ = 0;
  generation_ = 0;
  life_condition_ = 3;
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

void LifeGame::CheckLifesByIndex(int start, int end) {
  for (int j = 0; j < y_; ++j) {
    for (int i = start; i < end; ++i) {
      CheckLife(i, j, life_condition_);
    }
  }
}

void LifeGame::TransformLifes(int start, int end) {
    for (int j = 0; j < y_; ++j) {
      for (int i = start; i < end; ++i) {
        matrix_[j][i] >>= 1;
      }
    }
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
    if (CheckAllExpired()) {
      // life_condition = butil::RandInt(2, 8);
      auto end_point = std::chrono::steady_clock::now();
      double costed_time = std::chrono::duration<double>(end_point - time_point_).count();
      printf("LifeGame had experienced %d generation cost %.3fs\n", generation_, costed_time);
      usleep(1000 * 1000);
      time_point_ = end_point;
      InitRandom(init_number_);
    }

    CheckLifesByIndex(0, x_);
    TransformLifes(0, x_);
    generation_++;
    if (print) {
      Print();
    } else {
      std::cout << "generation:" << generation_ << " lived:" << lived_ << " \n";
    }
    // usleep(10 * 1000);
  }
}

LifeGameRunner::LifeGameRunner(LifeGame* game, int thread_num) :
  game_(game), thread_num_(thread_num) {
  thread_pool_ = new butil::DelegateSimpleThreadPool("work_pool", thread_num_);
  for (int i = 0; i < thread_num_; ++i) {
    auto check_runner = new CheckRunner(game_, i, thread_num_);
    check_delegates_.push_back(check_runner);
  }
}

LifeGameRunner::~LifeGameRunner() {
  delete thread_pool_;
  for (int i = 0; i < thread_num_; ++i) {
    delete check_delegates_[i];
  }
  check_delegates_.clear();
}

void LifeGameRunner::LifeThread(bool print) {
  while (true) {
    if (game_->CheckAllExpired()) {
      // life_condition = butil::RandInt(2, 8);
      auto end_point = std::chrono::steady_clock::now();
      double costed_time = std::chrono::duration<double>(end_point - game_->time_point_).count();
      printf("LifeGame had experienced %d generation cost %.3fs\n", game_->generation_, costed_time);
      usleep(1000 * 1000);
      game_->time_point_ = end_point;
      game_->InitRandom(game_->init_number_);
    }
    for (int i = 0; i < thread_num_; ++i) {
      thread_pool_->AddWork(check_delegates_[i], 1);
    }
    thread_pool_->Start();
    thread_pool_->JoinAll();

    game_->TransformLifes(0, game_->x());
    game_->generation_++;
    if (print) {
      game_->Print();
    } else {
      std::cout << "generation:" << game_->generation_ << " lived:" << game_->lived_ << " \n";
    }
  }
}

} // namespace YiNuo

