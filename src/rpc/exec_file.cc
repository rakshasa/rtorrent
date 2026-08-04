#include "config.h"

#include "rpc/exec_file.h"

// #include <cassert>
// #include <cerrno>
// #include <cstring>
// #include <fcntl.h>
// #include <spawn.h>
// #include <string>
#include <unistd.h>
// #include <sys/types.h>
#include <sys/uio.h>
// #include <torrent/net/fd.h>
#include <torrent/system/thread.h>
#include <torrent/system/spawn_process.h>
#include <torrent/system/types.h>

#include "rpc/parse.h"

namespace rpc {

// TODO: Access fd through torrent logging?

int
ExecFile::execute(const char* file, char* const* argv, int flags) {
  torrent::system::SpawnProcess spawn_process;

  spawn_process.set_log_fd(m_log_fd);
  spawn_process.set_background(flags & flag_background);
  spawn_process.set_capture_output(flags & flag_capture);

  if (m_log_fd != -1) {
    std::vector<struct iovec> iovecs;
    iovecs.reserve(32);
    iovecs.push_back({const_cast<char*>("\n---\n"), 5});

    for (auto* itr = argv; *itr != nullptr; itr++) {
      if (itr != argv)
        iovecs.push_back({const_cast<char*>(" "), 1});

      iovecs.push_back({*itr, std::strlen(*itr)});
    }

    iovecs.push_back({const_cast<char*>("\n---\n"), 5});

    [[maybe_unused]] int result = ::writev(m_log_fd, iovecs.data(), iovecs.size());
  }

  int spawn_status = spawn_process.execute(file, argv);

  if (spawn_status != 0) {
    if (m_log_fd != -1) {
      auto prefix    = "\n--- posix_spawn failed: ";
      auto errno_str = torrent::system::errno_enum_str(spawn_status) + " ---\n";

      struct iovec iovecs[2] = {
        {const_cast<char*>(prefix), std::strlen(prefix)},
        {const_cast<char*>(errno_str.c_str()), errno_str.size()},
      };

      [[maybe_unused]] int result = ::writev(m_log_fd, iovecs, 2);
    }

    throw torrent::input_error("ExecFile::execute() posix_spawn failed: " + torrent::system::errno_enum_str(spawn_status));
  }

  if (flags & flag_background) {
    m_waitpid_queue.close_pid(spawn_process.child_pid());
    return 0;
  }

  if (flags & flag_capture)
    m_capture = spawn_process.capture_child_output();

  return spawn_process.wait_for_child();
}

torrent::Object
ExecFile::execute_object(const torrent::Object& rawArgs, int flags) {
  char*  argsBuffer[max_args];
  char** argsCurrent = argsBuffer;

  // Size of value strings are less than 24.
  char   valueBuffer[buffer_size+1];
  char*  valueCurrent = valueBuffer;

  if (rawArgs.is_list()) {
    const torrent::Object::list_type& args = rawArgs.as_list();

    if (args.empty())
      throw torrent::input_error("Too few arguments.");

    for (torrent::Object::list_const_iterator itr = args.begin(), last = args.end(); itr != last; itr++, argsCurrent++) {
      if (argsCurrent == argsBuffer + max_args - 1)
        throw torrent::input_error("Too many arguments.");

      if (itr->is_string() && (!(flags & flag_expand_tilde) || *itr->as_string().c_str() != '~')) {
        *argsCurrent = const_cast<char*>(itr->as_string().c_str());

      } else {
        *argsCurrent = valueCurrent;
        valueCurrent = print_object(valueCurrent, valueBuffer + buffer_size, &*itr, flags) + 1;

        if (valueCurrent >= valueBuffer + buffer_size)
          throw torrent::input_error("Overflowed execute arg buffer.");
      }
    }

  } else {
    const torrent::Object::string_type& args = rawArgs.as_string();

    if ((flags & flag_expand_tilde) && args.c_str()[0] == '~') {
      *argsCurrent = valueCurrent;
      valueCurrent = print_object(valueCurrent, valueBuffer + buffer_size, &rawArgs, flags) + 1;
    } else {
      *argsCurrent = const_cast<char*>(args.c_str());
    }

    argsCurrent++;
  }

  *argsCurrent = NULL;

  int status = execute(argsBuffer[0], argsBuffer, flags);

  if ((flags & flag_throw) && status != 0)
    throw torrent::input_error("Bad return code.");

  if (flags & flag_capture)
    return m_capture;

  return torrent::Object((int64_t)status);
}

} // namespace rpc
