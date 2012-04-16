#pragma once
#pragma comment(lib, "X3DAudio.lib")

#include <XAudio2.h>
#include <X3DAudio.h>
#include <iostream>
#include <fstream>
#include <time.h>

enum SoundEffect { SFX_LASER, SFX_CRASH, SFX_ENGINE, SFX_BOOST, SFX_ROCKET, SFX_DROPMINE,
	SFX_SCREAM1, SFX_SCREAM2, SFX_SCREAM3, SFX_SCREAM, SFX_CAREXPLODE, SFX_EXPLOSION, SFX_BEEP,
	SFX_ROCKETLAUNCH, SFX_PICKUP, SFX_SELECT, SFX_SHOTGUN, SFX_TAKENLEAD, SFX_LOSTLEAD,
	SFX_NOAMMO, SFX_ONE, SFX_TWO, SFX_THREE };

#define NUM_EMITTERS 500

class Sound
{
public:
	Sound(void);
	~Sound(void);
	void initialize();
	void shutdown();
	void returnEmitter();
	IXAudio2SourceVoice* getSFXVoice();
	IXAudio2SourceVoice* reserveSFXVoice();

	void playSoundEffect(SoundEffect effect, X3DAUDIO_EMITTER* emit);
	void playEngine(X3DAUDIO_EMITTER* emit, float freq, IXAudio2SourceVoice* engine);
	void playRocket(X3DAUDIO_EMITTER* emit, IXAudio2SourceVoice* rocket);
	void playInGameMusic();
	void playMenuMusic();
	void stopMusic();
	

	bool initialized;

	X3DAUDIO_EMITTER* getEmitter();
	X3DAUDIO_LISTENER listener;

	static Sound* sound;

	X3DAUDIO_EMITTER* playerEmitter;

private:
	// Some methods from MSDN
	HRESULT FindChunk(HANDLE hFile, DWORD fourcc, DWORD & dwChunkSize, DWORD & dwChunkDataPosition);
	HRESULT ReadChunkData(HANDLE hFile, void * buffer, DWORD buffersize, DWORD bufferoffset);

	void loadSound(SoundEffect type, std::string filename);
	void loadMusic(IXAudio2SourceVoice* &voice, std::string filename, BYTE* &soundBuffer, UINT32* &xwmaBuffer);

	IXAudio2* audio;
	IXAudio2MasteringVoice* mVoice;

	IXAudio2SubmixVoice* smSFX;
	IXAudio2SubmixVoice* smMusic;

	XAUDIO2_SEND_DESCRIPTOR SFXSend;
	XAUDIO2_VOICE_SENDS SFXSendList;

	XAUDIO2_SEND_DESCRIPTOR musicSend;
	XAUDIO2_VOICE_SENDS musicSendList;


	X3DAUDIO_HANDLE audio3DHandle;

	X3DAUDIO_EMITTER* emitters;

	X3DAUDIO_DSP_SETTINGS dspSettings;

	int numClaimedEmitters;

	XAUDIO2_DEVICE_DETAILS details;


	IXAudio2SourceVoice* ingamemusic;
	IXAudio2SourceVoice* menumusic;

	BYTE* ingamemusicBuffer;
	BYTE* menumusicBuffer;


	XAUDIO2_BUFFER* laserBufferDetails;
	XAUDIO2_BUFFER* crashBufferDetails;
	XAUDIO2_BUFFER* engineBufferDetails;
	XAUDIO2_BUFFER* boostBufferDetails;
	XAUDIO2_BUFFER* rocketBufferDetails;
	XAUDIO2_BUFFER* dropmineBufferDetails;
	XAUDIO2_BUFFER* scream1BufferDetails;
	XAUDIO2_BUFFER* scream2BufferDetails;
	XAUDIO2_BUFFER* scream3BufferDetails;
	XAUDIO2_BUFFER* explosionBufferDetails;
	XAUDIO2_BUFFER* carexplodeBufferDetails;
	XAUDIO2_BUFFER* beepBufferDetails;
	XAUDIO2_BUFFER* rocketlaunchBufferDetails;
	XAUDIO2_BUFFER* pickupBufferDetails;
	XAUDIO2_BUFFER* selectBufferDetails;
	XAUDIO2_BUFFER* shotgunBufferDetails;
	XAUDIO2_BUFFER* takenleadBufferDetails;
	XAUDIO2_BUFFER* lostleadBufferDetails;
	XAUDIO2_BUFFER* noammoBufferDetails;
	XAUDIO2_BUFFER* oneBufferDetails;
	XAUDIO2_BUFFER* twoBufferDetails;
	XAUDIO2_BUFFER* threeBufferDetails;

	XAUDIO2_BUFFER_WMA* laserWMABuffer;
	XAUDIO2_BUFFER_WMA* crashWMABuffer;
	XAUDIO2_BUFFER_WMA* engineWMABuffer;
	XAUDIO2_BUFFER_WMA* boostWMABuffer;
	XAUDIO2_BUFFER_WMA* rocketWMABuffer;
	XAUDIO2_BUFFER_WMA* dropmineWMABuffer;
	XAUDIO2_BUFFER_WMA* scream1WMABuffer;
	XAUDIO2_BUFFER_WMA* scream2WMABuffer;
	XAUDIO2_BUFFER_WMA* scream3WMABuffer;
	XAUDIO2_BUFFER_WMA* explosionWMABuffer;
	XAUDIO2_BUFFER_WMA* carexplodeWMABuffer;
	XAUDIO2_BUFFER_WMA* beepWMABuffer;
	XAUDIO2_BUFFER_WMA* rocketlaunchWMABuffer;
	XAUDIO2_BUFFER_WMA* pickupWMABuffer;
	XAUDIO2_BUFFER_WMA* selectWMABuffer;
	XAUDIO2_BUFFER_WMA* shotgunWMABuffer;
	XAUDIO2_BUFFER_WMA* takenleadWMABuffer;
	XAUDIO2_BUFFER_WMA* lostleadWMABuffer;
	XAUDIO2_BUFFER_WMA* noammoWMABuffer;
	XAUDIO2_BUFFER_WMA* oneWMABuffer;
	XAUDIO2_BUFFER_WMA* twoWMABuffer;
	XAUDIO2_BUFFER_WMA* threeWMABuffer;


	int currentVoice;
	int maxVoices;

	int currentReservedVoice;
	int maxReservedVoices;

	IXAudio2SourceVoice** voiceBuffer;
	IXAudio2SourceVoice** voiceBufferReserved;

	WAVEFORMATEXTENSIBLE* wfm;

	UINT32* ingameMusicXMABuffer;
	UINT32* menuMusicXMABuffer;
};
