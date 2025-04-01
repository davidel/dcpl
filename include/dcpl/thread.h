#pragma once

#include <functional>
#include <memory>
#include <thread>
#include <utility>

namespace dcpl {
namespace thread {

using setup_fn = std::function<void(void)>;
using cleanup_fn = std::function<void(void)>;

int register_setup(setup_fn setup, cleanup_fn cleanup);

void unregister_setup(int sid);

std::function<void(void)> wrap_fn(std::function<void(void)> thread_fn);

std::unique_ptr<std::thread> create(std::function<void(void)> fn);

}

}

