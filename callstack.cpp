#include "backward/callstack.h"

#include <string>
#include <string_view>

#include "backward/backward.h"

#define COLOR(code, s) (color ? "\033[0;" #code "m" : "") << s << (color ? "\033[0;m" : "")

namespace backward {
int cfile_streambuf::underflow() { return traits_type::eof(); }
}  // namespace backward

static std::string FormatFunction(const std::string& function) {
  return function.empty() ? std::string{"??"} : function;
}

static std::string FormatFilename(const std::string& filename) {
  return filename.empty() ? std::string{"??"} : filename;
}

static bool EndsWith(const std::string_view text, const std::string_view suffix) {
  if (text.length() >= suffix.length()) {
    return (0 == text.compare(text.length() - suffix.length(), suffix.length(), suffix));
  }
  return false;
}

static bool ShouldSkipFrame(const backward::ResolvedTrace& tr) {
  return EndsWith(tr.source.filename, "/backward.h") || EndsWith(tr.source.filename, "/callstack.cpp");
}

static void PrintSourceLocation(std::ostream& output, bool color,
                                const backward::ResolvedTrace::SourceLoc& source, ssize_t frame = -1,
                                void* addr = nullptr, std::string_view objectFilename = {}) {
  if (frame >= 0) {
    output << (frame < 10 ? " #" : "#") << frame << " ";
  } else {
    output << "    ";
  }

  if (addr != nullptr) {
    output << COLOR(35, addr) << " ";
  }

  auto func = FormatFunction(source.function);
  auto file = FormatFilename(source.filename);
  output << COLOR(36, func) << " in " << COLOR(33, file) << ":" << COLOR(32, source.line) << "\n";

  if (!objectFilename.empty()) {
    output << "    from object " << COLOR(31, objectFilename) << "\n";
  }
}

static void PrintSnippet(std::ostream& output, bool color, const backward::SnippetFactory::lines_t& snippet,
                         unsigned int sourceLine) {
  for (const auto& [num, line] : snippet) {
    if (num == sourceLine) {
      output << COLOR(31, ">" << std::setw(4) << num << ": " << line) << "\n";
    } else {
      output << COLOR(90, std::setw(5) << num << ": " << line) << "\n";
    }
  }
}

std::string GetCurrentCallstack(bool color) {
  std::stringstream stack;
  PrintCurrentCallstack(stack, color);
  return stack.str();
}

void PrintCurrentCallstack(std::ostream& output, bool color, siginfo_t* info, size_t skipFrames) {
  backward::StackTrace st;
  if (info == nullptr) {
    st.load_here(MAX_STACK_FRAMES);
  } else {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdisabled-macro-expansion"
    st.load_from(info->si_addr, MAX_STACK_FRAMES);
#pragma clang diagnostic pop
  }
  if (skipFrames > 0) st.skip_n_firsts(skipFrames);
  PrintCallstack(output, st, color);
}

void PrintCallstack(std::ostream& output, const backward::StackTrace& callstack, bool color) {
  backward::TraceResolver resolver;
  backward::SnippetFactory snippets;

  resolver.load_stacktrace(callstack);

  output << "Stack trace (most recent call first)";
  if (callstack.thread_id()) {
    output << " in thread " << COLOR(31, callstack.thread_id());
  }
  output << ":\n";

  size_t skipped = 0;
  bool handledSnippet = false;
  for (size_t i = 0; i < callstack.size(); ++i) {
    backward::ResolvedTrace tr = resolver.resolve(callstack[i]);
    if (ShouldSkipFrame(tr)) {
      ++skipped;
      continue;
    }

    // Invalid frames appear as address 0xffffffffffffffff
    if (reinterpret_cast<size_t>(tr.addr) == std::numeric_limits<size_t>::max()) {
      break;
    }

    if (!handledSnippet) {
      handledSnippet = true;
      auto lines = snippets.get_snippet(tr.source.filename, tr.source.line, 5);
      if (!lines.empty()) {
        output << "\n";
        PrintSnippet(output, color, lines, tr.source.line);
        output << "\n";
      }
    }

    // Print a line for each inlined function connected to this stack frame
    for (const auto& source : tr.inliners) {
      PrintSourceLocation(output, color, source);
    }

    // Print a line for the real stack frame
    PrintSourceLocation(output, color, tr.source, ssize_t(i) - ssize_t(skipped), tr.addr, tr.object_filename);
  }
}
