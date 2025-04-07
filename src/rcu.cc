#include "dcpl/rcu/rcu.h"

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <exception>
#include <functional>
#include <limits>
#include <list>
#include <memory>
#include <mutex>
#include <unordered_set>
#include <thread>
#include <utility>
#include <vector>

#include "dcpl/env.h"
#include "dcpl/logging.h"
#include "dcpl/periodic_task.h"
#include "dcpl/thread.h"
#include "dcpl/utils.h"

namespace dcpl::rcu {
namespace {

using gen_t = std::uintmax_t;

struct callback {
  void* data = nullptr;
  void (*fn)(void*) = nullptr;

  void operator()() const {
    fn(data);
  }
};

struct gen_callbacks {
  gen_t gen = 0;
  std::vector<callback> callbacks;
};

struct rcu_tls {
  std::vector<callback> callbacks;
  gen_t generation = std::numeric_limits<gen_t>::max();
  int scope_count = 0;
};

struct rcu_context {
  std::mutex mtx;
  std::list<gen_callbacks> callbacks;
  std::atomic<gen_t> generation = 1;
  std::unordered_set<rcu_tls*> threads_tls;
  std::unique_ptr<periodic_task> purger;
};

thread_local std::unique_ptr<rcu_tls> tls;

rcu_context* initialize() {
  void thread_enter();
  void thread_exit();
  void purge();

  thread::register_setup(thread_enter, thread_exit);

  rcu_context* ctx = new rcu_context();

  ns_time purge_period = msecs(getenv<std::int64_t>("RCU_PURGE_PERIOD", 1000));

  ctx->purger = std::make_unique<periodic_task>(purge, purge_period);

  return ctx;
}

rcu_context* get_context() {
  static rcu_context* ctx = initialize();

  return ctx;
}

void thread_enter() {
  tls = std::make_unique<rcu_tls>();

  rcu_context* ctx = get_context();
  std::lock_guard guard(ctx->mtx);

  ctx->threads_tls.insert(tls.get());
}

void thread_exit() {
  flush_callbacks();

  rcu_context* ctx = get_context();
  std::lock_guard guard(ctx->mtx);

  ctx->threads_tls.erase(tls.get());
}

rcu_tls* get_tls() {
  if (tls == nullptr) [[unlikely]] {
    thread_enter();
  }

  return tls.get();
}

gen_t get_oldest_generation(rcu_context* ctx, rcu_tls* ctls) {
  // Must be called with ctx->mtx locked.
  gen_t mingen = std::numeric_limits<gen_t>::max();

  for (auto rtls : ctx->threads_tls) {
    if (ctls != rtls) {
      mingen = std::min(mingen, rtls->generation);
    }
  }

  return mingen;
}

void purge() {
  rcu_context* ctx = get_context();
  rcu_tls* ctls = get_tls();
  gen_t curgen = ctx->generation.fetch_add(1);

  DCPL_SLOG() << "Running RCU purge at " << curgen;

  std::lock_guard guard(ctx->mtx);
  gen_t mingen = get_oldest_generation(ctx, ctls);

  DCPL_SLOG() << "Oldest thread RCU generation is " << mingen;

  for (auto it = ctx->callbacks.begin(); it != ctx->callbacks.end();) {
    const gen_callbacks& callbacks = *it;

    if (mingen > callbacks.gen) {
      DCPL_SLOG() << "Running RCU callbacks for generation " << callbacks.gen;

      for (const auto& cb : callbacks.callbacks) {
        try {
          DCPL_SLOG() << "RCU callback for " << cb.data;

          cb();
        } catch (const std::exception& ex) {
          DCPL_ELOG() << "Exception while calling RCU callback for " << ex.what();
        }
      }

      auto eit = it;

      ++it;
      ctx->callbacks.erase(eit);
    } else {
      DCPL_SLOG() << "Skpping RCU callbacks for generation " << callbacks.gen;
      ++it;
    }
  }

  // When the RCU callbacks are called above, they might issue more callbacks,
  // which will be queued in the purger thread. So here we flush them for the next
  // round. We already hold the RCU context lock, taken by the above guard.
  if (!ctls->callbacks.empty()) {
    ctx->callbacks.emplace_back(ctx->generation.load() , std::move(ctls->callbacks));
    ctls->callbacks.clear();
  }
}

}

void flush_callbacks() {
  rcu_context* ctx = get_context();
  rcu_tls* ctls = get_tls();

  if (!ctls->callbacks.empty()) {
    std::lock_guard guard(ctx->mtx);

    ctx->callbacks.emplace_back(ctx->generation.load() , std::move(ctls->callbacks));
    ctls->callbacks.clear();
  }
}

void enter() {
  rcu_context* ctx = get_context();
  rcu_tls* ctls = get_tls();

  if (ctls->scope_count == 0) [[likely]] {
    ctls->generation = ctx->generation.load();
  }
  ctls->scope_count += 1;
}

void exit() {
  rcu_tls* ctls = get_tls();

  ctls->scope_count -= 1;
  if (ctls->scope_count == 0) [[likely]] {
    flush_callbacks();
    ctls->generation = std::numeric_limits<gen_t>::max();
  }
}

void enqueue_callback(void* data, void (*fn)(void*)) {
  rcu_tls* ctls = get_tls();

  ctls->callbacks.emplace_back(data, fn);

  DCPL_SLOG() << "Pointer added to the RCU queue: " << data;
}

void synchronize() {
  const ns_time period = msecs(250);

  std::thread::id this_id = std::this_thread::get_id();
  rcu_context* ctx = get_context();
  rcu_tls* ctls = get_tls();
  gen_t curgen = ctx->generation.load();

  DCPL_VLOG() << "Entering RCU synchronize for thread " << this_id;

  for (;;) {
    {
      std::lock_guard guard(ctx->mtx);
      gen_t mingen = get_oldest_generation(ctx, ctls);

      if (mingen > curgen) {
        break;
      }
    }

    sleep_for(period);
  }

  DCPL_VLOG() << "Exiting RCU synchronize for thread " << this_id;
}

}

