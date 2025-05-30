#ifndef TEST_HELPERS_TEST_THREAD_H
#define TEST_HELPERS_TEST_THREAD_H

#include <atomic>
#include <memory>

#include "test/helpers/test_utils.h"
#include "torrent/common.h"
#include "torrent/utils/thread.h"

class test_thread : public torrent::utils::Thread {
public:
  enum test_state {
    TEST_NONE,
    TEST_PRE_START,
    TEST_PRE_STOP,
    TEST_STOP
  };

  static const int test_flag_pre_stop       = 0x1;
  static const int test_flag_long_timeout   = 0x2;

  static const int test_flag_do_work   = 0x100;
  static const int test_flag_pre_poke  = 0x200;
  static const int test_flag_post_poke = 0x400;

  static std::unique_ptr<test_thread> create();

  ~test_thread() override;

  int                 test_state() const                 { return m_test_state; }

  bool                is_state(int state) const          { return m_state == state; }
  bool                is_test_state(int state) const     { return m_test_state == state; }
  bool                is_test_flags(int flags) const     { return (m_test_flags & flags) == flags; }
  bool                is_not_test_flags(int flags) const { return !(m_test_flags & flags); }

  // Loop count increments twice each loop.
  int                 loop_count() const                 { return m_loop_count; }

  const char*         name() const override              { return "test_thread"; }

  void                init_thread() override;
  void                cleanup_thread() override;

  void                set_pre_stop()           { m_test_flags |= test_flag_pre_stop; }
  void                set_test_flag(int flags) { m_test_flags |= flags; }

private:
  test_thread();

  void                      call_events() override;
  std::chrono::microseconds next_timeout() override;

  std::atomic_int m_test_state;
  std::atomic_int m_test_flags;
  std::atomic_int m_loop_count{0};
};

#endif
