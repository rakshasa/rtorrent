#include "config.h"

#include <fstream>
#include <stdexcept>
#include <torrent/bencode.h>
#include <torrent/torrent.h>

#include "utils/parse.h"

#include "download.h"
#include "download_store.h"

namespace core {

void
DownloadStore::activate(const std::string& path) {
  m_path = path;

  if (m_path.empty())
    throw std::logic_error("core::DownloadStore::activate(...) received an empty path");

  if (*m_path.rbegin() != '/')
    m_path += '/';
}

void
DownloadStore::disable() {
  m_path = "";
}

void
DownloadStore::save(Download* d) {
  std::fstream f(create_filename(d).c_str(), std::ios::out | std::ios::trunc);

  if (!f.is_open())
    throw std::runtime_error("core::DownloadStore::save(...) could not open file");

  f << torrent::download_bencode(d->get_hash());

  if (f.fail())
    throw std::runtime_error("core::DownloadStore::save(...) could not write torrent");
}

void
DownloadStore::remove(Download* d) {
}

std::string
DownloadStore::create_filename(Download* d) {
  return m_path + utils::string_to_hex(d->get_hash()) + ".torrent";
}

}
