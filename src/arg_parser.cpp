// src/arg_parser.cpp
#include "cut/arg_parser.hpp"

#include <cstddef>
#include <expected>
#include <format>
#include <iostream>
#include <span>
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

// spec_id: SPEC-3  req_id: REQ-008
auto format_error(std::string_view msg) -> std::string {
  return std::format("{}: {}\n{}", config::program_name, msg,
                     config::help_hint);
}

// spec_id: SPEC-3  req_id: REQ-008
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

// spec_id: SPEC-3  req_id: REQ-010
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

auto detect_mode(std::string_view flag) -> std::expected<CutMode, std::string> {
  if (flag.starts_with("-b")) {
    return CutMode::BYTE;
  }
  if (flag.starts_with("-c")) {
    return CutMode::CHARACTER;
  }
  if (flag.starts_with("-f")) {
    return CutMode::FIELD;
  }
  if (flag.size() >= 2 && flag.front() == '-') {
    return std::unexpected(format_error(
        std::format("invalid option -- '{}'", flag.substr(1).front())));
  }
  return std::unexpected(format_error("invalid option"));
}

auto extract_list_spec(std::string_view flag, int argc, char** argv, int& index)
    -> std::expected<std::string_view, std::string> {
  if (flag.size() > 2) {
    return flag.substr(2);
  }
  if (index >= argc) {
    const char opt = flag.substr(1).front();
    return std::unexpected(
        format_error(std::format("option requires an argument -- '{}'", opt)));
  }
  const auto args = std::span<char*>{argv, static_cast<std::size_t>(argc)};
  const std::string_view result{
      args.subspan(static_cast<std::size_t>(index)).front()};
  ++index;
  return result;
}

auto parse_mode_properties(int argc, char** argv, int& index, CutOptions& opts)
    -> std::expected<void, std::string> {
  const auto args = std::span<char*>{argv, static_cast<std::size_t>(argc)};
  while (std::cmp_less(index, args.size())) {
    const std::string_view arg{
        args.subspan(static_cast<std::size_t>(index)).front()};

    if (opts.mode == CutMode::BYTE && arg == "-n") {
      opts.no_split = true;
      ++index;
    } else if (opts.mode == CutMode::FIELD && arg == "-s") {
      opts.suppress = true;
      ++index;
    } else if (opts.mode == CutMode::FIELD && arg.starts_with("-d")) {
      if (arg.size() > 2) {
        auto delim_str = arg.substr(2);
        if (delim_str.size() != 1) {
          return std::unexpected(
              format_error("the delimiter must be a single character"));
        }
        opts.delim = delim_str.front();
        ++index;
      } else {
        if (!std::cmp_less(static_cast<std::size_t>(index) + 1U, args.size())) {
          return std::unexpected(
              format_error("option requires an argument -- 'd'"));
        }
        const std::string_view delim_str{
            args.subspan(static_cast<std::size_t>(index) + 1U).front()};
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

auto collect_files(int argc, char** argv, int index)
    -> std::vector<std::string> {
  std::vector<std::string> files;
  std::unordered_set<std::string> seen;
  const auto args = std::span<char*>{argv, static_cast<std::size_t>(argc)};
  for (const auto& arg : args.subspan(static_cast<std::size_t>(index))) {
    std::string path{arg};
    if (seen.insert(path).second) {
      files.push_back(std::move(path));
    }
  }
  return files;
}

auto parse_args(int argc, char** argv)
    -> std::expected<ParseResult, std::string> {
  const auto args = std::span<char*>{argv, static_cast<std::size_t>(argc)};

  // Check --help before anything else (subspan(1) is empty when argc==1)
  for (const auto& arg : args.subspan(1)) {
    if (std::string_view{arg} == "--help") {
      print_help();
      ParseResult help_result;
      help_result.help_requested = true;
      return help_result;
    }
  }

  static constexpr std::string_view no_mode_err =
      "you must specify a list of bytes, characters, or fields";

  if (args.size() < 2) {
    return std::unexpected(format_error(no_mode_err));
  }

  const std::string_view first{args.subspan(1).front()};

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
