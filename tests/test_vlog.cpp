#include <gtest/gtest.h>

#include "backward/callstack.h"
#include "vlog.h"

static bool Contains(const std::string_view haystack, const std::string_view needle) {
  return haystack.find(needle) != std::string::npos;
}

static bool EndsWith(const std::string_view text, const std::string_view suffix) {
  if (text.length() >= suffix.length()) {
    return (0 == text.compare(text.length() - suffix.length(), suffix.length(), suffix));
  }
  return false;
}

static bool HaveDebugSymbols() {
#ifndef NDEBUG
  // Force the assumption of debug symbols for debug builds. This will induce a
  // test failure if they are not present in a debug build for whatever reason
  return true;
#else
  std::ostringstream out;
  PrintCurrentCallstack(out, false, nullptr, 0);
  return Contains(out.str(), "test_vlog.cpp");
#endif
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
}

TEST(TestVLog, Fatal) {
  const std::string TOKEN = "d08206d9-211f-4a16-a7de-14417a8df699";

  vlog_option_exit_on_fatal = false;

  testing::internal::CaptureStdout();
  vlog_fatal(VCAT_GENERAL, "%s", TOKEN.c_str());
  EXPECT_TRUE(Contains(testing::internal::GetCapturedStdout(), TOKEN));

  vlog_option_exit_on_fatal = true;
  testing::internal::CaptureStdout();
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wused-but-marked-unused"
#pragma clang diagnostic ignored "-Wcovered-switch-default"
  // clang-format off
  ASSERT_DEATH({
    vlog_fatal(VCAT_GENERAL, "%s", TOKEN.c_str());
  }, "");
  // clang-format on
#pragma clang diagnostic pop
  std::string crash = testing::internal::GetCapturedStdout();
  EXPECT_TRUE(Contains(crash, TOKEN));
  if (HaveDebugSymbols()) {
    std::cout << "Debug symbols are present\n";
    EXPECT_TRUE(Contains(crash, "vlog_fatal(VCAT_GENERAL"));
    EXPECT_TRUE(Contains(crash, "TestVLog_Fatal_Test::TestBody()"));
    EXPECT_TRUE(Contains(crash, "tests/test_vlog.cpp"));

    if (!Contains(crash, "vlog_fatal(VCAT_GENERAL")) {
      std::cout << "Captured Output:\n" << crash << '\n';
    }
  } else {
    std::cout << "No debug symbols present\n";
    EXPECT_TRUE(Contains(crash, "tests/test_vlog"));

    if (!Contains(crash, "tests/test_vlog")) {
      std::cout << "Captured Output:\n" << crash << '\n';
    }
  }
}

TEST(TestVLog, Assert) {
  const std::string TOKEN = "1ecdf1cb-7524-41b9-b048-280db1f2cb44";

  testing::internal::CaptureStdout();
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wused-but-marked-unused"
#pragma clang diagnostic ignored "-Wcovered-switch-default"
  // clang-format off
  ASSERT_DEATH({
    VLOG_ASSERT(false, "%s", TOKEN.c_str());
  }, "");
  // clang-format on
#pragma clang diagnostic pop
  std::string crash = testing::internal::GetCapturedStdout();
  EXPECT_TRUE(Contains(crash, TOKEN));
  if (HaveDebugSymbols()) {
    std::cout << "Debug symbols are present\n";
    EXPECT_TRUE(Contains(crash, "VLOG_ASSERT("));
    EXPECT_TRUE(Contains(crash, "TestVLog_Assert_Test::TestBody()"));
    EXPECT_TRUE(Contains(crash, "tests/test_vlog.cpp"));

    if (!Contains(crash, "VLOG_ASSERT(")) {
      std::cout << "Captured Output:\n" << crash << '\n';
    }
  } else {
    std::cout << "No debug symbols present\n";
    EXPECT_TRUE(Contains(crash, "tests/test_vlog"));

    if (!Contains(crash, "tests/test_vlog")) {
      std::cout << "Captured Output:\n" << crash << '\n';
    }
  }
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
