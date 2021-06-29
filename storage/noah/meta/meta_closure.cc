// Author: Zhenwei Zhu losted.leo@gmail.com

#include "meta/meta_closure.h"

#include <memory>

#include <brpc/closure_guard.h>

#include "meta/meta_server.h"

namespace noah {
namespace meta {
void MetaClosure::Run() {
  std::unique_ptr<MetaClosure> self_guard(this);
  brpc::ClosureGuard done_guard(done_);
  if (status().ok()) {
    return;
  }
  meta_server_->Redirect(response_);
}
}  // namespace meta
}  // namespace noah
