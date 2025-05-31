#include "config.h"

#include "test/helpers/mock_function.h"

#include <fcntl.h>
#include <iostream>
#include <unistd.h>

#include "torrent/event.h"
#include "torrent/net/socket_address.h"
#include "torrent/net/fd.h"
#include "torrent/utils/log.h"
#include "torrent/utils/random.h"

#define MOCK_CLEANUP_MAP(MOCK_FUNC) \
  CPPUNIT_ASSERT_MESSAGE("expected mock function calls not completed for '" #MOCK_FUNC "'", mock_cleanup_map(&MOCK_FUNC) || ignore_assert);
#define MOCK_LOG(log_fmt, ...)                                          \
  lt_log_print(torrent::LOG_MOCK_CALLS, "%s: " log_fmt, __func__, __VA_ARGS__);

void
mock_clear(bool ignore_assert) {
  MOCK_CLEANUP_MAP(torrent::fd__accept);
  MOCK_CLEANUP_MAP(torrent::fd__bind);
  MOCK_CLEANUP_MAP(torrent::fd__close);
  MOCK_CLEANUP_MAP(torrent::fd__connect);
  MOCK_CLEANUP_MAP(torrent::fd__fcntl_int);
  MOCK_CLEANUP_MAP(torrent::fd__listen);
  MOCK_CLEANUP_MAP(torrent::fd__setsockopt_int);
  MOCK_CLEANUP_MAP(torrent::fd__socket);

  MOCK_CLEANUP_MAP(torrent::this_thread::event_open);
  MOCK_CLEANUP_MAP(torrent::this_thread::event_open_and_count);
  MOCK_CLEANUP_MAP(torrent::this_thread::event_close_and_count);
  MOCK_CLEANUP_MAP(torrent::this_thread::event_closed_and_count);
  MOCK_CLEANUP_MAP(torrent::this_thread::event_insert_read);
  MOCK_CLEANUP_MAP(torrent::this_thread::event_insert_write);
  MOCK_CLEANUP_MAP(torrent::this_thread::event_insert_error);
  MOCK_CLEANUP_MAP(torrent::this_thread::event_remove_read);
  MOCK_CLEANUP_MAP(torrent::this_thread::event_remove_write);
  MOCK_CLEANUP_MAP(torrent::this_thread::event_remove_error);
  MOCK_CLEANUP_MAP(torrent::this_thread::event_remove_and_close);

  MOCK_CLEANUP_MAP(torrent::random_uniform_uint16);
  MOCK_CLEANUP_MAP(torrent::random_uniform_uint32);

  mock_compare_map<torrent::Event>::values.clear();
};

void
mock_init() {
  log_add_group_output(torrent::LOG_MOCK_CALLS, "test_output");
  mock_clear(true);
}

void
mock_cleanup() {
  mock_clear(false);
}

void
mock_redirect_defaults([[maybe_unused]] mock_redirect_flags flags) {
  mock_redirect(torrent::fd__close, std::function<int(int fildes)>([](int fildes) { return ::close(fildes); }));
  mock_redirect(torrent::fd__fcntl_int, std::function<int(int fildes, int cmd, int arg)>([](int fildes, int cmd, int arg) { return ::fcntl(fildes, cmd, arg); }));
  mock_redirect(torrent::fd__setsockopt_int, std::function<int(int socket, int level, int option_name, int option_value)>([](int socket, int level, int option_name, int option_value) { return ::setsockopt(socket, level, option_name, &option_value, sizeof(int)); }));
  mock_redirect(torrent::fd__socket, std::function<int(int domain, int type, int protocol)>([](int domain, int type, int protocol) { return ::socket(domain, type, protocol); }));
}

namespace torrent {

//
// Mock functions for 'torrent/net/fd.h':
//

int fd__accept(int socket, sockaddr *address, socklen_t *address_len) {
  MOCK_LOG("entry socket:%i address:%s address_len:%u",
           socket, torrent::sa_pretty_str(address).c_str(), (unsigned int)(*address_len));
  auto ret = mock_call<int>(__func__, &torrent::fd__accept, socket, address, address_len);
  MOCK_LOG("exit socket:%i address:%s address_len:%u",
           socket, torrent::sa_pretty_str(address).c_str(), (unsigned int)(*address_len));
  return ret;
}

int fd__bind(int socket, const sockaddr *address, socklen_t address_len) {
  MOCK_LOG("socket:%i address:%s address_len:%u",
           socket, torrent::sa_pretty_str(address).c_str(), (unsigned int)address_len);
  return mock_call<int>(__func__, &torrent::fd__bind, socket, address, address_len);
}

int fd__close(int fildes) {
  MOCK_LOG("filedes:%i", fildes);
  return mock_call<int>(__func__, &torrent::fd__close, fildes);
}

int fd__connect(int socket, const sockaddr *address, socklen_t address_len) {
  MOCK_LOG("socket:%i address:%s address_len:%u",
           socket, torrent::sa_pretty_str(address).c_str(), (unsigned int)address_len);
  return mock_call<int>(__func__, &torrent::fd__connect, socket, address, address_len);
}

int fd__fcntl_int(int fildes, int cmd, int arg) {
  MOCK_LOG("filedes:%i cmd:%i arg:%i", fildes, cmd, arg);
  return mock_call<int>(__func__, &torrent::fd__fcntl_int, fildes, cmd, arg);
}

int fd__listen(int socket, int backlog) {
  MOCK_LOG("socket:%i backlog:%i", socket, backlog);
  return mock_call<int>(__func__, &torrent::fd__listen, socket, backlog);
}

int fd__setsockopt_int(int socket, int level, int option_name, int option_value) {
  MOCK_LOG("socket:%i level:%i option_name:%i option_value:%i",
           socket, level, option_name, option_value);
  return mock_call<int>(__func__, &torrent::fd__setsockopt_int, socket, level, option_name, option_value);
}

int fd__socket(int domain, int type, int protocol) {
  MOCK_LOG("domain:%i type:%i protocol:%i", domain, type, protocol);
  return mock_call<int>(__func__, &torrent::fd__socket, domain, type, protocol);
}

//
// Mock functions for 'torrent/common.h':
//

namespace this_thread {

void event_open(Event* event) {
  MOCK_LOG("fd:%i type_name:%s", event->file_descriptor(), event->type_name());
  return mock_call<void>(__func__, &torrent::this_thread::event_open, event);
}

void event_open_and_count(Event* event) {
  MOCK_LOG("fd:%i type_name:%s", event->file_descriptor(), event->type_name());
  return mock_call<void>(__func__, &torrent::this_thread::event_open_and_count, event);
}

void event_close_and_count(Event* event) {
  MOCK_LOG("fd:%i type_name:%s", event->file_descriptor(), event->type_name());
  return mock_call<void>(__func__, &torrent::this_thread::event_close_and_count, event);
}

void event_closed_and_count(Event* event) {
  MOCK_LOG("fd:%i type_name:%s", event->file_descriptor(), event->type_name());
  return mock_call<void>(__func__, &torrent::this_thread::event_closed_and_count, event);
}

void event_insert_read(Event* event) {
  MOCK_LOG("fd:%i type_name:%s", event->file_descriptor(), event->type_name());
  return mock_call<void>(__func__, &torrent::this_thread::event_insert_read, event);
}

void event_insert_write(Event* event) {
  MOCK_LOG("fd:%i type_name:%s", event->file_descriptor(), event->type_name());
  return mock_call<void>(__func__, &torrent::this_thread::event_insert_write, event);
}

void event_insert_error(Event* event) {
  MOCK_LOG("fd:%i type_name:%s", event->file_descriptor(), event->type_name());
  return mock_call<void>(__func__, &torrent::this_thread::event_insert_error, event);
}

void event_remove_read(Event* event) {
  MOCK_LOG("fd:%i type_name:%s", event->file_descriptor(), event->type_name());
  return mock_call<void>(__func__, &torrent::this_thread::event_remove_read, event);
}

void event_remove_write(Event* event) {
  MOCK_LOG("fd:%i type_name:%s", event->file_descriptor(), event->type_name());
  return mock_call<void>(__func__, &torrent::this_thread::event_remove_write, event);
}

void event_remove_error(Event* event) {
  MOCK_LOG("fd:%i type_name:%s", event->file_descriptor(), event->type_name());
  return mock_call<void>(__func__, &torrent::this_thread::event_remove_error, event);
}

void event_remove_and_close(Event* event) {
  MOCK_LOG("fd:%i type_name:%s", event->file_descriptor(), event->type_name());
  return mock_call<void>(__func__, &torrent::this_thread::event_remove_and_close, event);
}

}

//
// Mock functions for 'torrent/utils/random.h':
//

uint16_t random_uniform_uint16(uint16_t min, uint16_t max) {
  MOCK_LOG("min:%" PRIu16 " max:%" PRIu16, min, max);
  return mock_call<uint16_t>(__func__, &torrent::random_uniform_uint16, min, max);
}

uint32_t random_uniform_uint32(uint32_t min, uint32_t max) {
  MOCK_LOG("min:%" PRIu32 " max:%" PRIu32, min, max);
  return mock_call<uint32_t>(__func__, &torrent::random_uniform_uint32, min, max);
}

}
