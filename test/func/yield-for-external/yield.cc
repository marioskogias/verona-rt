// Copyright Microsoft and Project Verona Contributors.
// SPDX-License-Identifier: MIT

#include <cpp/when.h>
#include <debug/harness.h>

#define BEHAVIOUR_YIELD_WAITING_EXTERNAL(X) \
  { \
    verona::rt::Behaviour::behaviour_yield_waiting_external() = true; \
    return X; \
  }

class ObjectWithState
{
public:
  enum State
  {
    StateA = 0,
    StateB,
    StateC
  };
  State s;

  ObjectWithState() : s(StateA) {}
};


using namespace verona::cpp;

void external_thread_main(uint64_t work_id)
{
    std::cout << "I'm the external thread\n";
    std::cout << "From the external thread the work id is " << work_id << std::endl;
    //assert(obj.s == ObjectWithState::StateB)

    auto pause_time = std::chrono::milliseconds(1000);
    std::this_thread::sleep_for(pause_time);
    std::cout << "I'm the external thread after sleeping\n";

    // This needs to run on a scheduler thread
    when() << [=]() {
        Behaviour::behaviour_reschedule(work_id);
    };
}

void test_counter(SystematicTestHarness *harness)
{
  Logging::cout() << "Yield external input test" << Logging::endl;

  auto obj_cown = make_cown<ObjectWithState>();

  when(obj_cown) << [=](auto obj) {
    uint64_t work_id;
    switch (obj->s)
    {
      case ObjectWithState::StateA:
        std::cout << "In state A" << Logging::endl;
        obj->s = ObjectWithState::StateB;

        work_id = Behaviour::behaviour_get_id();
        Scheduler::add_external_event_source();
        harness->external_thread([=]() { external_thread_main(work_id); });

        BEHAVIOUR_YIELD_WAITING_EXTERNAL();
        break;
      case ObjectWithState::StateB:
        assert(0);
        break;
      case ObjectWithState::StateC:
        Logging::cout() << "In state C" << Logging::endl;
        break;
    }
  };
}

int main(int argc, char** argv)
{
  SystematicTestHarness harness(argc, argv);

  Logging::cout() << "Yield test" << Logging::endl;

  harness.run(test_counter, &harness);

  return 0;
}
