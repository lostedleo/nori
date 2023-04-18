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
  int sleep_ms = 0;
  if (argc > 1) {
    if ((strcmp(argv[1], "-h") == 0) or (strcmp(argv[1], "--help") == 0)) {
      printf("Usage: %s initialization_counter width height print[or not] thread_num sleep_time\n", argv[0]);
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
    sleep_ms = atoi(argv[6]);
  }
  // (314, 68);
  YiNuo::LifeGame life_game(width, height);
  life_game.Init(init_count);
  // life_game.Start(print);

  YiNuo::LifeGameRunner runner(&life_game, thread_num, sleep_ms);
  while (!stop) {
    runner.Run(print);
  }

  return 0;
}

