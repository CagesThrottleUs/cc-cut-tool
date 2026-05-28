// src/arg_parser.cpp
#include "cut/arg_parser.hpp"

#include <expected>
#include <format>
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

#include "cut/config.hpp"
#include "cut/list_parser.hpp"
#include "cut/mode.hpp"
#include "cut/options.hpp"
#include "cut/parse_result.hpp"

namespace cc_cut {

namespace {

auto format_error(std::string_view msg) -> std::string {
  return std::format("{}: {}\n{}", config::program_name, msg,
                     config::help_hint);
}

auto translate_list_error(std::string_view list_err, CutMode mode)
    -> std::string {
  if (list_err == "values may not include zero") {
    if (mode == CutMode::FIELD) {
      return format_error("fields are numbered from 1");
    }
    return format_error("byte/character positions are numbered from 1");
  }
  return format_error(list_err);
}

auto print_help() -> void {
  std::cout << std::format(
      "Usage: {0} -b list [-n] [file ...]\n"
      "       {0} -c list [file ...]\n"
      "       {0} -f list [-d delim] [-s] [file ...]\n\n"
      "  -b list  Cut by byte positions\n"
      "  -c list  Cut by character positions (UTF-8)\n"
      "  -f list  Cut by fields\n"
      "  -d delim Field delimiter (default: tab)\n"
      "  -n       Do not split multi-byte characters (byte mode)\n"
      "  -s       Suppress lines with no delimiter (field mode)\n"
      "  --help   Show this help\n",
      config::program_name);
}

}  // anonymous namespace

// NOLINTNEXTLINE(misc-use-internal-linkage)
auto detect_mode(std::string_view flag) -> std::expected<CutMode, std::string> {
  if (flag.size() >= 2 && flag.front() == '-') {
    const char opt = flag[1];  // NOLINT(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
    switch (opt) {
      case 'b':
        return CutMode::BYTE;
      case 'c':
        return CutMode::CHARACTER;
      case 'f':
        return CutMode::FIELD;
      default:
        return std::unexpected(
            format_error(std::format("invalid option -- '{}'", opt)));
    }
  }
  return std::unexpected(format_error("invalid option"));
}

// NOLINTNEXTLINE(misc-use-internal-linkage)
auto extract_list_spec(std::string_view flag, int argc, char** argv, int& index)
    -> std::expected<std::string_view, std::string> {
  if (flag.size() > 2) {
    return flag.substr(2);
  }
  if (index >= argc) {
    const char opt = flag[1];  // NOLINT(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
    return std::unexpected(
        format_error(std::format("option requires an argument -- '{}'", opt)));
  }
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic,cppcoreguidelines-pro-bounds-constant-array-index)
  return std::string_view{argv[index++]};
}

// NOLINTNEXTLINE(misc-use-internal-linkage)
auto parse_mode_properties(int argc, char** argv, int& index, CutOptions& opts)
    -> std::expected<void, std::string> {
  while (index < argc) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic,cppcoreguidelines-pro-bounds-constant-array-index)
    const std::string_view arg{argv[index]};

    if (opts.mode == CutMode::BYTE && arg == "-n") {
      opts.no_split = true;
      ++index;
    } else if (opts.mode == CutMode::FIELD && arg == "-s") {
      opts.suppress = true;
      ++index;
    } else if (opts.mode == CutMode::FIELD && arg.size() >= 2 &&
               arg.front() == '-' &&
               arg[1] == 'd') {  // NOLINT(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
      if (arg.size() > 2) {
        auto delim_str = arg.substr(2);
        if (delim_str.size() != 1) {
          return std::unexpected(
              format_error("the delimiter must be a single character"));
        }
        opts.delim = delim_str.front();
        ++index;
      } else {
        if (index + 1 >= argc) {
          return std::unexpected(
              format_error("option requires an argument -- 'd'"));
        }
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic,cppcoreguidelines-pro-bounds-constant-array-index)
        const std::string_view delim_str{argv[index + 1]};
        if (delim_str.size() != 1) {
          return std::unexpected(
              format_error("the delimiter must be a single character"));
        }
        opts.delim = delim_str.front();
        index += 2;
      }
    } else {
      break;
    }
  }
  return {};
}

// NOLINTNEXTLINE(misc-use-internal-linkage)
auto collect_files(int argc, char** argv, int index)
    -> std::vector<std::string> {
  std::vector<std::string> files;
  std::unordered_set<std::string> seen;
  for (int i = index; i < argc; ++i) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic,cppcoreguidelines-pro-bounds-constant-array-index)
    std::string path{argv[i]};
    if (seen.insert(path).second) {
      files.push_back(std::move(path));
    }
  }
  return files;
}

// NOLINTNEXTLINE(misc-use-internal-linkage)
auto parse_args(int argc, char** argv)
    -> std::expected<ParseResult, std::string> {
  // Check --help before anything else
  for (int i = 1; i < argc; ++i) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic,cppcoreguidelines-pro-bounds-constant-array-index)
    if (std::string_view{argv[i]} == "--help") {
      print_help();
      ParseResult help_result;
      help_result.help_requested = true;
      return help_result;
    }
  }

  static constexpr std::string_view no_mode_err =
      "you must specify a list of bytes, characters, or fields";

  if (argc < 2) {
    return std::unexpected(format_error(no_mode_err));
  }

  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic,cppcoreguidelines-pro-bounds-constant-array-index)
  const std::string_view first{argv[1]};

  // Reject non-flags and "--" before detect_mode
  if (first.size() < 2 || first.front() != '-' || first == "--") {
    return std::unexpected(format_error(no_mode_err));
  }

  // Unknown flag (-x) propagates detect_mode's error; no masking
  auto mode_result = detect_mode(first);
  if (!mode_result) {
    return std::unexpected(mode_result.error());
  }

  ParseResult result;
  result.opts.mode = *mode_result;

  int index = 2;

  auto spec_result = extract_list_spec(first, argc, argv, index);
  if (!spec_result) {
    return std::unexpected(spec_result.error());
  }

  auto list_result = parse_list(*spec_result);
  if (!list_result) {
    return std::unexpected(
        translate_list_error(list_result.error(), *mode_result));
  }
  result.opts.list = *list_result;

  auto props_result = parse_mode_properties(argc, argv, index, result.opts);
  if (!props_result) {
    return std::unexpected(props_result.error());
  }

  result.files = collect_files(argc, argv, index);

  return result;
}

}  // namespace cc_cut
