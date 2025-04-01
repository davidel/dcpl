#include "dcpl/thread.h"

#include <exception>
#include <map>
#include <mutex>
#include <vector>

#include "dcpl/cleanup.h"
#include "dcpl/logging.h"

namespace dcpl {
namespace thread {
namespace {

struct prepare {
  setup_fn setup;
  cleanup_fn cleanup;
};

using prepare_ptr = std::shared_ptr<prepare>;

struct context {
  std::mutex mtx;
  int next_sid = 1;
  std::map<int, prepare_ptr> thread_fns;
};

context* get_context() {
  static context* ctx = new context();

  return ctx;
}

void bootstrap(const std::function<void(void)>& thread_fn) {
  context* ctx = get_context();
  std::vector<prepare_ptr> cleanups;
  cleanup clean([&]() {
    for (auto rit = cleanups.rbegin(); rit != cleanups.rend(); ++rit) {
      (*rit)->cleanup();
    }
  });

  {
    std::lock_guard guard(ctx->mtx);

    cleanups.reserve(ctx->thread_fns.size());
    for (const auto& [sid, pptr] : ctx->thread_fns) {
      try {
        pptr->setup();
      } catch (const std::exception& ex) {
        DCPL_ELOG() << "Error executing thread setup: " << ex.what();
        throw;
      }

      cleanups.push_back(pptr);
    }
  }

  try {
    thread_fn();
  } catch (const std::exception& ex) {
    DCPL_ELOG() << "Error executing thread function: " << ex.what();
    throw;
  }
}

}

int register_setup(setup_fn setup, cleanup_fn cleanup) {
  context* ctx = get_context();
  std::lock_guard guard(ctx->mtx);
  int sid = ctx->next_sid;

  ctx->thread_fns.emplace(sid, std::make_shared<prepare>(std::move(setup),
                                                         std::move(cleanup)));
  ctx->next_sid += 1;

  return sid;
}

void unregister_setup(int sid) {
  context* ctx = get_context();
  std::lock_guard guard(ctx->mtx);

  ctx->thread_fns.erase(sid);
}

std::function<void(void)> wrap_fn(std::function<void(void)> thread_fn) {
  return [thread_fn = std::move(thread_fn)]() {
    bootstrap(thread_fn);
  };
}

std::unique_ptr<std::thread> create(std::function<void(void)> fn) {
  return std::make_unique<std::thread>(wrap_fn(std::move(fn)));
}

}

}

