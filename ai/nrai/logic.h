/*************************************************************************
  > File Name:    logic.h
  > Author:       Zhu Zhenwei
  > Mail:         losted.leo@gmail.com
  > Created Time: Mon 09 Oct 2017 08:29:17 PM CST
 ************************************************************************/

#ifndef LOGIC_H_
#define LOGIC_H_

#include "handle.h"

namespace nrai {

class Logic : public Handle {
 public:
  Logic();
  ~Logic();

  virtual bool Learn(void* information);
  virtual bool HandleProblem(void* problem);
};

}
#endif
