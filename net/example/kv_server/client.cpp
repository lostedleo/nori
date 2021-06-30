// Copyright (c) 2014 Baidu, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// A client sending requests to server by multiple threads.

#include <atomic>

#include <gflags/gflags.h>
#include <bthread/bthread.h>
#include <butil/logging.h>
#include <brpc/server.h>
#include <brpc/channel.h>
#include <bvar/bvar.h>
#include <butil/rand_util.h>
#include <butil/base64.h>
#include <butil/synchronization/blocking_queue.h>

#include "cache.pb.h"
#include "file_read.h"

DEFINE_int32(thread_num, 50, "Number of threads to send requests");
DEFINE_bool(use_bthread, false, "Use bthread to send requests");
DEFINE_int32(attachment_size, 0, "Carry so many byte attachment along with requests");
DEFINE_string(data_file, "", "File to read data");
DEFINE_int64(offset, 0, "Offset of lines to read file");
DEFINE_int32(size, 1000*1000, "Size of key_values");
DEFINE_int32(key_size, 16, "Bytes of key");
DEFINE_int32(value_size, 16, "Bytes of value");
DEFINE_int32(batch_size, 100, "Size of key value pairs in each request");
DEFINE_string(protocol, "baidu_std", "Protocol type. Defined in src/brpc/options.proto");
DEFINE_string(connection_type, "", "Connection type. Available values: single, pooled, short");
DEFINE_string(server, "0.0.0.0:8002", "IP Address of server");
DEFINE_string(load_balancer, "", "The algorithm for load balancing");
DEFINE_int32(timeout_ms, 100, "RPC timeout in milliseconds");
DEFINE_int32(max_retry, 3, "Max retries(not including the first RPC)");
DEFINE_bool(dont_fail, false, "Print fatal when some call failed");
DEFINE_int32(dummy_port, -1, "Launch dummy server at this port");
DEFINE_string(http_content_type, "application/json", "Content type of http request");
DEFINE_bool(set, true, "Is set or get");

std::vector<std::string>* g_keys = NULL;
std::vector<std::string>* g_values = NULL;
std::atomic<int> g_index;
std::string g_attachment;

bvar::LatencyRecorder g_latency_recorder("client");
bvar::Adder<int> g_error_count("client_error_count");

static void* get(void* arg) {
  // Normally, you should not call a Channel directly, but instead construct
  // a stub Service wrapping it. stub can be shared by all threads as well.
  push::CacheService_Stub stub(static_cast<google::protobuf::RpcChannel*>(arg));

  int log_id = 0;
  int index = g_index++;
  int start = (FLAGS_size / FLAGS_thread_num) * index;
  int end = (FLAGS_size / FLAGS_thread_num) * (index + 1);
  if (end >= FLAGS_size) {
  end = FLAGS_size;
  }
  while (!brpc::IsAskedToQuit()) {
  // We will receive response synchronously, safe to put variables
  // on stack.
  push::GetRequest request;
  push::GetResponse response;
  brpc::Controller cntl;

  for (int i = 0; i < FLAGS_batch_size; ++i) {
    request.add_keys((*g_keys)[start++ % FLAGS_size]);
  }
  cntl.set_log_id(log_id++);  // set by user
  if (FLAGS_protocol != "http" && FLAGS_protocol != "h2c") {
    // Set attachment which is wired to network directly instead of
    // being serialized into protobuf messages.
    cntl.request_attachment().append(g_attachment);
  } else {
    cntl.http_request().set_content_type(FLAGS_http_content_type);
  }

  // Because `done'(last parameter) is NULL, this function waits until
  // the response comes back or error occurs(including timedout).
  stub.Get(&cntl, &request, &response, NULL);
  if (!cntl.Failed()) {
    g_latency_recorder << cntl.latency_us();
  } else {
    g_error_count << 1;
    CHECK(brpc::IsAskedToQuit() || !FLAGS_dont_fail)
    << "error=" << cntl.ErrorText() << " latency=" << cntl.latency_us();
    // We can't connect to the server, sleep a while. Notice that this
    // is a specific sleeping to prevent this thread from spinning too
    // fast. You should continue the business logic in a production
    // server rather than sleeping.
    bthread_usleep(50000);
  }
  }
  return NULL;
}

