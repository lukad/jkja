// The sound device when there is none. The mixer gets a buffer in the format the game
// was shipped with and a play position that follows the clock; what it mixes is
// discarded. Sounds are therefore loaded and mixed exactly as with a device, which is
// what a run without one is meant to exercise.

#include "sys_local.h"

#include "../client/snd_local.h"

#include <array>
#include <cstdint>

namespace {

constexpr int channelCount = 2;
constexpr int bitsPerSample = 16;
constexpr int samplesPerSecond = 22050;

// The mixer wraps positions with a mask, so the number of samples is a power of two.
constexpr int sampleCount = 0x8000;

std::array<byte, sampleCount * (bitsPerSample / 8)> mixBuffer;

} // namespace

qboolean SNDDMA_Init(void) {
	mixBuffer.fill(0);
	dma.channels = channelCount;
	dma.samplebits = bitsPerSample;
	dma.speed = samplesPerSecond;
	dma.samples = sampleCount;
	dma.submission_chunk = 1;
	dma.buffer = mixBuffer.data();
	return qtrue;
}

// The position in the buffer, in mono samples, of the sample that would be playing now.
int SNDDMA_GetDMAPos(void) {
	const std::int64_t played =
	    static_cast<std::int64_t>(Sys_Milliseconds()) * samplesPerSecond / 1000 * channelCount;
	return static_cast<int>(played & (sampleCount - 1));
}

void SNDDMA_Shutdown(void) {
	dma.buffer = nullptr;
}

void SNDDMA_BeginPainting(void) {}

void SNDDMA_Submit(void) {}
