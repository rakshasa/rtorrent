#ifndef LIBTORRENT_HELPER_PROTECTORS_H
#define LIBTORRENT_HELPER_PROTECTORS_H

#include <functional>
#include <string>
#include <cppunit/Protector.h>

class ExceptionProtector : public CppUnit::Protector {
public:
  bool protect(const CppUnit::Functor &functor, const CppUnit::ProtectorContext &context) override;
};

#endif // LIBTORRENT_HELPER_PROTECTORS_H
