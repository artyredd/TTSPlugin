#include "core/runtime.h"
#include "core/tasks.h"
#include "core/strings.h"
#include "core/file.h"

#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <SDL.h>
#include <SDL_audio.h>

#define API_URL "https://api.openai.com/v1/audio/speech"
#define MAX_TEXT_SIZE 2048
#define CACHE_CONTROL UCHAR_MAX
#define CACHE_SIZE 8192
#define CACHE_PATH "audio_cache.BIN"

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

CURL* curl;
struct curl_slist* headers = NULL;
const char* api_key = NULL;
array(tuple(string, string)) GLOBAL_TTSCache = empty_stack_array(tuple(string, string), CACHE_SIZE);
string controlSequence = stack_string("\31\1\xFF");

private void LoadTTSCache(string path)
{
	GLOBAL_TTSCache->Count = CACHE_SIZE;


	using (File file = Files.Open(path, FileModes.Read), file, Files)
	{
		while (file && !feof(file))
		{
			string text = dynamic_array(byte, 0);
			ulong dataLength = 0;
			string data = dynamic_array(byte, 0);

			Files.ReadUntil(file, text, controlSequence);
			fscanf(file, "%lli\31\1\xFF", &dataLength);
			Files.ReadUntil(file, data, controlSequence);

			if (text->Count is 0 or data->Count is 0)
			{
				strings.Dispose(text);
				strings.Dispose(data);
				continue;
			}

			int index = strings.Hash(text) % CACHE_SIZE;

			if (at(GLOBAL_TTSCache, index).First != null)
			{
				fprintf_red(stdout, "TTS Cache Colision Detected at index %i%s\n", index, "");
			}

			at(GLOBAL_TTSCache, index) = (tuple(string, string)){ text, data };
		}
	}

}

private bool TryGetCachedAudio(string text, struct MemoryStruct* chunk)
{
	int index = strings.Hash(text) % CACHE_SIZE;

	if (at(GLOBAL_TTSCache, index).First is null)
	{
		return false;
	}

	chunk->memory = at(GLOBAL_TTSCache, index).Second->Values;
	chunk->size = at(GLOBAL_TTSCache, index).Second->Count;

	return true;
}

private void MaybeCacheTTSAudio(string text, string path, struct MemoryStruct* chunk)
{
	int index = strings.Hash(text) % CACHE_SIZE;

	if (at(GLOBAL_TTSCache, index).First is null)
	{
		using (File file = Files.Open(path, FileModes.Append), file, Files)
		{
			fprintf_s(file, "%s%s", text->Values, controlSequence->Values);
			fprintf_s(file, "%lli%s", chunk->size, controlSequence->Values);

			for (int i = 0; i < chunk->size; i++)
			{
				int c = chunk->memory[i];
				fputc(c, file);
			}

			fprintf_s(file, "%s", controlSequence->Values);
		}
	}
}

array(string) GLOBAL_TTSQueue;
locker GLOBAL_TTSQueueLocker;

//#undef main
//void main()
//{
//	Application.Start();
//}


void QueueTTS(string text)
{
	lock(GLOBAL_TTSQueueLocker, {
		arrays(string).Append(GLOBAL_TTSQueue, text);
		});
}

OnStart(1)
{
	//LoadTTSCache(stack_string(CACHE_PATH));

	api_key = getenv("OPENAI_API_KEY");
	if (!api_key) {
		fprintf(stderr, "Environment variable OPENAI_API_KEY not set.\n");
		exit(1);
	}

	curl_global_init(CURL_GLOBAL_DEFAULT);
	curl = curl_easy_init();
	SDL_Init(SDL_INIT_AUDIO);
	GLOBAL_TTSQueue = dynamic_array(string, 1);

	//QueueTTS(dynamic_string(("Jack is a bird. Dusky is a German Shepard.")));
}

private void get_tts_audio(string text, struct MemoryStruct* chunk) {

	/*if (TryGetCachedAudio(text, chunk))
	{
		return;
	}*/

	chunk->memory = malloc(1);
	chunk->size = 0;

	if (curl) {
		char postdata[MAX_TEXT_SIZE + 128];

		if (text->Count > MAX_TEXT_SIZE)
		{
			fprintf_red(stderr, "TTS Text was bigger than %i characters and would have overflown curl request buffer.\n", MAX_TEXT_SIZE);
			throw(IndexOutOfRangeException);
		}

		snprintf(postdata, sizeof(postdata), "{ %s,\"input\":\"%s\"}", POST_HEADER, text->Values);


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