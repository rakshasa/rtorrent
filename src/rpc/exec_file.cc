// rTorrent - BitTorrent client
// Copyright (C) 2005-2007, Jari Sundell
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// In addition, as a special exception, the copyright holders give
// permission to link the code of portions of this program with the
// OpenSSL library under certain conditions as described in each
// individual source file, and distribute linked combinations
// including the two.
//
// You must obey the GNU General Public License in all respects for
// all of the code used other than OpenSSL.  If you modify file(s)
// with this exception, you may extend this exception to your version
// of the file(s), but you are not obligated to do so.  If you do not
// wish to do so, delete this exception statement from your version.
// If you delete this exception statement from all source files in the
// program, then also delete it here.
//
// Contact:  Jari Sundell <jaris@ifi.uio.no>
//
//           Skomakerveien 33
//           3185 Skoppum, NORWAY

#include "config.h"

#include <string>
#include <unistd.h>
#include <rak/error_number.h>
#include <rak/path.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "exec_file.h"
#include "parse.h"

namespace rpc {

// Close m_logFd.

int
ExecFile::execute(const char* file, char* const* argv) {
  // Write the execued command and its parameters to the log fd.
  if (m_logFd != -1) {
    for (char* const* itr = argv; *itr != NULL; itr++) {
      if (itr == argv)
        write(m_logFd, "\n---\n", sizeof("\n---\n"));
      else
        write(m_logFd, " ", 1);

      write(m_logFd, *itr, std::strlen(*itr));
    }

    write(m_logFd, "\n---\n", sizeof("\n---\n"));
  }

  pid_t childPid = fork();

  if (childPid == -1)
    throw torrent::input_error("ExecFile::execute(...) Fork failed.");

  if (childPid == 0) {
    ::close(0);
    ::close(1);
    ::close(2);

    if (m_logFd != -1) {
      dup2(m_logFd, 1);
      dup2(m_logFd, 2);
    }

    // Close all fd's.
    for (int i = 3, last = sysconf(_SC_OPEN_MAX); i != last; i++)
      ::close(i);

    int result = execvp(file, argv);

    _exit(result);

  } else {
    int status;
    int wpid = waitpid(childPid, &status, 0);

    while (wpid == -1 && rak::error_number::current().value() == rak::error_number::e_intr)
      wpid = waitpid(childPid, &status, 0);

    if (wpid != childPid)
      throw torrent::internal_error("ExecFile::execute(...) waitpid failed.");

    // Check return value?
    if (m_logFd) {
      if (status == 0)
        write(m_logFd, "\n--- Success ---\n", sizeof("\n--- Success ---\n"));
      else
        write(m_logFd, "\n--- Error ---\n", sizeof("\n--- Error ---\n"));
    }

    return status;
  }
}

torrent::Object
ExecFile::execute_object(const torrent::Object& rawArgs, int flags) {
  char*  argsBuffer[max_args];
  char** argsCurrent = argsBuffer;

  // Size of value strings are less than 24.
  char   valueBuffer[buffer_size];
  char*  valueCurrent = valueBuffer;

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

  *argsCurrent = NULL;

  int status = execute(argsBuffer[0], argsBuffer);

  if ((flags & flag_throw) && status != 0)
    throw torrent::input_error("Bad return code.");

  return torrent::Object((int64_t)status);
}

}
