// src/arg_parser.cpp
#include "cut/arg_parser.hpp"

#include <algorithm>
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
#include "cut/list.hpp"
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

// Parses -d<char> or -d <char>. Advances idx past consumed args.
auto parse_delim(std::string_view arg, const std::span<char*>& args, int& idx)
    -> std::expected<char, std::string> {
  if (arg.size() > 2) {
    const auto delim_str = arg.substr(2);
    if (delim_str.size() != 1) {
      return std::unexpected(
          format_error("the delimiter must be a single character"));
    }
    ++idx;
    return delim_str.front();
  }
  if (!std::cmp_less(static_cast<std::size_t>(idx) + 1U, args.size())) {
    return std::unexpected(format_error("option requires an argument -- 'd'"));
  }
  const std::string_view delim_str{
      args.subspan(static_cast<std::size_t>(idx) + 1U).front()};
  if (delim_str.size() != 1) {
    return std::unexpected(
        format_error("the delimiter must be a single character"));
  }
  idx += 2;
  return delim_str.front();
}

// Parses mode + list from a mode flag. idx must already point past the mode arg.
auto parse_mode_and_list(std::string_view arg, int argc, char** argv, int& idx)
    -> std::expected<std::pair<CutMode, CutList>, std::string> {
  auto mode_result = detect_mode(arg);
  if (!mode_result) {
    return std::unexpected(mode_result.error());
  }
  auto spec_result = extract_list_spec(arg, argc, argv, idx);
  if (!spec_result) {
    return std::unexpected(spec_result.error());
  }
  auto list_result = parse_list(*spec_result);
  if (!list_result) {
    return std::unexpected(translate_list_error(list_result.error(), *mode_result));
  }
  return std::make_pair(*mode_result, *list_result);
}

auto check_help(const std::span<char*>& args) -> bool {
  return std::ranges::any_of(args.subspan(1), [](const char* arg) -> bool {
    return std::string_view{arg} == "--help";
  });
}

auto is_mode_flag(std::string_view arg) -> bool {
  return arg.starts_with("-b") || arg.starts_with("-c") || arg.starts_with("-f");
}

auto is_unknown_flag(std::string_view arg) -> bool {
  return arg.size() >= 2 && arg.front() == '-';
}

auto add_file(std::string_view file_path,
              std::unordered_set<std::string>& seen,
              std::vector<std::string>& files) -> void {
  std::string path{file_path};
  if (seen.insert(path).second) { files.push_back(std::move(path)); }
}

auto validate_final(const CutOptions& opts, bool mode_set)
    -> std::expected<void, std::string> {
  if (!mode_set) {
    return std::unexpected(format_error(
        "you must specify a list of bytes, characters, or fields"));
  }
  if (opts.suppress && opts.mode != CutMode::FIELD) {
    return std::unexpected(format_error(
        "suppressing non-delimited lines makes sense\n"
        "\tonly when operating on fields"));
  }
  return {};
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

  if (check_help(args)) {
    print_help();
    ParseResult help_result;
    help_result.help_requested = true;
    return help_result;
  }

  if (args.size() < 2) {
    return std::unexpected(
        format_error("you must specify a list of bytes, characters, or fields"));
  }

  ParseResult result;
  bool mode_set = false;
  bool stop_flags = false;
  std::vector<std::string> raw_files;
  std::unordered_set<std::string> seen;

  int idx = 1;
  while (std::cmp_less(idx, args.size())) {
    const std::string_view arg{
        args.subspan(static_cast<std::size_t>(idx)).front()};

    if (arg == "--") { stop_flags = true; ++idx; continue; }
    if (stop_flags)  { add_file(arg, seen, raw_files); ++idx; continue; }

    if (is_mode_flag(arg)) {
      ++idx;
      auto mode_list = parse_mode_and_list(arg, argc, argv, idx);
      if (!mode_list) { return std::unexpected(mode_list.error()); }
      result.opts.mode = mode_list->first;
      result.opts.list = mode_list->second;
      mode_set = true;
      continue;
    }

    if (arg == "-n") { result.opts.no_split = true;  ++idx; continue; }
    if (arg == "-s") { result.opts.suppress = true;   ++idx; continue; }

    if (arg.starts_with("-d")) {
      auto delim = parse_delim(arg, args, idx);
      if (!delim) { return std::unexpected(delim.error()); }
      result.opts.delim = *delim;
      continue;
    }

    if (is_unknown_flag(arg)) {
      return std::unexpected(format_error(
          std::format("invalid option -- '{}'", arg.substr(1).front())));
    }

    add_file(arg, seen, raw_files);
    ++idx;
  }

  if (auto val = validate_final(result.opts, mode_set); !val) {
    return std::unexpected(val.error());
  }

  result.files = std::move(raw_files);
  return result;
}

}  // namespace cc_cut
