// Copyright Microsoft and Project Verona Contributors.
// SPDX-License-Identifier: MIT

#include <cpp/when.h>
#include <debug/harness.h>
#include <thread>

class Body
{
public:
  ~Body()
  {
    Logging::cout() << "Body destroyed" << Logging::endl;
  }
};

using namespace verona::cpp;

void test_body()
{
  BehaviourCore::configure_cown_filtering(0, 120);
  Logging::cout() << "test_body()" << Logging::endl;
  Behaviour* barray[64];

  auto log1 = make_cown<Body>();
  auto log2 = make_cown<Body>();
  auto log3 = make_cown<Body>();

  auto b = when(log1,log2,log3) << [=](auto,auto,auto) { std::cout << "log" << std::endl; };

  size_t b_count = b.get_behaviours(barray);
  BehaviourCore::schedule_many(reinterpret_cast<BehaviourCore**>(barray), 1);

  // spawn other thread to schedule emulate scheduling from multiple threads
  std::thread t([=]() mutable {
      BehaviourCore::configure_cown_filtering(121, 255);
      BehaviourCore::schedule_many(reinterpret_cast<BehaviourCore**>(barray), 1);
      });

  t.join();
}

int main(int argc, char** argv)
{
  SystematicTestHarness harness(argc, argv);

  harness.run(test_body);

  return 0;
}
