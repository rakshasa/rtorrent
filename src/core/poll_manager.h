#ifndef RTORRENT_CORE_POLL_MANAGER_H
#define RTORRENT_CORE_POLL_MANAGER_H

#include "curl_stack.h"

namespace torrent {
class Poll;
}

namespace core {

torrent::Poll* create_poll();

}

#endif
