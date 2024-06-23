#include "core/runtime.h"
#include "core/tasks.h"
#include "core/strings.h"
#include "core/file.h"
#include "core/modules.h"

#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <SDL.h>
#include <SDL_audio.h>

#define API_URL "https://api.openai.com/v1/audio/speech"
#define MAX_TEXT_SIZE 2048

struct MemoryStruct {
	char* memory;
	size_t size;
};

private void get_tts_audio(string text, struct MemoryStruct* chunk);

static size_t WriteMemoryCallback(void* contents, size_t size, size_t nmemb, void* userp) {
	size_t realsize = size * nmemb;
	struct MemoryStruct* mem = (struct MemoryStruct*)userp;

	char* ptr = realloc(mem->memory, mem->size + realsize + 1);
	if (ptr == NULL) {
		printf("Not enough memory (realloc returned NULL)\n");
		return 0;
	}

	mem->memory = ptr;
	memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;

	return realsize;
}

SDL_AudioDeviceID deviceId = 0;

void play_audio_from_memory(const Uint8* wavBuffer, Uint32 wavLength) {

	SDL_AudioSpec wavSpec;
	SDL_zero(wavSpec);
	SDL_AudioSpec obtainedSpec;

	SDL_RWops* rw = SDL_RWFromConstMem(wavBuffer, wavLength);
	if (!rw) {
		fprintf(stderr, "Could not create SDL_RWops: %s\n", SDL_GetError());
		return;
	}

	Uint8* audio_buf;
	Uint32 audio_len;
	if (SDL_LoadWAV_RW(rw, 1, &wavSpec, &audio_buf, &audio_len) == NULL) {
		fprintf(stderr, "Could not load WAV: %s\n", SDL_GetError());
		return;
	}

	if (deviceId is 0)
	{
		deviceId = SDL_OpenAudioDevice(NULL, 0, &wavSpec, &obtainedSpec, 0);
	}
	if (deviceId == 0) {
		fprintf(stderr, "Could not open audio device: %s\n", SDL_GetError());
		SDL_FreeWAV(audio_buf);
		return;
	}

	if (SDL_QueueAudio(deviceId, audio_buf, audio_len) < 0) {
		fprintf(stderr, "Could not queue audio data: %s\n", SDL_GetError());
		SDL_CloseAudioDevice(deviceId);
		SDL_FreeWAV(audio_buf);
		return;
	}

	SDL_PauseAudioDevice(deviceId, 0);

	SDL_Delay((audio_len / obtainedSpec.freq) * 1000);

	SDL_FreeWAV(audio_buf);
}

const char* POST_HEADER = "\"model\":\"tts-1\",\"voice\":\"echo\",\"response_format\":\"wav\"";

const char* api_key = NULL;
array(string) GLOBAL_TTSQueue;
locker GLOBAL_TTSQueueLocker;

public void QueueTTS(string text)
{
	if (text->StackObject)
	{
		fprintf_red(stderr, "Strings passed to QueueTTS must be dynamic as they are freed after they are played. Use strings.Clone(stackString) or dynamic_string('cString')%s", "\n");
		throw(StackObjectModifiedException);
	}

	lock(GLOBAL_TTSQueueLocker, {
		arrays(string).Append(GLOBAL_TTSQueue, text);
		fprintf(stdout, "Queued TTS [%i]: %s\n",Tasks.ThreadId(), text->Values);
		});

}

OnStart(1)
{
	api_key = getenv("OPENAI_API_KEY");
	if (!api_key) {
		fprintf(stderr, "Environment variable OPENAI_API_KEY not set.\n");
		exit(1);
	}

	curl_global_init(CURL_GLOBAL_DEFAULT);
	SDL_Init(SDL_INIT_AUDIO);
	GLOBAL_TTSQueue = dynamic_array(string, 1);/*

	QueueTTS(dynamic_string(("Jack is a bird. Dusky is a German Shepard.")));
	QueueTTS(dynamic_string(("Hyper is a protogen and Toohmascene is a fox.")));
	QueueTTS(dynamic_string(("Arty is a fox and Sora is a ferret.")));*/
}

private void get_tts_audio(string text, struct MemoryStruct* chunk) {

	/*if (TryGetCachedAudio(text, chunk))
	{
		return;
	}*/

	CURL* curl;
	struct curl_slist* headers = NULL;

	chunk->memory = malloc(1);
	chunk->size = 0;

	curl = curl_easy_init();

	if (curl) {
		char postdata[MAX_TEXT_SIZE + 128];

		if (text->Count > MAX_TEXT_SIZE)
		{
			fprintf_red(stderr, "TTS Text was bigger than %i characters and would have overflown curl request buffer.\n", MAX_TEXT_SIZE);
			throw(IndexOutOfRangeException);
		}

		snprintf(postdata, sizeof(postdata), "{ %s,\"input\":\"%s\"}", POST_HEADER, text->Values);

		strings.Dispose(text);

		char auth_header[256];
		snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", api_key);

		headers = curl_slist_append(headers, "Content-Type: application/json");
		headers = curl_slist_append(headers, auth_header);

		curl_easy_setopt(curl, CURLOPT_URL, API_URL);
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postdata);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)chunk);

		CURLcode res = curl_easy_perform(curl);

		if (res != CURLE_OK) {
			fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
		}

		curl_easy_cleanup(curl);
		curl_slist_free_all(headers);

		//MaybeCacheTTSAudio(text, stack_string(CACHE_PATH), chunk);
	}
}


OnUpdate(1)
{
	if (GLOBAL_TTSQueue->Count)
	{
		string text = null;
		lock(GLOBAL_TTSQueueLocker, {
				text = at(GLOBAL_TTSQueue,0);
				arrays(string).RemoveIndex(GLOBAL_TTSQueue,0);
			});

		size_t textLength = text->Count;
		struct MemoryStruct audio_chunk;

		get_tts_audio(text, &audio_chunk);

		if (audio_chunk.memory) {
			if (Strings.Contains(audio_chunk.memory, audio_chunk.size, "error\":", sizeof("error\":") - 1))
			{
				fprintf(stderr, "Failed to get tts from openai error:\n%s\n", audio_chunk.memory);
				exit(-1);
			}

			play_audio_from_memory((Uint8*)audio_chunk.memory, audio_chunk.size);
		}
		else
		{
			fprintf(stderr, "Failed to get audio chunk");
		}
	}
}

AfterUpdate(1)
{
}

OnClose(1)
{
	curl_global_cleanup();
	SDL_CloseAudioDevice(deviceId);
	SDL_Quit();
}