#include "dcpl/threadpool.h"

#include "dcpl/env.h"

namespace dcpl {
namespace {

static const std::size_t num_threads = getenv<std::size_t>("DCPL_NUM_THREADS", 0);

}

threadpool* threadpool::create_system_pool() {
  return new threadpool(num_threads);
}

threadpool* threadpool::get() {
  static threadpool* pool = create_system_pool();

  return pool;
}

}
