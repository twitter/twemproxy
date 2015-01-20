#include "gtest/gtest.h"
extern "C" {
#include "hashkit/nc_hashkit.h"
}

static char UTF8_ZHONG[] = "\xe4\xb8\xad";  // "中" in UTF-8


TEST(hashkit, crc32a) {
  ASSERT_EQ(hash_crc32a("", 0U), 0x0U);
  ASSERT_EQ(hash_crc32a("123456", 6U), 0x972d361U);
  ASSERT_EQ(hash_crc32a("abcdef", 6U), 0x4b8e39efU);
  ASSERT_EQ(hash_crc32a(UTF8_ZHONG, 3U), 0xd5d15a8bU);
}
