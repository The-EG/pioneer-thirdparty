#include "testutilities.h"
#include <sigc++/sigc++.h>
#include <sstream>
#include <string>
#include <cstdlib>

namespace
{
std::ostringstream result_stream;

class Param : public sigc::trackable
{
public:
  Param(const std::string& name)
  : name_(name)
  {}

  //non-copyable,
  //so it can only be used with sigc::bind() via std::ref()
  Param(const Param&) = delete;
  Param& operator=(const Param&) = delete;

  //non movable:
  Param(Param&&) = delete;
  Param& operator=(Param&&) = delete;

  std::string name_;
};

void handler(Param& param)
{
  result_stream << "  handler(param): param.name_=" << param.name_;
}

} // end anonymous namespace

int main(int argc, char* argv[])
{
  auto util = TestUtilities::get_instance();

  if (!util->check_command_args(argc, argv))
    return util->get_result_and_delete_instance() ? EXIT_SUCCESS : EXIT_FAILURE;

  auto slot_full = sigc::ptr_fun(&handler);
  sigc::slot<void> slot_bound;

  slot_bound();
  util->check_result(result_stream, "");

  {
    //Because Param derives from sigc::trackable(), std::ref() should disconnect
    // the signal handler when param is destroyed.
    Param param("murrayc");
    // A convoluted way to do
    // slot_bound = sigc::bind(slot_full, std::ref(param));
    slot_bound = sigc::bind< -1, std::reference_wrapper<Param> >(slot_full, std::ref(param));

    result_stream << "Calling slot when param exists:";
    slot_bound();
    util->check_result(result_stream,
      "Calling slot when param exists:  handler(param): param.name_=murrayc");
  } // auto-disconnect

  result_stream << "Calling slot when param does not exist:";
  slot_bound();
  util->check_result(result_stream, "Calling slot when param does not exist:");
  // This causes a crash when using g++ 3.3.4 or 3.3.5 (but not 3.4.x) when not specifying
  // the exact template specialization in visit_each_type() - see the comments there.
  // It looks like the auto-disconnect does not work, so the last slot_bound() call tries
  // to access the param data again.

  return util->get_result_and_delete_instance() ? EXIT_SUCCESS : EXIT_FAILURE;
}
