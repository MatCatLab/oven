#include "base/command_line.h"

#include <algorithm>
#include <cassert>
#include <fstream>
#include <iterator>
#include <sstream>
#include <stdexcept>

namespace {
const wchar_t kDashes[] = L"--";
const wchar_t kHelp[] = L"help";

bool StartsWithDashes(const std::wstring_view& input) {
  return input.length() >= 2 && input.substr(0, 2) == kDashes;
}
bool IsResponseFile(const std::wstring_view& input) {
  return !input.empty() && input[0] == L'@';
}
}  // anonymous namespace

namespace oven {
namespace base {
CommandLine::CommandLine(int argc, wchar_t** argv) : executable_(argv[0]) {
  initial_arguments_.reserve(kMaxNumberOfArguments);
  for (int arg = 1; arg < std::min(int(kMaxNumberOfArguments), argc); ++arg) {
    initial_arguments_.emplace_back(argv[arg]);
  }

  expected_arguments_.emplace(
      kHelp, ExpectedArgument{
                 L"Show this message",
                 ArgumentType::kBool,
                 /* is_optional */ true,
             });
}

void CommandLine::AddArgument(const std::wstring_view argument,
                              const std::wstring_view help,
                              const ArgumentType expected_type) {
#if defined(ENABLE_ASSERTIONS)
  assert(argument != kHelp && L"'help' is a reserved argument");
  assert(!argument.empty() && L"Empty arguments are not supported");
  assert(argument.find(L'=') == argument.npos &&
         L"Character '=' is not allowed in argument name");
#endif
  expected_arguments_.emplace(
      argument, ExpectedArgument{
                    help,
                    expected_type,
                    /* is_optional */ false,
                });
}

void CommandLine::AddOptionalArgument(const std::wstring_view argument,
                                      const std::wstring_view help,
                                      const ArgumentType expected_type) {
#if defined(ENABLE_ASSERTIONS)
  assert(argument != kHelp && L"'help' is a reserved argument");
  assert(!argument.empty() && L"Empty arguments are not supported");
  assert(argument.find(L'=') == argument.npos &&
         L"Character '=' is not allowed in argument name");
#endif
  expected_arguments_.emplace(
      argument, ExpectedArgument{
                    help,
                    expected_type,
                    /* is_optional */ true,
                });
}

std::wstring CommandLine::Parse() {
  std::wstring_view current_argument;
  std::vector<std::wstring> rsp_arguments_storage;
  for (auto argument = initial_arguments_.begin();
       argument != initial_arguments_.end(); ++argument) {
    std::wstring_view arg(*argument);
    if (arg == kDashes) {
      std::copy(argument + 1, initial_arguments_.end(), std::back_inserter(unparsed_arguments_));
      break;
    }
    if (current_argument.empty()) {
      if (IsResponseFile(arg)) {
        arg.remove_prefix(1);  // Remove '@' prefix.
        rsp_arguments_storage.reserve(kMaxNumberOfArguments);
        ReadReponseFile(arg, &rsp_arguments_storage);
        continue;
      }
      if (!StartsWithDashes(arg))
        return std::wstring(L"Unknown argument: ") + std::wstring(arg);

      arg.remove_prefix(2);  // Remove dashes.
      std::wstring_view value;
      const size_t eq_char_position = arg.find('=');
      if (eq_char_position != arg.npos) {
         value = arg.substr(eq_char_position + 1);
         arg = arg.substr(0, eq_char_position);
         std::wstring error = TryAddExpectedArgument(arg, value);
         if (!error.empty())
           return error;
      } else {
        // Try to add current argument as-is, it may succeed if doesn't
        // require value.
        std::wstring error = TryAddExpectedArgument(arg, L"");
        if (!error.empty()) {
          // On error remember this argument and look for value for it a
          // little bit further.
          current_argument = arg;
        }
      }
    } else {
      std::wstring error = TryAddExpectedArgument(current_argument, arg);
      if (!error.empty())
        return error;

      current_argument = std::wstring_view();
    }
  }

  return CheckRequiredArguments();
}

bool CommandLine::ShouldShowUsage() const {
  return GetValue<bool>(kHelp).value_or(false);
}

void CommandLine::ShowUsage(std::wostream& output_stream) const {
  std::wstringstream usage;
  usage << L"Usage: \n" << executable_;
  for (const auto& [name, argument] : expected_arguments_) {
    if (name == kHelp)
      continue;
    std::wstring format(kDashes);
    format += name;
    if (argument.type == ArgumentType::kInt) {
      format += L"(=<int>| <int>)";
    } else if (argument.type == ArgumentType::kString) {
      format += L"(=<string>| <string>)";
    }

    if (argument.is_optional) {
      format.insert(0, 1, L'[');
      format.insert(format.size(), 1, L']');
    }
    usage << L' ' << format;
  }
  usage << L"\n\nArguments description:\n";

  for (const auto& [name, argument] : expected_arguments_) {
    usage << L"  " << name << L": " << argument.help << L'\n';
  }
  output_stream << usage.str();
}

bool CommandLine::IsSpecified(const std::wstring_view argument) const {
  return arguments_.find(argument) != arguments_.end();
}

std::wstring CommandLine::TryAddExpectedArgument(
    const std::wstring_view argument,
    const std::wstring_view value) {
  const auto expected_argument = expected_arguments_.find(argument);
  if (expected_argument == expected_arguments_.end())
    return std::wstring(L"Unknown argument: ") + std::wstring(argument);

  if (expected_argument->second.type == ArgumentType::kString) {
    if (value.empty()) {
      return std::wstring(L"Argument '") + std::wstring(argument) +
      std::wstring(L"' is expected to have non-empty string value");
    }

    arguments_.insert(std::make_pair(expected_argument->first, Argument(std::wstring(value))));
  } else if (expected_argument->second.type == ArgumentType::kInt) {
    try {
      const std::int64_t integer_value = std::stol(std::wstring(value), nullptr, 0);
      arguments_.insert(std::make_pair(expected_argument->first, Argument(integer_value)));
    } catch (const std::logic_error&) {
      return std::wstring(L"Unable to convert value of argument '") + std::wstring(argument) +
             std::wstring(L"' to integer");
    }
  } else if (expected_argument->second.type == ArgumentType::kBool) {
    if (!value.empty()) {
      return std::wstring(L"Argument '") + std::wstring(argument) +
          L"' is not expected to have a value, but specifies '" +
          std::wstring(value) + std::wstring(L"'");
    }
    
    arguments_.insert(std::make_pair(expected_argument->first, Argument(true)));
  }
  return std::wstring();
}

std::wstring CommandLine::CheckRequiredArguments() const {
  std::wstring error;
  for (const auto& expected_arg : expected_arguments_) {
    if (expected_arg.second.is_optional)
      continue;

    if (arguments_.find(expected_arg.first) == arguments_.end()) {
      std::wstring error_message =
          std::wstring(L"Unable to find required argument '") +
          std::wstring(expected_arg.first) + L"'";
      if (!error.empty()) {
        error_message.insert(0, L"; ");
      }
      error += error_message;
    }
  }

  return error;
}

void CommandLine::ReadReponseFile(const std::wstring_view& filename,
                                  std::vector<std::wstring>* rsp_arguments_storage) {
  std::wfstream response_file(filename.data());
  std::wstring argument;
  while (initial_arguments_.size() < kMaxNumberOfArguments &&
         response_file >> argument) {
    rsp_arguments_storage->emplace_back(std::move(argument));
    // It's OK to place new arguments directory to |inistial_arguments_| as
    // it has a pre-allocated capacity of kMaxNumberOfArguments and such
    // additions do not invalidate iterators.
    initial_arguments_.push_back(rsp_arguments_storage->back());
  }
}

}  // namespace base
}  // namespace oven
