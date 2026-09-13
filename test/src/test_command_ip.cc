#include "config.h"

#include "test/src/test_command_ip.h"

#include <cstdint>

CPPUNIT_TEST_SUITE_REGISTRATION(TestCommandIp);

bool ipv4_range_parse(const char* address, uint32_t* address_start, uint32_t* address_end);

static uint32_t
ipv4(uint32_t a, uint32_t b, uint32_t c, uint32_t d) {
  return (a << 24) | (b << 16) | (c << 8) | d;
}

#define RANGE_ASSERT(address, expected_start, expected_end)             \
  {                                                                     \
    uint32_t start = 0;                                                 \
    uint32_t end = 0;                                                   \
                                                                        \
    CPPUNIT_ASSERT(ipv4_range_parse(address, &start, &end));            \
    CPPUNIT_ASSERT_EQUAL(expected_start, start);                        \
    CPPUNIT_ASSERT_EQUAL(expected_end, end);                            \
  }

void
TestCommandIp::test_single_address() {
  RANGE_ASSERT("10.1.2.3", ipv4(10, 1, 2, 3), ipv4(10, 1, 2, 3));
}

void
TestCommandIp::test_explicit_range() {
  RANGE_ASSERT("10.1.2.3-10.1.2.9", ipv4(10, 1, 2, 3), ipv4(10, 1, 2, 9));
}

void
TestCommandIp::test_cidr() {
  RANGE_ASSERT("10.0.0.0/8", ipv4(10, 0, 0, 0), ipv4(10, 255, 255, 255));
  RANGE_ASSERT("10.1.2.0/24", ipv4(10, 1, 2, 0), ipv4(10, 1, 2, 255));
  RANGE_ASSERT("10.1.2.128/25", ipv4(10, 1, 2, 128), ipv4(10, 1, 2, 255));
  RANGE_ASSERT("10.1.2.3/31", ipv4(10, 1, 2, 2), ipv4(10, 1, 2, 3));
}

void
TestCommandIp::test_cidr_zero_mask() {
  RANGE_ASSERT("0.0.0.0/0", ipv4(0, 0, 0, 0), ipv4(255, 255, 255, 255));
  RANGE_ASSERT("10.1.2.3/0", ipv4(0, 0, 0, 0), ipv4(255, 255, 255, 255));
}

void
TestCommandIp::test_cidr_full_mask() {
  RANGE_ASSERT("10.1.2.3/32", ipv4(10, 1, 2, 3), ipv4(10, 1, 2, 3));
  RANGE_ASSERT("0.0.0.0/32", ipv4(0, 0, 0, 0), ipv4(0, 0, 0, 0));
  RANGE_ASSERT("255.255.255.255/32", ipv4(255, 255, 255, 255), ipv4(255, 255, 255, 255));
}
