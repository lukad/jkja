// The system console's input: lines typed on standard input become console events.
// Its output is standard output (Sys_Print).

#include "sys_local.h"

#include <cstring>
#include <deque>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>

namespace {

std::mutex linesMutex;
std::deque<std::string> lines;

// Reading standard input blocks, so it has a thread of its own. The thread ends when
// standard input does and is otherwise left behind when the process exits.
void ReadStandardInput() {
	std::string line;

	while (std::getline(std::cin, line)) {
		const std::lock_guard<std::mutex> lock(linesMutex);
		lines.push_back(line);
	}
}

} // namespace

void Sys_StartConsoleInput(void) {
	std::thread(ReadStandardInput).detach();
}

void Sys_QueueConsoleInput(void) {
	const std::lock_guard<std::mutex> lock(linesMutex);

	for (const std::string &line : lines) {
		const int length = static_cast<int>(line.size()) + 1;
		char *text = static_cast<char *>(Z_Malloc(length, TAG_EVENT, qfalse));

		std::memcpy(text, line.c_str(), static_cast<std::size_t>(length));
		Sys_QueEvent(0, SE_CONSOLE, 0, 0, length, text);
	}
	lines.clear();
}
