// Author: Zhenwei Zhu losted.leo@gmail.com

#include "common/wait_closure.h"

#include "bthread/countdown_event.h"

namespace noah {
namespace common {
void WaitClosure::Run() {
  if (cond_) {
    cond_->signal();
  }
  delete this;
}

}  // namespace common
}  // namespace noah

