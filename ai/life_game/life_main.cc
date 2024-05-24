/*************************************************************************
  > File Name:    life_main.cpp
  > Author:       Zhu Zhenwei
  > Mail:         losted.leo@gmail.com
  > Created Time: 六  3/11 06:46:51 2017
 ************************************************************************/

#include "life_game.h"

#include <iostream>
#include <string.h>
#include <time.h>
#include <sys/signal.h>

double coeff = 0.283;
static bool stop = false;

void sig_handler(int signum){
  stop = true;
  printf("Exiting LifeGame...! Please waiting...! signal: %d.\n", signum);
}

int main(int argc, char** argv) {
  signal(SIGINT, sig_handler); // Register signal handler

  int width = 204;
  int height = 48;
  int init_count = 1000;
  init_count = int(width * height * coeff);
  bool print = true;
  int thread_num = 2;
  int work_num = 4;
  int sleep_ms = 0;
  if (argc > 1) {
    if ((strcmp(argv[1], "-h") == 0) or (strcmp(argv[1], "--help") == 0)) {
      printf("Usage: %s initialization_counter width height print[or not] thread_num work_num sleep_time\n", argv[0]);
      exit(0);
    }
    init_count = atoi(argv[1]);
  }

  if (argc > 3) {
    width = atoi(argv[2]);
    height = atoi(argv[3]);
  }
  if (argc > 4) {
    print = bool(atoi(argv[4]));
  }
  if (argc > 5) {
    thread_num = atoi(argv[5]);
  }
  if (argc > 6) {
    work_num = atoi(argv[6]);
  }
  if (argc > 7) {
    sleep_ms = atoi(argv[7]);
  }
  // (314, 68);
  auto start_point = std::chrono::steady_clock::now();
  YiNuo::LifeGame life_game(width, height);
  auto end_point = std::chrono::steady_clock::now();
  double costed_time = std::chrono::duration<double>(end_point - start_point).count();
  printf("LifeGame consturct cost %.3fs\n", costed_time);
  life_game.Init(init_count, false);
  // life_game.Start(print);

  YiNuo::LifeGameRunner runner(&life_game, thread_num, work_num, sleep_ms);
  start_point = std::chrono::steady_clock::now();
  runner.InitGame();
  end_point = std::chrono::steady_clock::now();
  costed_time = std::chrono::duration<double>(end_point - start_point).count();
  printf("LifeGame init cost %.3fs\n", costed_time);
  while (!stop) {
    runner.Run(print);
  }

  return 0;
}

