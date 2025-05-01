#ifndef RTORRENT_DISPLAY_FILE_LIST_H
#define RTORRENT_DISPLAY_FILE_LIST_H

#include "window.h"

namespace ui {
  class ElementFileList;
}

namespace torrent {
  class File;
  class FileListIterator;
  class file_list_collapsed_iterator;
}

namespace display {

class WindowFileList : public Window {
public:
  typedef torrent::FileListIterator             iterator;
  typedef torrent::file_list_collapsed_iterator collapsed_iterator;

  WindowFileList(const ui::ElementFileList* element);

  virtual void        redraw();

private:
  int                 done_percentage(torrent::File* e);

  const ui::ElementFileList* m_element;
};

}

#endif
