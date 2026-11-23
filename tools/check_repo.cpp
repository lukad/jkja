// check-repo: checks the rules every commit in this repository has to keep.
//
//   git ls-files -z | check-repo [--known-missing=<file>]
//
// Run from the repository root; the list of tracked files is read from standard
// input.
//
// 1. No forbidden files: binaries, IDE project files, version-control leftovers,
//    game data.
// 2. No machine-specific paths in tracked files: home directories and game library
//    locations (docs/decisions/0007-game-data-location.md).
// 3. Text files use LF line endings.
// 4. Directories under src/ have lower-case names.
// 5. Includes follow the convention that fix-includes establishes: a header that
//    lives in the repository is included in quotes, by its path relative to the
//    including file, with forward slashes and exact letter case. A quoted include
//    that names nothing in the repository is an error unless the file given with
//    --known-missing lists it. An entry in that file that is no longer needed is an
//    error too, so the list can only shrink.
//
// Exit status: 0 if everything holds, 1 if there are problems, 2 on a usage or
// input error.

#include "repo_text.h"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <utility>

namespace {

using tools::ends_with_any;

bool is_forbidden_type(std::string_view lower_path) {
	return ends_with_any(lower_path, {
	                                     // binaries
	                                     ".a",
	                                     ".dll",
	                                     ".dylib",
	                                     ".exe",
	                                     ".idb",
	                                     ".lib",
	                                     ".o",
	                                     ".obj",
	                                     ".pdb",
	                                     ".so",
	                                     // IDE projects and per-user IDE state
	                                     ".dsp",
	                                     ".dsw",
	                                     ".ncb",
	                                     ".opt",
	                                     ".plg",
	                                     ".sln",
	                                     ".suo",
	                                     ".vcproj",
	                                     ".vcxproj",
	                                     // version-control leftovers
	                                     ".scc",
	                                     ".vspscc",
	                                     // game data
	                                     ".pk3",
	                                 });
}

bool is_binary(std::string_view lower_path) {
	return ends_with_any(lower_path, {".bmp", ".ico", ".png"});
}

bool is_user_name_char(char c) {
	return std::isalnum(static_cast<unsigned char>(c)) || c == '.' || c == '_' || c == '-';
}

// Finds "<prefix><user name>/", for example a home directory.
std::optional<std::size_t> find_user_directory(std::string_view data, std::string_view prefix) {
	for (std::size_t at = data.find(prefix); at != std::string_view::npos;
	     at = data.find(prefix, at + 1)) {
		std::size_t i = at + prefix.size();
		const std::size_t name_begin = i;
		while (i < data.size() && is_user_name_char(data[i]))
			++i;
		if (i > name_begin && i < data.size() && data[i] == '/')
			return at;
	}
	return std::nullopt;
}

// Finds a Windows profile directory: a drive letter, a colon, slashes of either
// kind, "Users", another slash. Expects lower-case data.
std::optional<std::size_t> find_windows_profile(std::string_view lower_data) {
	const std::string_view users = "users";
	for (std::size_t colon = lower_data.find(':'); colon != std::string_view::npos;
	     colon = lower_data.find(':', colon + 1)) {
		if (colon == 0 || !std::isalpha(static_cast<unsigned char>(lower_data[colon - 1])))
			continue;
		std::size_t i = colon + 1;
		const std::size_t slashes_begin = i;
		while (i < lower_data.size() && (lower_data[i] == '\\' || lower_data[i] == '/'))
			++i;
		if (i == slashes_begin || lower_data.substr(i, users.size()) != users)
			continue;
		i += users.size();
		if (i < lower_data.size() && (lower_data[i] == '\\' || lower_data[i] == '/'))
			return colon - 1;
	}
	return std::nullopt;
}

// Paths that identify one person's machine. The literals are written in pieces so
// that this file does not contain what it looks for.
std::optional<std::size_t> find_machine_path(std::string_view data) {
	if (auto at = find_user_directory(data, "/home/"))
		return at;
	if (auto at = find_user_directory(data, "/Users/"))
		return at;
	const std::string lower_data = tools::lower(data);
	if (auto at = find_windows_profile(lower_data))
		return at;
	for (const std::string &needle :
	     {std::string("steam") + "apps", std::string(".local/share/") + "steam"}) {
		const std::size_t at = lower_data.find(needle);
		if (at != std::string::npos)
			return at;
	}
	return std::nullopt;
}

// Blanks out comments, keeping line structure and string literals.
std::string strip_comments(std::string_view text) {
	std::string out;
	out.reserve(text.size());
	std::size_t i = 0;
	const std::size_t n = text.size();
	while (i < n) {
		const char c = text[i];
		if (c == '/' && i + 1 < n && text[i + 1] == '/') {
			while (i < n && text[i] != '\n')
				++i;
		} else if (c == '/' && i + 1 < n && text[i + 1] == '*') {
			std::size_t end = text.find("*/", i + 2);
			end = end == std::string_view::npos ? n : end + 2;
			for (; i < end; ++i)
				out.push_back(text[i] == '\n' ? '\n' : ' ');
		} else if (c == '"' || c == '\'') {
			std::size_t j = i + 1;
			while (j < n && text[j] != c && text[j] != '\n')
				j += text[j] == '\\' ? 2 : 1;
			const std::size_t end = std::min(j + 1, n);
			out.append(text.substr(i, end - i));
			i = end;
		} else {
			out.push_back(c);
			++i;
		}
	}
	return out;
}

using KnownMissing = std::set<std::pair<std::string, std::string>>;

// Each line: <including file> <include name> [# reason]
std::optional<KnownMissing> read_known_missing(const std::string &path) {
	const std::optional<std::string> data = tools::read_file(path);
	if (!data)
		return std::nullopt;
	KnownMissing entries;
	for (std::string_view line : tools::split(*data, '\n')) {
		const std::size_t hash = line.find('#');
		std::istringstream fields{std::string(line.substr(0, hash))};
		std::string includer, name;
		if (fields >> includer >> name)
			entries.emplace(includer, name);
	}
	return entries;
}

} // namespace

