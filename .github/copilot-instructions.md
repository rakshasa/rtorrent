# Copilot Behavior Guidelines
- The environment is strictly Ubuntu 22.04.
- All compilation tools (automake, aclocal, libtoolize, autoconf) are fully pre-installed.
- Never suggest commands that probe the system architecture, OS release version, or package manager availability.
- Always output direct compilation commands without environment-checking wrappers.

# Environment Initialization & Session Scope
- **This session may start account-scoped without a repository checkout.** 
- If the repository files are not present in the current working directory, you must immediately perform a repository checkout to establish the workspace.
- **Explicitly initialize standard tool paths.** Ensure `/usr/bin`, `/usr/local/bin`, and `/usr/share/aclocal` are actively appended to the PATH and fully exported before running any tools, preventing "aclocal not found" errors.

# Project Architecture & Bootstrapping Strategy
- **This is a barebones Autotools repository** centered around `configure.ac` and `Makefile.am`.
- **There is no `autogen.sh`, `bootstrap`, or pre-generated `configure` script.** Do not look for or check for their existence.
- Skip all environment verification loops. Once the workspace is established, proceed directly to these sequential execution steps:
  1. `autoreconf -ivf`
  2. `./configure`
  3. `make`
