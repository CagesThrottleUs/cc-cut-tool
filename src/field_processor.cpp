// src/field_processor.cpp
#include "cut/field_processor.hpp"

#include <algorithm>
#include <cstddef>
#include <ios>
#include <ostream>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "cut/config.hpp"
#include "cut/list.hpp"
#include "cut/make_file_source.hpp"
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

  int max_pos = -1;
  if (!list.indices.empty()) {
    max_pos = *list.indices.rbegin();
  }
  if (list.open_from.has_value()) {
    max_pos = std::max(max_pos, static_cast<int>(fields.size()) - 1);
  }
  if (max_pos < 0) {
    return {};
  }

  const int open_start = list.open_from.value_or(max_pos + 1);
  const auto fspan = std::span<const std::string_view>{fields};

  std::vector<std::string_view> result;
  for (int pos = 0; pos <= max_pos; ++pos) {
    if (!list.indices.contains(pos) && pos < open_start) {
      continue;
    }
    const auto idx = static_cast<std::size_t>(pos);
    result.push_back(idx < fspan.size() ? fspan.subspan(idx).front()
                                        : std::string_view{});
  }
  return result;
}

void FieldProcessor::process_line(std::string_view line, std::ostream& out) {
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

  const char join_char = opts_.delim.value_or(' ');
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

auto FieldProcessor::run(std::ostream& out,
                         const std::vector<std::string>& files,
                         std::ostream& err) -> int {
  int exit_code = 0;

  const auto process_source = [&](const std::string& path) -> void {
    auto source_result = make_file_source(path);
    if (!source_result) {
      err << source_result.error() << '\n';
      exit_code = 1;
      return;
    }
    try {
      (*source_result)->load();
    } catch (const std::ios_base::failure& ex) {
      err << config::program_name << ": " << path << ": " << ex.what() << '\n';
      exit_code = 1;
      return;
    }
    while (auto line = (*source_result)->getline()) {
      process_line(*line, out);
    }
  };

  if (files.empty()) {
    process_source("-");
  } else {
    for (const auto& path : files) {
      process_source(path);
    }
  }

  return exit_code;
}

}  // namespace cc_cut
