#include "config.h"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <spawn.h>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <torrent/system/thread.h>

#include "exec_file.h"
#include "parse.h"

// Standard POSIX environment pointer
extern char** environ;

namespace rpc {

// TODO: Access fd through torrent logging?

int
ExecFile::execute(const char* file, char* const* argv, int flags) {
  // Write the executed command and its parameters to the log fd.
  [[maybe_unused]] int result;

  if (m_log_fd != -1) {
    for (char* const* itr = argv; *itr != NULL; itr++) {
      if (itr == argv)
        result = write(m_log_fd, "\n---\n", sizeof("\n---\n"));
      else
        result = write(m_log_fd, " ", 1);

      result = write(m_log_fd, *itr, std::strlen(*itr));
    }

    result = write(m_log_fd, "\n---\n", sizeof("\n---\n"));
  }

  int pipe_fd[2];

  if ((flags & flag_capture) && pipe(pipe_fd))
    throw torrent::input_error("ExecFile::execute(...) Pipe creation failed.");

  auto clean_fn = [pipe_fd, flags]() {
      if (flags & flag_capture) {
        ::close(pipe_fd[0]);
        ::close(pipe_fd[1]);
      }
    };

  posix_spawn_file_actions_t actions{};

  if (posix_spawn_file_actions_init(&actions) != 0) {
    clean_fn();
    throw torrent::internal_error("ExecFile::execute(...) posix_spawn_file_actions_init failed.");
  }

  // Handle standard input redirection (/dev/null), posix_spawn_file_actions_addopen handles opening
  // and dup2 natively
  if (posix_spawn_file_actions_addopen(&actions, 0, "/dev/null", O_RDWR, 0) != 0) {
    // Fallback if open fails inside action setup
    posix_spawn_file_actions_addclose(&actions, 0);
  }

  // Handle standard output redirection
  if (flags & flag_capture) {
    posix_spawn_file_actions_adddup2(&actions, pipe_fd[1], 1);

    // Ensure the write end of the pipe is closed in the child after duplicating.
    posix_spawn_file_actions_addclose(&actions, pipe_fd[1]);
    posix_spawn_file_actions_addclose(&actions, pipe_fd[0]);

  } else if (m_log_fd != -1) {
    posix_spawn_file_actions_adddup2(&actions, m_log_fd, 1);

  } else {
    posix_spawn_file_actions_addopen(&actions, 1, "/dev/null", O_RDWR, 0);
  }

  if (m_log_fd != -1) {
    posix_spawn_file_actions_adddup2(&actions, m_log_fd, 2);
  } else {
    posix_spawn_file_actions_addopen(&actions, 2, "/dev/null", O_RDWR, 0);
  }

  posix_spawnattr_t attr;
  posix_spawnattr_init(&attr);

  // If you are using standard close-on-exec (O_CLOEXEC) across rtorrent, posix_spawn honors it
  // automatically. If you want to explicitly enforce a clean slate, modern systems support
  // POSIX_SPAWN_CLOEXEC_DEFAULT.

#ifdef POSIX_SPAWN_CLOEXEC_DEFAULT
  posix_spawnattr_setflags(&attr, POSIX_SPAWN_CLOEXEC_DEFAULT);
#endif

  pid_t child_pid{};

  int spawn_status = posix_spawnp(&child_pid, file, &actions, &attr, argv, environ);

  posix_spawn_file_actions_destroy(&actions);
  posix_spawnattr_destroy(&attr);

  if (spawn_status != 0) {
    clean_fn();
    throw torrent::input_error("ExecFile::execute(...) posix_spawn failed: " + std::string(std::strerror(spawn_status)));
  }

  if (flags & flag_capture) {
    m_capture = std::string();
    ::close(pipe_fd[1]);

    char buffer[4096];
    ssize_t length;

    do {
      length = read(pipe_fd[0], buffer, sizeof(buffer));

      if (length > 0)
        m_capture += std::string(buffer, length);

    } while (length > 0);

    ::close(pipe_fd[0]);

    if (m_log_fd != -1) {
      result = write(m_log_fd, "Captured output:\n", sizeof("Captured output:\n"));
      result = write(m_log_fd, m_capture.data(), m_capture.length());
    }
  }

  int status;

  while (waitpid(child_pid, &status, 0) == -1) {
    switch (errno) {
    case EINTR:
      continue;
    case ECHILD:
      throw torrent::internal_error("ExecFile::execute(...) waitpid failed with ECHILD, child process not found.");
    case EINVAL:
      throw torrent::internal_error("ExecFile::execute(...) waitpid failed with EINVAL.");
    default:
      throw torrent::internal_error("ExecFile::execute(...) waitpid failed with unexpected error: " + std::string(std::strerror(errno)));
    }
  };

  // Check return value?
  if (m_log_fd != -1) {
    if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
      result = write(m_log_fd, "\n--- Success ---\n", sizeof("\n--- Success ---\n"));
    else
      result = write(m_log_fd, "\n--- Error ---\n", sizeof("\n--- Error ---\n"));
  }

  return status;
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

}
