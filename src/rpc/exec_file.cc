// rTorrent - BitTorrent client
// Copyright (C) 2006, Jari Sundell
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

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "exec_file.h"

namespace rpc {

int
ExecFile::execute(const char* file, char* const* argv) {
  pid_t childPid = fork();

  if (childPid == -1)
    throw torrent::input_error("ExecFile::execute(...) Fork failed.");

  if (childPid == 0) {
    // Close all fd's.
    for (int i = 0, last = sysconf(_SC_OPEN_MAX); i != last; i++)
      ::close(i);

    int result = execvp(file, argv);

    _exit(result);

  } else {
    int status;

    if (waitpid(childPid, &status, 0) != childPid)
      throw torrent::internal_error("ExecFile::execute(...) waitpid failed.");

    // Check return value?

    return status;
  }
}

torrent::Object
ExecFile::execute_object(const torrent::Object& rawArgs) {
  char*  argsBuffer[128];
  char** argsCurrent = argsBuffer;

  // Size of strings are less than 24.
  char   valueBuffer[3072];
  char*  valueCurrent = valueBuffer;

  const torrent::Object::list_type& args = rawArgs.as_list();

  if (args.empty())
    throw torrent::input_error("Too few arguments.");

  for (torrent::Object::list_type::const_iterator itr = args.begin(), last = args.end(); itr != last; itr++, argsCurrent++) {
    if (argsCurrent == argsBuffer + 128 - 1)
      throw torrent::input_error("Too many arguments.");

    switch (itr->type()) {
    case torrent::Object::TYPE_STRING:
      *argsCurrent = const_cast<char*>(itr->as_string().c_str());
      break;

    case torrent::Object::TYPE_VALUE:
      *argsCurrent = valueCurrent;

      valueCurrent += std::max(snprintf(valueCurrent, valueBuffer + 3072 - valueCurrent, "%lli", itr->as_value()), 0);
      break;

    default:
      throw torrent::input_error("Invalid type.");
    }
  }

  *argsCurrent = NULL;

  int status = execute(argsBuffer[0], argsBuffer);

  if (status != 0)
    throw torrent::input_error("ExecFile::execute_object(...) status != 0.");

  return torrent::Object();
}

}
