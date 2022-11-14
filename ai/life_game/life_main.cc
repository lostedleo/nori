/*************************************************************************
  > File Name:    life_main.cpp
  > Author:       Zhu Zhenwei
  > Mail:         losted.leo@gmail.com
  > Created Time: 六  3/11 06:46:51 2017
 ************************************************************************/

#include "life_game.h"
#include <iostream>

double coeff = 0.283;

int main(int argc, char** argv) {
  int width = 204;
  int height = 48;
  int init_count = 1000;
  init_count = int(width * height * coeff);
  bool print = true;
  if (argc > 1) {
    init_count = atoi(argv[1]);
  }
  if (argc > 3) {
    width = atoi(argv[2]);
    height = atoi(argv[3]);
  }
  if (argc > 4) {
    print = bool(atoi(argv[4]));
  }
  // (314, 68);
  YiNuo::LifeGame life_game(width, height);
  life_game.Init(init_count);
  life_game.Start(print);

  return 0;
}

