#pragma once

#include <signal.h>

#include <iostream>
#include <string>

constexpr size_t MAX_STACK_FRAMES = 64;

namespace backward {
class StackTrace;
};

std::string GetCurrentCallstack(bool color = true);

void PrintCurrentCallstack(std::ostream& output, bool color = true, siginfo_t* info = nullptr,
                           size_t skipFrames = 0);

void PrintCallstack(std::ostream& output, const backward::StackTrace& callstack, bool color = true);
