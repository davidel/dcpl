#include "dcpl/threadpool.h"

namespace dcpl {

threadpool* threadpool::create_system_pool() {
  return new threadpool();
}

threadpool* threadpool::get() {
  static threadpool* pool = create_system_pool();

  return pool;
}

}