static void* set(void* arg) {
  // Normally, you should not call a Channel directly, but instead construct
  // a stub Service wrapping it. stub can be shared by all threads as well.
  push::CacheService_Stub stub(static_cast<google::protobuf::RpcChannel*>(arg));

  int log_id = 0;
  int index = g_index++;
  int start = (FLAGS_size / FLAGS_thread_num) * index;
  int end = (FLAGS_size / FLAGS_thread_num) * (index + 1);
  if (end >= FLAGS_size) {
  end = FLAGS_size;
  }
  while (!brpc::IsAskedToQuit()) {
  // We will receive response synchronously, safe to put variables
  // on stack.
  push::SetRequest request;
  push::SetResponse response;
  brpc::Controller cntl;

  google::protobuf::Map<std::string, std::string>* key_values = request.mutable_key_values();
  for (int i = 0; i < FLAGS_batch_size; ++i) {
    (*key_values)[(*g_keys)[start % FLAGS_size]] =
    (*g_values)[start % FLAGS_size];
    start++;
  }
  cntl.set_log_id(log_id++);  // set by user
  if (FLAGS_protocol != "http" && FLAGS_protocol != "h2c") {
    // Set attachment which is wired to network directly instead of
    // being serialized into protobuf messages.
    cntl.request_attachment().append(g_attachment); } else {
    cntl.http_request().set_content_type(FLAGS_http_content_type);
  }

  // Because `done'(last parameter) is NULL, this function waits until
  // the response comes back or error occurs(including timedout).
  stub.Set(&cntl, &request, &response, NULL);
  if (!cntl.Failed()) {
    g_latency_recorder << cntl.latency_us();
  } else {
    g_error_count << 1;
    CHECK(brpc::IsAskedToQuit() || !FLAGS_dont_fail)
    << "error=" << cntl.ErrorText() << " latency=" << cntl.latency_us();
    // We can't connect to the server, sleep a while. Notice that this
    // is a specific sleeping to prevent this thread from spinning too
    // fast. You should continue the business logic in a production
    // server rather than sleeping.
    bthread_usleep(50000);
  }
  }
  return NULL;
}

bool get_key_values() {
  g_keys = new std::vector<std::string>();
  g_values = new std::vector<std::string>();
  if (FLAGS_data_file.empty()) {
  std::string key, base64_key, value, base64_value;
  for (int i = 0; i < FLAGS_size; ++i) {
    key = butil::RandBytesAsString(FLAGS_key_size);
    value = butil::RandBytesAsString(FLAGS_value_size);
    butil::Base64Encode(key, &base64_key);
    butil::Base64Encode(value, &base64_value);
    g_keys->push_back(base64_key);
    g_values->push_back(base64_value);
  }
  return true;
  } else {
  push::FileRead file_read(FLAGS_data_file.c_str());
  int size = file_read.Read(FLAGS_offset, FLAGS_size, g_keys, g_values);
  if (size != FLAGS_size) {
    LOG(ERROR) << "get key_values error size:" << size << " expect size:"
    << FLAGS_size;
    delete g_keys;
    delete g_values;
    return false;
  }
  }
  return true;
}

int main(int argc, char* argv[]) {
  // Parse gflags. We recommend you to use gflags as well.
  google::ParseCommandLineFlags(&argc, &argv, true);

  // A Channel represents a communication line to a Server. Notice that
  // Channel is thread-safe and can be shared by all threads in your program.
  brpc::Channel channel;

  // Initialize the channel, NULL means using default options.
  brpc::ChannelOptions options;
  options.protocol = FLAGS_protocol;
  options.connection_type = FLAGS_connection_type;
  options.connect_timeout_ms = std::min(FLAGS_timeout_ms / 2, 100);
  options.timeout_ms = FLAGS_timeout_ms;
  options.max_retry = FLAGS_max_retry;
  if (channel.Init(FLAGS_server.c_str(), FLAGS_load_balancer.c_str(), &options) != 0) {
  LOG(ERROR) << "Fail to initialize channel";
  return -1;
  }

  if (FLAGS_attachment_size > 0) {
  g_attachment.resize(FLAGS_attachment_size, 'a');
  }

  if (FLAGS_dummy_port >= 0) {
  brpc::StartDummyServerAt(FLAGS_dummy_port);
  }

  if (!get_key_values()) {
  return -1;
  }

  std::vector<bthread_t> tids;
  tids.resize(FLAGS_thread_num);
  void *(*func_ptr)(void*);
  if (FLAGS_set) {
  func_ptr = set;
  } else {
  func_ptr = get;
  }

  if (!FLAGS_use_bthread) {
  for (int i = 0; i < FLAGS_thread_num; ++i) {
    if (pthread_create(&tids[i], NULL, func_ptr, &channel) != 0) {
    LOG(ERROR) << "Fail to create pthread";
    return -1;
    }
  }
  } else {
  for (int i = 0; i < FLAGS_thread_num; ++i) {
    if (bthread_start_background(&tids[i], NULL, func_ptr, &channel) != 0) {
    LOG(ERROR) << "Fail to create bthread";
    return -1;
    }
  }
  }

  while (!brpc::IsAskedToQuit()) {
  sleep(1);
  LOG(INFO) << "Sending Request at qps=" << g_latency_recorder.qps(1) * FLAGS_batch_size
    << " latency=" << g_latency_recorder.latency(1);
  }

  LOG(INFO) << "Client is going to quit";
  for (int i = 0; i < FLAGS_thread_num; ++i) {
  if (!FLAGS_use_bthread) {
    pthread_join(tids[i], NULL);
  } else {
    bthread_join(tids[i], NULL);
  }
  }
  delete g_keys;
  delete g_values;

  return 0;
}
