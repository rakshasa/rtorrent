#include "config.h"

#include <sys/ioctl.h>
#include <termios.h>
#include <torrent/exceptions.h>
#include <unistd.h>

#include "rpc/parse_commands.h"

#include "canvas.h"

namespace display {

bool Canvas::m_initialized{};
bool Canvas::m_daemon{};

// Maps ncurses color IDs to a ncurses attribute int
std::unordered_map<int, int> Canvas::m_attr_map = {};

Canvas::Canvas(int x, int y, int width, int height) {
  if (!m_daemon) {
    m_window = newwin(height, width, y, x);

    if (m_window == nullptr)
      throw torrent::internal_error("Could not allocate ncurses canvas.");
  }
}

Canvas::~Canvas() {
  if (!m_daemon && m_window != nullptr) {
    delwin(m_window);
    m_window = nullptr;
  }
}

void
Canvas::resize(int x, int y, int w, int h) {
  if (!m_daemon) {
    wresize(m_window, h, w);
    mvwin(m_window, y, x);
  }
}

void
Canvas::print_attributes(unsigned int x, unsigned int y, const char* first, const char* last, const attributes_list* attributes) {
  if (!m_daemon) {
    move(x, y);

    attr_t org_attr;
    short  org_pair;
    wattr_get(m_window, &org_attr, &org_pair, NULL);

    attributes_list::const_iterator attrItr = attributes->begin();
    wattr_set(m_window, Attributes::a_normal, Attributes::color_default, NULL);

    while (first != last) {
      const char* next = last;

      if (attrItr != attributes->end()) {
        next = attrItr->position();

        if (first >= next) {
          wattr_set(m_window, attrItr->attributes(), attrItr->colors(), NULL);
          ++attrItr;
        }
      }

      print("%.*s", next - first, first);
      first = next;
    }

    // Reset the color.
    wattr_set(m_window, org_attr, org_pair, NULL);
  }
}

void
Canvas::initialize() {
  if (m_initialized)
    throw torrent::internal_error("Canvas::initialize() called more than once.");

  m_daemon = rpc::call_command_value("system.daemon");
  m_initialized = true;

  if (!m_daemon) {
    initscr();
    start_color();
    use_default_colors();
    Canvas::build_colors();
    raw();
    noecho();
    nodelay(stdscr, TRUE);
    keypad(stdscr, TRUE);
    curs_set(0);
  }
}

void
Canvas::cleanup() {
  if (!m_initialized)
    return;

  m_initialized = false;

  if (!m_daemon) {
    noraw();
    endwin();
  }
}

// Function wrapper for what possibly is a macro
int
get_colors() {
  return COLORS;
}

// Turns the string color definitions from the "ui.color.*" RPC
// commands into valid ncurses color pairs
void
Canvas::build_colors() {

  // This may get called early in the start process by the config
  // file, so we need to delay building until initscr() has a chance
  // to run
  if (!m_initialized || m_daemon)
    return;

  // basic color names, index maps to ncurses COLOR_*
  static constexpr std::array color_names{
    "black", "red", "green", "yellow", "blue", "magenta", "cyan", "white"};

  // Those hold the background colors of "odd" and "even"
  int bg_odd  = -1;
  int bg_even = -1;

  for (int k = 1; k < RCOLOR_MAX; k++) {
    init_pair(k, -1, -1);

    std::string color_def = rpc::call_command_string(color_vars[k]);

    if (color_def.empty())
      continue; // Use terminal default if definition is empty

    short         color[2]  = {-1, -1}; // fg, bg
    short         color_idx = 0;        // 0 = fg; 1 = bg
    short         bright    = 0;
    unsigned long attr      = A_NORMAL;

    // Process string as space-separated words
    size_t start = 0, end = 0;

    while (true) {
      end              = color_def.find(' ', start);
      std::string word = color_def.substr(start, end - start);

      if (word == "bold")
        attr |= A_BOLD;
      else if (word == "standout")
        attr |= A_STANDOUT;
      else if (word == "underline")
        attr |= A_UNDERLINE;
      else if (word == "reverse")
        attr |= A_REVERSE;
      else if (word == "blink")
        attr |= A_BLINK;
      else if (word == "dim")
        attr |= A_DIM;
      else if (word == "on") {
        color_idx = 1;
        bright    = 0;
      } // Switch to background color
      else if (word == "gray" || word == "grey")
        color[color_idx] = bright ? 7 : 8; // Bright gray is white
      else if (word == "bright")
        bright = 8;
      else if (word.find_first_not_of("0123456789") == std::string::npos) {
        // Handle numeric index
        short c = -1;
        sscanf(word.c_str(), "%hd", &c);
        color[color_idx] = c;
      } else
        for (short c = 0; c < 8; c++) { // Check for basic color names
          if (word == color_names[c]) {
            color[color_idx] = bright + c;
            break;
          }
        }
      if (end == std::string::npos)
        break;
      start = end + 1;
    }

    // Check that fg & bg color index is valid
    if ((color[0] != -1 && color[0] >= get_colors()) || (color[1] != -1 && color[1] >= get_colors())) {
      Canvas::cleanup();
      throw torrent::input_error(color_def + ": your terminal only supports " + std::to_string(get_colors()) + " colors.");
    }

    m_attr_map[k] = attr; // overwrite or insert the value
    init_pair(k, color[0], color[1]);
    if (k == RCOLOR_EVEN)
      bg_even = color[1];
    if (k == RCOLOR_ODD)
      bg_odd = color[1];
  }

  // Now make copies of the basic colors with the "odd" and "even" definitions mixed in
  for (int k = 1; k < RCOLOR_MAX; k++) {
    short fg, bg;
    pair_content(k, &fg, &bg);

    // Replace the background color, and mix in the attributes
    m_attr_map[k + 1 * RCOLOR_MAX] = m_attr_map[k] | m_attr_map[RCOLOR_EVEN];
    m_attr_map[k + 2 * RCOLOR_MAX] = m_attr_map[k] | m_attr_map[RCOLOR_ODD];
    init_pair(k + 1 * RCOLOR_MAX, fg, bg == -1 ? bg_even : bg);
    init_pair(k + 2 * RCOLOR_MAX, fg, bg == -1 ? bg_odd : bg);
  }
}

std::pair<int, int>
Canvas::term_size() {
  struct winsize ws;

  if (!m_daemon) {
    if (ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) == 0)
      return std::pair<int, int>(ws.ws_col, ws.ws_row);
  }
  return std::pair<int, int>(80, 24);
}

} // namespace display
