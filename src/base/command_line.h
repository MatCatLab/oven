#ifndef _OVEN_BASE_COMMAND_LINE_H_
#define _OVEN_BASE_COMMAND_LINE_H_

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

namespace oven {
namespace base {

class CommandLine {
 public:
  enum Limits {
    kMaxNumberOfArguments = 1024,
  };
  enum class ArgumentType {
    kString,
    kInt,
    //kStringList,
    //kIntList,
    kBool,  /* Argument turns true if specified, false otherwise */
  };
  CommandLine(int argc, wchar_t** argv);

  void AddArgument(const std::wstring_view argument,
                   const std::wstring_view help,
                   const ArgumentType expected_type);
  void AddOptionalArgument(const std::wstring_view argument,
                           const std::wstring_view help,
                           const ArgumentType expected_type);

  std::wstring Parse();
  const std::vector<std::wstring_view>& GetUnparsed() const {
    return unparsed_arguments_;
  }
  bool ShouldShowUsage() const;
  void ShowUsage(std::wostream& output_stream) const;

  bool IsSpecified(const std::wstring_view argument) const;

  template <typename ValueType>
  std::optional<ValueType> GetValue(const std::wstring_view argument) const {
    const auto value_it = arguments_.find(argument);
    if (value_it != arguments_.end() &&
        std::holds_alternative<ValueType>(value_it->second)) {
      return std::get<ValueType>(value_it->second);
    }
    return {};
  }

  template <typename ValueType>
  ValueType GetValue(const std::wstring_view argument,
                     const ValueType& default_value) const {
    return GetValue<ValueType>(argument).value_or(default_value);
  }
  
 private:
  struct ExpectedArgument {
    const std::wstring_view help;
    const ArgumentType type;
    const bool is_optional;
  };

  using Argument = std::
      variant<std::wstring, std::int64_t, /*std::vector<std::wstring>, std::vector<int>,*/ bool>;

  std::wstring TryAddExpectedArgument(const std::wstring_view argument,
                                      const std::wstring_view value);

  std::wstring CheckRequiredArguments() const;

  // As CommandLine contains only string views, we use a separate storage to keep that views valid.
  void ReadReponseFile(const std::wstring_view& filename,
                       std::vector<std::wstring>* rsp_arguments_storage);

  std::wstring_view executable_;
  std::vector<std::wstring_view> initial_arguments_;
  std::vector<std::wstring_view> unparsed_arguments_;
  std::unordered_map<std::wstring_view, ExpectedArgument> expected_arguments_;
  std::unordered_map<std::wstring_view, Argument> arguments_;
};

}  // namespace base
}  // namespace oven

#endif  // _OVEN_BASE_COMMAND_LINE_H_
