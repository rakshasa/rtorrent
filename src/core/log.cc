#include "config.h"

#include <algorithm>

#include "log.h"
#include "utils/functional.h"

namespace core {

void
Log::push_front(const std::string& msg) {
  Base::push_front(Type(utils::Timer::cache(), msg));

  m_signalUpdate.emit();
}

Log::iterator
Log::find_older(utils::Timer t) {
  return std::find_if(begin(), end(), func::on(func::mem_ptr_ref(&Type::first),
					       std::bind1st(std::less_equal<utils::Timer>(), t)));
}

}
