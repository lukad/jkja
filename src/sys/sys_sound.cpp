// The sound device. The mixer paints into a buffer that the device plays in a loop, as
// it did with the looping DirectSound buffer of the release: SDL asks for sound when it
// needs some, and gets the next stretch of the buffer.

#include "sys_local.h"

#include "../client/snd_local.h"

#include <SDL3/SDL.h>

#include <array>
#include <atomic>

namespace {

constexpr int channelCount = 2;
constexpr int bitsPerSample = 16;
constexpr int bytesPerSample = bitsPerSample / 8;

// The size of the release's DirectSound buffer. The mixer wraps positions with a mask,
// so the number of samples in it is a power of two.
constexpr int bufferBytes = 0x10000;

std::array<byte, bufferBytes> mixBuffer;

SDL_AudioStream *stream;

// Where in mixBuffer the device will take its next sound from, in bytes. Written by
// SDL's audio thread, read by the mixer.
std::atomic<int> playPosition;

// What the device plays instead while the game is not active.
const std::array<byte, bufferBytes> silence{};
std::atomic<bool> silenced;

// True when no device could be opened and the null device stands in.
bool usingNullDevice;

void SDLCALL FeedDevice(void *, SDL_AudioStream *audioStream, int additionalAmount, int) {
	int position = playPosition.load();
	const byte *source = silenced.load() ? silence.data() : mixBuffer.data();

	while (additionalAmount > 0) {
		const int chunk = SDL_min(additionalAmount, bufferBytes - position);
		SDL_PutAudioStreamData(audioStream, source + position, chunk);
		position = (position + chunk) % bufferBytes;
		additionalAmount -= chunk;
	}
	playPosition.store(position);
}

bool OpenDevice() {
	if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
		Com_Printf("no audio system: %s\n", SDL_GetError());
		return false;
	}

	SDL_AudioSpec format;
	format.format = SDL_AUDIO_S16;
	format.channels = channelCount;
	if (s_khz->integer == 44) {
		format.freq = 44100;
	} else if (s_khz->integer == 22) {
		format.freq = 22050;
	} else {
		format.freq = 11025;
	}

	mixBuffer.fill(0);
	playPosition.store(0);

	stream =
	    SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &format, FeedDevice, nullptr);
	if (!stream) {
		Com_Printf("could not open a sound device: %s\n", SDL_GetError());
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
		return false;
	}

	dma.channels = format.channels;
	dma.samplebits = bitsPerSample;
	dma.speed = format.freq;
	dma.samples = bufferBytes / bytesPerSample;
	dma.submission_chunk = 1;
	dma.buffer = mixBuffer.data();

	SDL_ResumeAudioStreamDevice(stream);
	return true;
}

} // namespace

/*
==================
SNDDMA_Init

Without a device, and when running headless, the null device takes the mix: a build
that is not a final build refuses maps with ambient sounds when the sound system is off.
==================
*/
qboolean SNDDMA_Init(void) {
	if (!Sys_Headless() && OpenDevice()) {
		usingNullDevice = false;
		return qtrue;
	}

	usingNullDevice = true;
	return NullSound_Init();
}

/*
==============
SNDDMA_GetDMAPos

The position of the device in the buffer, in mono samples.
===============
*/
int SNDDMA_GetDMAPos(void) {
	if (usingNullDevice) {
		return NullSound_GetDMAPos();
	}
	return playPosition.load() / bytesPerSample;
}

/*
==============
SNDDMA_Shutdown
===============
*/
void SNDDMA_Shutdown(void) {
	if (usingNullDevice) {
		NullSound_Shutdown();
		return;
	}
	if (stream) {
		SDL_DestroyAudioStream(stream);
		stream = nullptr;
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
	}
	dma.buffer = nullptr;
}

/*
==============
SNDDMA_Activate
===============
*/
void SNDDMA_Activate(bool active) {
	silenced.store(!active);
}

/*
==============
SNDDMA_BeginPainting

Keeps the device out of the buffer while the mixer writes to it.
===============
*/
void SNDDMA_BeginPainting(void) {
	if (stream) {
		SDL_LockAudioStream(stream);
	}
}

/*
==============
SNDDMA_Submit
===============
*/
void SNDDMA_Submit(void) {
	if (stream) {
		SDL_UnlockAudioStream(stream);
	}
}