int main(int argc, char **argv) {
	KnownMissing known_missing;
	for (int i = 1; i < argc; ++i) {
		const std::string_view arg = argv[i];
		const std::string_view option = "--known-missing=";
		if (!arg.starts_with(option)) {
			std::cerr << "usage: git ls-files -z | check-repo [--known-missing=<file>]\n";
			return 2;
		}
		const std::string path(arg.substr(option.size()));
		std::optional<KnownMissing> entries = read_known_missing(path);
		if (!entries) {
			std::cerr << "check-repo: cannot read " << path << "\n";
			return 2;
		}
		known_missing = std::move(*entries);
	}

	const std::vector<std::string> tracked = tools::read_file_list(std::cin);
	if (tracked.empty()) {
		std::cerr << "check-repo: no file list on standard input\n";
		return 2;
	}
	const std::set<std::string> tracked_set(tracked.begin(), tracked.end());
	std::map<std::string, std::string> by_lower;
	for (const std::string &f : tracked)
		by_lower.emplace(tools::lower(f), f);

	std::vector<std::string> problems;
	const auto problem = [&](const std::string &path, const std::string &message,
	                         std::size_t line = 0) {
		problems.push_back(path + (line ? ":" + std::to_string(line) : "") + ": " + message);
	};
	KnownMissing used_known_missing;

	for (const std::string &f : tracked) {
		const std::string lower_path = tools::lower(f);
		const std::vector<std::string_view> parts = tools::split(f, '/');
		const bool under_source_root = parts.size() > 1 && parts.front() == tools::source_root;

		if (is_forbidden_type(lower_path))
			problem(f, "forbidden file type");
		for (std::size_t i = 0; i + 1 < parts.size(); ++i) {
			if (parts[i] == "CVS" || parts[i] == ".svn") {
				problem(f, "version-control leftover");
				break;
			}
		}
		if (under_source_root && tools::directory_of(f) != tools::directory_of(lower_path))
			problem(f, "directories under src/ must have lower-case names");
		if (is_binary(lower_path))
			continue;

		const std::optional<std::string> data = tools::read_file(f);
		if (!data) {
			problem(f, "cannot be read");
			continue;
		}
		if (data->find('\r') != std::string::npos)
			problem(f, "contains CR characters; text files use LF line endings");
		if (const auto at = find_machine_path(*data)) {
			const auto line =
			    std::count(data->begin(), data->begin() + static_cast<std::ptrdiff_t>(*at), '\n') +
			    1;
			problem(f, "contains a machine-specific path", static_cast<std::size_t>(line));
		}

		if (!under_source_root || !tools::is_source(lower_path))
			continue;

		const std::string directory = tools::directory_of(f);
		const std::string text = strip_comments(*data);
		std::size_t number = 0;
		for (std::string_view line : tools::split(text, '\n')) {
			++number;
			const std::optional<tools::Include> include = tools::parse_include(line);
			if (!include)
				continue;
			const std::string &name = include->name;
			if (name.find('\\') != std::string::npos) {
				problem(f, "include uses backslashes: " + name, number);
				continue;
			}
			const std::string target = tools::resolve(directory, name);
			if (tracked_set.contains(target)) {
				const std::string expected = tools::relative_to(directory, target);
				if (!include->quoted)
					problem(f, "header from this repository in angle brackets: " + name, number);
				else if (name != expected)
					problem(f, "include should be written \"" + expected + "\": " + name, number);
			} else if (by_lower.contains(tools::lower(target))) {
				problem(f, "include has the wrong letter case: " + name, number);
			} else if (include->quoted) {
				if (known_missing.contains({f, name}))
					used_known_missing.emplace(f, name);
				else
					problem(f, "quoted include names nothing in the repository: " + name, number);
			}
		}
	}

	for (const auto &entry : known_missing) {
		if (!used_known_missing.contains(entry))
			problem(entry.first, "known-missing entry is no longer needed: " + entry.second);
	}

	for (const std::string &line : problems)
		std::cout << line << "\n";
	if (!problems.empty()) {
		std::cerr << problems.size() << " problem(s)\n";
		return 1;
	}
	std::cout << "repository checks passed (" << tracked.size() << " files)\n";
	return 0;
}
