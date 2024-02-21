#include <graph.h>
#include <gtest/gtest.h>
#include <library/const_string.h>

TEST(test_const_string, simple) {
  using lib::const_string;

  const_string s = "1234";

  const_string ss = s;

  ASSERT_EQ(s, "1234");
  ASSERT_EQ(s, ss);

  nonstd::string_view view = ss;
  ASSERT_EQ(view, ss);

  ASSERT_NE(ss, "124");
}