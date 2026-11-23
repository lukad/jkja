// Helpers shared by the repository tools: the list of tracked files, file contents,
// repository-relative paths and #include lines.
//
// Paths are always repository-relative with forward slashes, as git prints them.

#ifndef TOOLS_REPO_TEXT_H
#define TOOLS_REPO_TEXT_H

#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <istream>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace tools {

inline const std::string_view source_root = "src";

// ASCII lower case. File names in this repository are ASCII.
inline std::string lower(std::string_view text) {
	std::string result(text);
	for (char &c : result) {
		if (c >= 'A' && c <= 'Z')
			c = static_cast<char>(c - 'A' + 'a');
	}
	return result;
}

inline bool ends_with_any(std::string_view text, std::initializer_list<std::string_view> suffixes) {
	for (std::string_view suffix : suffixes) {
		if (text.ends_with(suffix))
			return true;
	}
	return false;
}

// True for C and C++ sources and headers. Expects a lower-case path.
inline bool is_source(std::string_view lower_path) {
	return ends_with_any(lower_path, {".c", ".cpp", ".h", ".hpp"});
}

// Reads the list of tracked files as "git ls-files -z" prints it. Entries may be
// separated by NUL or by newlines.
inline std::vector<std::string> read_file_list(std::istream &in) {
	std::vector<std::string> files;
	std::string entry;
	const auto flush = [&] {
		if (!entry.empty() && entry.back() == '\r')
			entry.pop_back();
		if (!entry.empty())
			files.push_back(entry);
		entry.clear();
	};
	for (std::istreambuf_iterator<char> it(in), end; it != end; ++it) {
		if (*it == '\0' || *it == '\n')
			flush();
		else
			entry.push_back(*it);
	}
	flush();
	return files;
}

inline std::optional<std::string> read_file(const std::string &path) {
	std::ifstream in(path, std::ios::binary);
	if (!in)
		return std::nullopt;
	return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

inline bool write_file(const std::string &path, std::string_view data) {
	std::ofstream out(path, std::ios::binary | std::ios::trunc);
	out.write(data.data(), static_cast<std::streamsize>(data.size()));
	return static_cast<bool>(out);
}

inline std::vector<std::string_view> split(std::string_view text, char separator) {
	std::vector<std::string_view> parts;
	std::size_t begin = 0;
	for (;;) {
		const std::size_t end = text.find(separator, begin);
		if (end == std::string_view::npos) {
			parts.push_back(text.substr(begin));
			return parts;
		}
		parts.push_back(text.substr(begin, end - begin));
		begin = end + 1;
	}
}

// The directory part of a path; empty for a file at the repository root.
inline std::string directory_of(std::string_view path) {
	const std::size_t slash = path.rfind('/');
	return slash == std::string_view::npos ? std::string() : std::string(path.substr(0, slash));
}

inline std::string_view file_name_of(std::string_view path) {
	const std::size_t slash = path.rfind('/');
	return slash == std::string_view::npos ? path : path.substr(slash + 1);
}

// Joins a directory and a relative path and resolves "." and "..". Backslashes in
// the relative path count as separators, as they do for the compilers that the
// legacy code was written for.
inline std::string resolve(std::string_view directory, std::string_view relative) {
	std::string rel(relative);
	for (char &c : rel) {
		if (c == '\\')
			c = '/';
	}
	const std::filesystem::path joined = std::filesystem::path(directory) / rel;
	return joined.lexically_normal().generic_string();
}

// The path that names "target" from a file in "directory".
inline std::string relative_to(std::string_view directory, std::string_view target) {
	if (directory.empty())
		return std::string(target);
	return std::filesystem::path(target).lexically_relative(directory).generic_string();
}

// An #include line: where the delimiters are, and what is between them.
struct Include {
	std::size_t open;  // index of the opening " or <
	std::size_t close; // index of the closing " or >
	std::string name;
	bool quoted;
};

inline std::optional<Include> parse_include(std::string_view line) {
	const auto skip_blanks = [&](std::size_t i) {
		while (i < line.size() && (line[i] == ' ' || line[i] == '\t'))
			++i;
		return i;
	};
	std::size_t i = skip_blanks(0);
	if (i >= line.size() || line[i] != '#')
		return std::nullopt;
	i = skip_blanks(i + 1);
	const std::string_view keyword = "include";
	if (line.substr(i, keyword.size()) != keyword)
		return std::nullopt;
	i = skip_blanks(i + keyword.size());
	if (i >= line.size() || (line[i] != '"' && line[i] != '<'))
		return std::nullopt;
	std::size_t j = i + 1;
	while (j < line.size() && line[j] != '"' && line[j] != '>' && line[j] != '\r')
		++j;
	if (j >= line.size() || line[j] == '\r' || j == i + 1)
		return std::nullopt;
	return Include{i, j, std::string(line.substr(i + 1, j - i - 1)), line[i] == '"'};
}

} // namespace tools

#endif
