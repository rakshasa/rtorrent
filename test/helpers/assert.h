#ifndef HELPERS_ASSERT_H
#define HELPERS_ASSERT_H

#define ASSERT_CATCH_INPUT_ERROR(some_code)                             \
 try { some_code; CPPUNIT_ASSERT("torrent::input_error not caught" && false); } catch (torrent::input_error& e) { }

#endif
