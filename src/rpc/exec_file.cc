#include "config.h"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <string>
#include <unistd.h>
#include <rak/path.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <torrent/utils/thread.h>

#include "exec_file.h"
#include "parse.h"

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

  int pipeFd[2];

  if ((flags & flag_capture) && pipe(pipeFd))
    throw torrent::input_error("ExecFile::execute(...) Pipe creation failed.");

  pid_t childPid = fork();

  if (childPid == -1)
    throw torrent::input_error("ExecFile::execute(...) Fork failed.");

  if (childPid == 0) {
    if (flags & flag_background) {
      pid_t detached_pid = fork();

      if (detached_pid == -1)
        _exit(-1);

      if (detached_pid != 0) {
        if (m_log_fd != -1)
          result = write(m_log_fd, "\n--- Background task ---\n", sizeof("\n--- Background task ---\n"));

        _exit(0);
      }

      m_log_fd = -1;
      flags &= ~flag_capture;
    }

    int devNull = open("/dev/null", O_RDWR);

    if (devNull != -1)
      dup2(devNull, 0);
    else
      ::close(0);

    if (flags & flag_capture)
      dup2(pipeFd[1], 1);
    else if (m_log_fd != -1)
      dup2(m_log_fd, 1);
    else if (devNull != -1)
      dup2(devNull, 1);
    else
      ::close(1);

    if (m_log_fd != -1)
      dup2(m_log_fd, 2);
    else if (devNull != -1)
      dup2(devNull, 2);
    else
      ::close(2);

    // Close all fd's.
    for (int i = 3, last = sysconf(_SC_OPEN_MAX); i != last; i++)
      ::close(i);

    result = execvp(file, argv);

    _exit(result);
  }

  if (flags & flag_capture) {
    m_capture = std::string();
    ::close(pipeFd[1]);

    char buffer[4096];
    ssize_t length;

    do {
      length = read(pipeFd[0], buffer, sizeof(buffer));

      if (length > 0)
        m_capture += std::string(buffer, length);
    } while (length > 0);

    ::close(pipeFd[0]);

    if (m_log_fd != -1) {
      result = write(m_log_fd, "Captured output:\n", sizeof("Captured output:\n"));
      result = write(m_log_fd, m_capture.data(), m_capture.length());
    }
  }

  int status;

  while (waitpid(childPid, &status, 0) == -1) {
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
