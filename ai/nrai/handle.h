/*************************************************************************
  > File Name:    handle.h
  > Author:       Zhu Zhenwei
  > Mail:         losted.leo@gmail.com
  > Created Time: Mon 09 Oct 2017 09:11:11 PM CST
 ************************************************************************/

#ifndef HANDLE_H_
#define HANDLE_H_
namespace nrai {
class Handle {
  virtual bool Study(void* information) = 0;
  virtual bool HandleProblem(void* problem) = 0;
};
} // namespace nrai
#endif
