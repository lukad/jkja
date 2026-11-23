// fix-includes: rewrites #include lines to the repository's include convention.
//
//   git ls-files -z | fix-includes [--apply]
//
// Run from the repository root; the list of tracked files is read from standard
// input. Without --apply it prints what would change; with --apply it changes it.
// Run it again after moving or renaming source files.
//
// Convention: a header that lives in this repository is included in quotes, by its
// path relative to the including file, with forward slashes and the exact letter
// case of the file on disk. No include directories are needed to find it.
//
// For every #include in the C and C++ sources under src/, the named header is
// looked up case-insensitively: relative to the including file, then relative to
// src/, then by file name alone if exactly one tracked file under src/ has that
// name. If it is found, the line is rewritten. Headers that are not in the
// repository (system and third-party headers) are left alone.
//
// Exit status: 0 on success, 1 if an include could not be decided because several
// files have its name, 2 on a usage or input error.

#include "repo_text.h"

#include <iostream>
#include <map>

int main(int argc, char **argv) {
	bool apply = false;
	for (int i = 1; i < argc; ++i) {
		if (std::string_view(argv[i]) != "--apply") {
			std::cerr << "usage: git ls-files -z | fix-includes [--apply]\n";
			return 2;
		}
		apply = true;
	}

	std::vector<std::string> tracked;
	for (std::string &f : tools::read_file_list(std::cin)) {
		if (f.starts_with(std::string(tools::source_root) + "/"))
			tracked.push_back(std::move(f));
	}
	if (tracked.empty()) {
		std::cerr << "fix-includes: no files under src/ on standard input\n";
		return 2;
	}

	std::map<std::string, std::string> by_lower;
	std::map<std::string, std::vector<std::string>> by_name;
	for (const std::string &f : tracked) {
		by_lower.emplace(tools::lower(f), f);
		by_name[tools::lower(tools::file_name_of(f))].push_back(f);
	}

	std::vector<std::string> ambiguous;
	const auto find = [&](const std::string &includer,
	                      const std::string &name) -> const std::string * {
		for (const std::string &base :
		     {tools::directory_of(includer), std::string(tools::source_root)}) {
			const auto found = by_lower.find(tools::lower(tools::resolve(base, name)));
			if (found != by_lower.end())
				return &found->second;
		}
		const std::string plain_name = tools::lower(tools::file_name_of(tools::resolve("", name)));
		const auto candidates = by_name.find(plain_name);
		if (candidates == by_name.end())
			return nullptr;
		if (candidates->second.size() == 1)
			return &candidates->second.front();
		std::string note = includer + ": " + name + " could be";
		for (std::size_t i = 0; i < candidates->second.size(); ++i)
			note += (i ? " or " : " ") + candidates->second[i];
		ambiguous.push_back(note);
		return nullptr;
	};

	std::size_t rewritten = 0;
	std::size_t files_changed = 0;
	for (const std::string &f : tracked) {
		if (!tools::is_source(tools::lower(f)))
			continue;
		const std::optional<std::string> data = tools::read_file(f);
		if (!data) {
			std::cerr << "fix-includes: cannot read " << f << "\n";
			return 2;
		}
		const std::string directory = tools::directory_of(f);
		std::string result;
		result.reserve(data->size());
		bool changed = false;
		std::size_t number = 0;
		for (std::string_view line : tools::split(*data, '\n')) {
			if (number++ > 0)
				result.push_back('\n');
			const std::optional<tools::Include> include = tools::parse_include(line);
			const std::string *target = include ? find(f, include->name) : nullptr;
			if (!target) {
				result.append(line);
				continue;
			}
			const std::string replacement = tools::relative_to(directory, *target);
			if (replacement == include->name && include->quoted) {
				result.append(line);
				continue;
			}
			if (!apply)
				std::cout << f << ":" << number << ": " << include->name << " -> " << replacement
				          << "\n";
			result.append(line.substr(0, include->open));
			result.append("\"" + replacement + "\"");
			result.append(line.substr(include->close + 1));
			++rewritten;
			changed = true;
		}
		if (!changed)
			continue;
		++files_changed;
		if (apply && !tools::write_file(f, result)) {
			std::cerr << "fix-includes: cannot write " << f << "\n";
			return 2;
		}
	}

	for (const std::string &note : ambiguous)
		std::cerr << "ambiguous, left alone: " << note << "\n";
	std::cout << (apply ? "rewrote " : "would rewrite ") << rewritten << " include lines in "
	          << files_changed << " files\n";
	return ambiguous.empty() ? 0 : 1;
}
