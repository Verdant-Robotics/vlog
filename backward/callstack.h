#pragma once

#include <iostream>
#include <signal.h>
#include <string>

constexpr size_t MAX_STACK_FRAMES = 64;

namespace backward
{
  class StackTrace;
};

std::string GetCurrentCallstack( bool color = true );

void PrintCurrentCallstack( std::ostream& output, bool color = true, siginfo_t* info = nullptr );

void PrintCallstack( std::ostream& output, const backward::StackTrace& callstack, bool color = true );
