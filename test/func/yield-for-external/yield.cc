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

void external_thread_main(ObjectWithState::State *s, uint64_t work_id)
{
  std::cout << "I'm the external thread\n";
  std::cout << "From the external thread the work id is " << work_id
            << std::endl;
  assert(*s == ObjectWithState::StateB);

  auto pause_time = std::chrono::milliseconds(1000);
  std::this_thread::sleep_for(pause_time);
  std::cout << "I'm the external thread after sleeping\n";

  *s = ObjectWithState::StateC;

  // This needs to run on a scheduler thread
  //when() << [=]() { Behaviour::behaviour_reschedule(work_id); };
  Behaviour::behaviour_reschedule(work_id);
}

void test_counter(SystematicTestHarness* harness)
{
  Logging::cout() << "Yield external input test" << Logging::endl;

  auto obj_cown = make_cown<ObjectWithState>();

  when(obj_cown) << [=](auto obj) {
    uint64_t work_id;
    ObjectWithState::State *s;
    switch (obj->s)
    {
      case ObjectWithState::StateA:
        std::cout << "In state A" << Logging::endl;
        obj->s = ObjectWithState::StateB;

        work_id = Behaviour::behaviour_get_id();
        Scheduler::add_external_event_source();
        s = &(obj->s);
        harness->external_thread([=]() mutable { external_thread_main(s, work_id); });

        BEHAVIOUR_YIELD_WAITING_EXTERNAL();
        break;
      case ObjectWithState::StateB:
        assert(0);
        break;
      case ObjectWithState::StateC:
        std::cout << "In state C" << std::endl;
        Scheduler::remove_external_event_source();
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
