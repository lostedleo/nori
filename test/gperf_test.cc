/*************************************************************************
  > File Name:    gperf_test.cc
  > Author:       Zhu Zhenwei
  > Mail:         losted.leo@gmail.com
  > Created Time: Wed Jun 30 12:44:04 2021
 ************************************************************************/
#include "gperftools/profiler.h"

void test1() {
    int i = 0;
    while (i < 1000) i++;
}

void test2() {
    int i = 0;
    while (i < 2000) i++;
}

void test3() {
    for (int i = 0; i < 100000; ++i){
        test1();
        test2();
    }
}

int main() {
    ProfilerStart("test.prof"); // test.prof is the name of profile file
    test3();
    ProfilerStop();
    return 0;
}

