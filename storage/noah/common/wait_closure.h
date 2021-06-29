// Author: Zhenwei Zhu losted.leo@gmail.com

#ifndef NOAH_COMMON_WAIT_CLOSURE_H_
#define NOAH_COMMON_WAIT_CLOSURE_H_

#include <google/protobuf/stubs/callback.h>

#include "butil/macros.h"

namespace bthread {
class CountdownEvent;
}

namespace noah {
namespace common {
class WaitClosure;
}
}

#define NEW_WAITCLOSURE(cond) \
        (new noah::common::WaitClosure(cond, __FILE__ ":" BAIDU_SYMBOLSTR(__LINE__)))

namespace noah {
namespace common {
class WaitClosure : public google::protobuf::Closure {
 public:
  WaitClosure(bthread::CountdownEvent* cond, const char* pos)
    : cond_(cond), pos_(pos) {
  }

  void Run() override;

 private:
  bthread::CountdownEvent*  cond_;
  const char*               pos_;
};

}  // namespace common
}  // namespace noah
#endif
