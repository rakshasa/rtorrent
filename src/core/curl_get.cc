#include "config.h"

#include <iostream>
#include <curl/curl.h>
#include <curl/easy.h>
#include <torrent/exceptions.h>

#include "curl_get.h"
#include "curl_stack.h"

namespace core {

CurlGet::CurlGet(CurlStack* s) :
  m_handle(NULL),
  m_stack(s) {

  if (m_stack == NULL)
    throw torrent::client_error("Tried to create CurlGet without a valid CurlStack");
}

CurlGet::~CurlGet() {
  close();
}

CurlGet*
CurlGet::new_object(CurlStack* s) {
  return new CurlGet(s);
}

void
CurlGet::start() {
  if (is_busy())
    throw torrent::internal_error("Tried to call CurlGet::start on a busy object");

  if (m_stream == NULL)
    throw torrent::internal_error("Tried to call CurlGet::start without a valid output stream");

  m_handle = curl_easy_init();

  curl_easy_setopt(m_handle, CURLOPT_URL,           m_url.c_str());
  curl_easy_setopt(m_handle, CURLOPT_USERAGENT,     m_userAgent.c_str());
  curl_easy_setopt(m_handle, CURLOPT_WRITEFUNCTION, &curl_get_receive_write);
  curl_easy_setopt(m_handle, CURLOPT_WRITEDATA,     this);
  curl_easy_setopt(m_handle, CURLOPT_FORBID_REUSE,  1);

  m_stack->add_get(this);
}

void
CurlGet::close() {
  if (!is_busy())
    return;

  m_stack->remove_get(this);

  curl_easy_cleanup(m_handle);

  m_handle = NULL;
}


double
CurlGet::get_size_done() {
  double d = 0.0;
  curl_easy_getinfo(m_handle, CURLINFO_SIZE_DOWNLOAD, &d);

  return d;
}

double
CurlGet::get_size_total() {
  double d = 0.0;
  curl_easy_getinfo(m_handle, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &d);

  return d;
}

void
CurlGet::perform(CURLMsg* msg) {
  if (msg->msg != CURLMSG_DONE)
    throw torrent::client_error("CurlGet::process got CURLMSG that isn't done");

  if (msg->data.result == CURLE_OK)
    m_signalDone.emit();
  else
    m_signalFailed.emit(curl_easy_strerror(msg->data.result));
}

size_t
curl_get_receive_write(void* data, size_t size, size_t nmemb, void* handle) {
  return ((CurlGet*)handle)->m_stream->write((char*)data, size * nmemb).fail() ? 0 : size * nmemb;
}

}
