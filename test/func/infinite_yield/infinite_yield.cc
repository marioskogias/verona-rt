// Copyright Microsoft and Project Verona Contributors.
// SPDX-License-Identifier: MIT

#include <atomic>
#include <cpp/when.h>
#include <debug/harness.h>

using namespace verona::cpp;

// Global storage for the waker
std::unique_ptr<verona::rt::Waker> global_waker;
std::atomic<bool> waker_ready{false};

void test_infinite_yield(SystematicTestHarness* harness)
{
  Logging::cout() << "Infinite yield test" << Logging::endl;

  // Reset global state
  waker_ready = false;
  global_waker.reset();

  auto cown = make_cown<int>(0);

  // Schedule a behaviour that suspends itself
  when(cown) << [](auto c) {
    Logging::cout() << "Suspending behaviour" << Logging::endl;
    if (*c == 0)
    {
      // Prevent runtime from quiescing while we wait for the external event
      verona::rt::Scheduler::add_external_event_source();

      *c = 1;
      global_waker = std::make_unique<verona::rt::Waker>(
        verona::rt::Behaviour::suspend_with_waker());
      waker_ready = true;
    }
  };

  // Spawn a thread to wake the behaviour
  harness->external_thread([]() {
    Logging::cout() << "Waker thread waiting..." << Logging::endl;
    while (!waker_ready)
    {
      verona::rt::Systematic::yield();
    }
    Logging::cout() << "Waker thread waking..." << Logging::endl;

    // Schedule the wake on a scheduler thread to be safe
    schedule_lambda([]() {
      if (global_waker)
      {
        global_waker->wake();
      }
      // Allow runtime to finish
      verona::rt::Scheduler::remove_external_event_source();
    });
  });

  // Schedule a cleanup/check behaviour
  when(cown) << [](auto c) {
    Logging::cout() << "Finished, value is " << *c << Logging::endl;
    if (*c != 1)
    {
      Logging::cout() << "Error: Value not updated!" << Logging::endl;
      abort();
    }
    // Clean up waker
    global_waker.reset();
  };
}

int main(int argc, char** argv)
{
  SystematicTestHarness harness(argc, argv);

  harness.run(test_infinite_yield, &harness);

  return 0;
}
