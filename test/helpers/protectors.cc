#include <cppunit/Exception.h>
#include <cppunit/Message.h>
#include <cppunit/extensions/TypeInfoHelper.h>
#include <test/helpers/protectors.h>
#include <torrent/exceptions.h>
#include <typeinfo>

bool
ExceptionProtector::protect(const CppUnit::Functor& functor, const CppUnit::ProtectorContext& context) {
  try {
    return functor();

  } catch (CppUnit::Exception &failure) {
    reportFailure( context, failure );

  } catch (torrent::base_error& e) {
    std::string short_description("uncaught exception of base type torrent::base_error: " + std::string(typeid(e).name()));

    CppUnit::Message message(short_description, e.what());
    reportError(context, message);

  } catch ( std::exception &e ) {
    std::string short_description("uncaught exception of type ");

    short_description += CppUnit::TypeInfoHelper::getClassName(typeid(e));

    CppUnit::Message message(short_description, e.what());
    reportError(context, message);

  } catch ( ... ) {
    reportError(context, CppUnit::Message("uncaught exception of unknown type"));
  }

  return false;
}
