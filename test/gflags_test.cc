/*************************************************************************
  > File Name:    test.cc
  > Author:       Zhu Zhenwei
  > Mail:         losted.leo@gmail.com
  > Created Time: Tue Jun 29 23:27:42 2021
 ************************************************************************/

#include <iostream>
#include <gflags/gflags.h>

DEFINE_string(message, "hello world", "Server listen ip of this peer");

void print_message() {
  std::cout << FLAGS_message << std::endl;
}

int main(int argc, char** argv) {
  print_message();
  return 0;
}
