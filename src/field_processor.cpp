// src/field_processor.cpp
#include "cut/field_processor.hpp"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <ostream>
#include <string_view>
#include <utility>
#include <vector>

#include "cut/config.hpp"
#include "cut/list.hpp"
#include "cut/options.hpp"

namespace cc_cut {

FieldProcessor::FieldProcessor(CutOptions opts) : opts_(std::move(opts)) {}

auto FieldProcessor::split_fields(std::string_view line, char delim)
    -> std::vector<std::string_view> {
  std::vector<std::string_view> fields;
  std::size_t start = 0;
  while (true) {
    const auto pos = line.find(delim, start);
    if (pos == std::string_view::npos) {
      fields.push_back(line.substr(start));
      break;
    }
    fields.push_back(line.substr(start, pos - start));
    start = pos + 1;
  }
  return fields;
}

auto FieldProcessor::split_fields(std::string_view line)
    -> std::vector<std::string_view> {
  std::vector<std::string_view> fields;
  std::size_t pos = 0;
  while (pos < line.size()) {
    const auto first = line.find_first_not_of(" \t", pos);
    if (first == std::string_view::npos) {
      break;
    }
    const auto end = line.find_first_of(" \t", first);
    if (end == std::string_view::npos) {
      fields.push_back(line.substr(first));
      break;
    }
    fields.push_back(line.substr(first, end - first));
    pos = end;
  }
  return fields;
}

auto FieldProcessor::select_fields(const std::vector<std::string_view>& fields,
                                   const CutList& list)
    -> std::vector<std::string_view> {
  if (list.indices.empty() && !list.open_from.has_value()) {
    return {};
  }

  const std::size_t num_fields = fields.size();
  const std::size_t open_start =
      list.open_from.value_or(std::numeric_limits<std::size_t>::max());
  const std::size_t indices_max =
      list.indices.empty() ? 0 : *list.indices.rbegin();
  const std::size_t max_pos =
      list.open_from.has_value()
          ? std::max(indices_max,
                     num_fields > 0 ? num_fields - 1 : std::size_t{0})
          : indices_max;

  std::vector<std::string_view> result;
  result.reserve(num_fields);
  for (std::size_t pos = 0; pos <= max_pos; ++pos) {
    if (!list.indices.contains(pos) && pos < open_start) {
      continue;
    }
    if (pos < num_fields) {
      result.push_back(fields.at(pos));
    }
  }
  return result;
}

void FieldProcessor::process_line(std::string_view line,
                                  std::ostream& out) const {
  const bool has_delim =
      opts_.delim.has_value()
          ? line.contains(opts_.delim.value())
          : line.find_first_of(" \t") != std::string_view::npos;

  if (!has_delim) {
    if (!opts_.suppress) {
      out << line << '\n';
    }
    return;
  }

  const auto fields = opts_.delim.has_value()
                          ? split_fields(line, opts_.delim.value())
                          : split_fields(line);
  const auto selected = select_fields(fields, opts_.list);

  const char join_char = opts_.delim.value_or(config::default_field_delim);
  bool first = true;
  for (const auto& field : selected) {
    if (!first) {
      out << join_char;
    }
    out << field;
    first = false;
  }
  out << '\n';
}

}  // namespace cc_cut
