#include "cut/list_parser.hpp"

#include <charconv>
#include <expected>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include "cut/list.hpp"

namespace cc_cut {

namespace {

auto parse_pos_int(std::string_view str) -> std::optional<int> {
  if (str.empty() || str.front() < '0' || str.front() > '9') {
    return std::nullopt;
  }
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
  const char* const str_end = str.data() + str.size();
  int parsed = 0;
  // NOLINTNEXTLINE(bugprone-suspicious-stringview-data-usage)
  const auto [ptr, ec] = std::from_chars(str.data(), str_end, parsed);
  if (ec != std::errc{} || ptr != str_end) {
    return std::nullopt;
  }
  return parsed;
}

auto tokenize(std::string_view list_arg) -> std::vector<std::string> {
  std::vector<std::string> tokens;

  if (list_arg.contains(',')) {
    std::string_view rem = list_arg;
    while (true) {
      auto comma = rem.find(',');
      const std::string_view part =
          (comma == std::string_view::npos) ? rem : rem.substr(0, comma);

      auto first = part.find_first_not_of(" \t");
      if (first == std::string_view::npos) {
        tokens.emplace_back("");
      } else {
        auto last = part.find_last_not_of(" \t");
        tokens.emplace_back(part.substr(first, last - first + 1));
      }

      if (comma == std::string_view::npos) {
        break;
      }
      rem = rem.substr(comma + 1);
    }
  } else {
    std::string_view rem = list_arg;
    while (!rem.empty()) {
      auto first = rem.find_first_not_of(" \t");
      if (first == std::string_view::npos) {
        break;
      }
      rem = rem.substr(first);
      auto end = rem.find_first_of(" \t");
      if (end == std::string_view::npos) {
        tokens.emplace_back(rem);
        break;
      }
      tokens.emplace_back(rem.substr(0, end));
      rem = rem.substr(end);
    }
  }

  return tokens;
}

// Zero-position check precedes decreasing-range check per SPEC-2 REQ-005.
auto apply_token(std::string_view token, CutList& cutlist)
    -> std::expected<void, std::string> {
  if (token.empty()) {
    return std::unexpected(std::format("invalid field value: {}", token));
  }

  auto dash = token.find('-');

  if (dash == std::string_view::npos) {
    auto num = parse_pos_int(token);
    if (!num) {
      return std::unexpected(std::format("invalid field value: {}", token));
    }
    if (*num == 0) {
      return std::unexpected("values may not include zero");
    }
    cutlist.indices.insert(*num - 1);
    return {};
  }

  if (dash == 0) {
    auto rest = token.substr(1);
    if (rest.empty()) {
      return std::unexpected("invalid range with no endpoint: -");
    }
    auto end = parse_pos_int(rest);
    if (!end) {
      return std::unexpected(std::format("invalid field value: {}", token));
    }
    if (*end == 0) {
      return std::unexpected("values may not include zero");
    }
    for (int idx = 0; idx < *end; ++idx) {
      cutlist.indices.insert(idx);
    }
    return {};
  }

  auto left = token.substr(0, dash);
  auto right = token.substr(dash + 1);

  auto left_num = parse_pos_int(left);
  if (!left_num) {
    return std::unexpected(std::format("invalid field value: {}", token));
  }
  if (*left_num == 0) {
    return std::unexpected("values may not include zero");
  }

  if (right.empty()) {
    int open = *left_num - 1;
    if (!cutlist.open_from.has_value() || open < *cutlist.open_from) {
      cutlist.open_from = open;
    }
    return {};
  }

  auto right_num = parse_pos_int(right);
  if (!right_num) {
    return std::unexpected(std::format("invalid field value: {}", token));
  }
  if (*right_num == 0) {
    return std::unexpected("values may not include zero");
  }
  if (*left_num > *right_num) {
    return std::unexpected("invalid decreasing range");
  }
  for (int idx = *left_num - 1; idx < *right_num; ++idx) {
    cutlist.indices.insert(idx);
  }
  return {};
}

}  // anonymous namespace

// external linkage required
auto parse_list(std::string_view list_arg)
    -> std::expected<CutList, std::string> {
  auto tokens = tokenize(list_arg);
  if (tokens.empty()) {
    return std::unexpected("missing list specification");
  }

  CutList result;
  for (const auto& token : tokens) {
    auto outcome = apply_token(token, result);
    if (!outcome) {
      return std::unexpected(outcome.error());
    }
  }
  return result;
}

}  // namespace cc_cut
