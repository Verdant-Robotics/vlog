#include <gtest/gtest.h>

#include "vlog.h"

char really_long_string[100000];

static bool Contains(const std::string_view haystack, const std::string_view needle) {
  return haystack.find(needle) != std::string::npos;
}

static bool EndsWith(const std::string_view text, const std::string_view suffix) {
  if (text.length() >= suffix.length()) {
    return (0 == text.compare(text.length() - suffix.length(), suffix.length(), suffix));
  }
  return false;
}

TEST(TestVLog, StartStop) {
  EXPECT_TRUE(vlog_init());
  EXPECT_TRUE(vlog_init());
  vlog_fini();
  EXPECT_TRUE(vlog_init());
}

TEST(TestVLog, NonFatalLevels) {
  const std::string TOKEN = "3364e2a3-ae5a-4e0a-ab92-36deb0ceb332";

  testing::internal::CaptureStdout();
  vlog_error(VCAT_GENERAL, "%s", TOKEN.c_str());
  const std::string output = testing::internal::GetCapturedStdout();
  EXPECT_TRUE(Contains(output, TOKEN));
  EXPECT_TRUE(EndsWith(output, "\n"));

  testing::internal::CaptureStdout();
  vlog_warning(VCAT_GENERAL, "%s", TOKEN.c_str());
  EXPECT_TRUE(Contains(testing::internal::GetCapturedStdout(), TOKEN));

  testing::internal::CaptureStdout();
  vlog_info(VCAT_GENERAL, "%s", TOKEN.c_str());
  EXPECT_TRUE(Contains(testing::internal::GetCapturedStdout(), TOKEN));

  testing::internal::CaptureStdout();
  vlog_debug(VCAT_GENERAL, "%s", TOKEN.c_str());
  EXPECT_FALSE(Contains(testing::internal::GetCapturedStdout(), TOKEN));

  testing::internal::CaptureStdout();
  vlog_fine(VCAT_GENERAL, "%s", TOKEN.c_str());
  EXPECT_FALSE(Contains(testing::internal::GetCapturedStdout(), TOKEN));

  testing::internal::CaptureStdout();
  vlog_finer(VCAT_GENERAL, "%s", TOKEN.c_str());
  EXPECT_FALSE(Contains(testing::internal::GetCapturedStdout(), TOKEN));

  testing::internal::CaptureStdout();
  vlog_finest(VCAT_GENERAL, "%s", TOKEN.c_str());
  EXPECT_FALSE(Contains(testing::internal::GetCapturedStdout(), TOKEN));

  testing::internal::CaptureStdout();
  vlog_always("%s", TOKEN.c_str());
  EXPECT_TRUE(Contains(testing::internal::GetCapturedStdout(), TOKEN));

  set_log_level_string("DEBUG");

  testing::internal::CaptureStdout();
  vlog_debug(VCAT_GENERAL, "%s", TOKEN.c_str());
  EXPECT_TRUE(Contains(testing::internal::GetCapturedStdout(), TOKEN));

  testing::internal::CaptureStdout();
  vlog_fine(VCAT_GENERAL, "%s", TOKEN.c_str());
  EXPECT_FALSE(Contains(testing::internal::GetCapturedStdout(), TOKEN));

  set_log_level_string("FINEST");

  testing::internal::CaptureStdout();
  vlog_fine(VCAT_GENERAL, "%s", TOKEN.c_str());
  EXPECT_TRUE(Contains(testing::internal::GetCapturedStdout(), TOKEN));

  testing::internal::CaptureStdout();
  vlog_finer(VCAT_GENERAL, "%s", TOKEN.c_str());
  EXPECT_TRUE(Contains(testing::internal::GetCapturedStdout(), TOKEN));

  testing::internal::CaptureStdout();
  vlog_finest(VCAT_GENERAL, "%s", TOKEN.c_str());
  EXPECT_TRUE(Contains(testing::internal::GetCapturedStdout(), TOKEN));

  // Test that long strings are truncated instead of overflowing.
  constexpr int VLOG_SBUFFER_LEN = 8192;  // defined in vlog.cpp
  memset(really_long_string, 'A', sizeof(really_long_string));
  testing::internal::CaptureStdout();
  vlog_info(VCAT_GENERAL, "%s", really_long_string);
  EXPECT_TRUE(testing::internal::GetCapturedStdout().length() == VLOG_SBUFFER_LEN - 1);

  // Repeat with lots of optional information printed
  vlog_option_location = true;
  vlog_option_thread_id = true;
  vlog_option_thread_name = true;
  vlog_option_timelog = true;
  vlog_option_time_date = true;
  vlog_option_print_category = true;
  vlog_option_print_level = true;
  testing::internal::CaptureStdout();
  vlog_info(VCAT_GENERAL, "%s", really_long_string);
  EXPECT_TRUE(testing::internal::GetCapturedStdout().length() == VLOG_SBUFFER_LEN - 1);
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
