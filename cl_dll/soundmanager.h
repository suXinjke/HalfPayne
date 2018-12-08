#ifndef SOUNDMANAGER_H
#define SOUNDMANAGER_H

#include "bass.h"
#include "bass_fx.h"
#include <string>

void SM_Init();

struct ManagedStream {
	HSTREAM handle = 0;
	BASS_CHANNELINFO channelInfo;
	QWORD channelLength;
	std::string loadedSoundPath;
	HFX bqfEffect = 0;
	BASS_BFX_BQF bqfParams;
	bool sliding = false;

	float volume;
	float lastVolume;
};

void SM_Play( ManagedStream &stream, const char *soundPath, int looping = false );
void SM_PlayMusic( const char *soundPath, int looping = false );
void SM_PlayRandomMainMenuMusic();
void SM_SetPaused( ManagedStream &stream, bool paused );
bool SM_Stop( ManagedStream &stream );
bool SM_StopMusic();
void SM_StopSmooth( ManagedStream &stream );
void SM_Seek( ManagedStream &stream, double positionSeconds );
void SM_SeekMusic( double positionSeconds );
void SM_SetPitch( ManagedStream &stream, float pitch );
void SM_SetSlowmotion( ManagedStream &stream, int slowmotion );
void SM_ApplyLowPassEffects( ManagedStream &stream );
void SM_ApplyPitchEffects( ManagedStream &stream );

void SM_Think( double time );
void SM_ThinkMusic( double time );
void SM_ThinkCommentary( double time );
void SM_ThinkVolume( ManagedStream &stream, float min = 0.0f, float max = 1.0f );

void SM_CheckError();

// Messages
int SM_OnBassPlay( const char *pszName, int iSize, void *pbuf );
int SM_OnBassStop( const char *pszName, int iSize, void *pbuf );
int SM_OnBassStopC( const char *pszName, int iSize, void *pbuf );
int SM_OnBassComm( const char *pszName, int iSize, void *pbuf );

#endif // SOUNDMANAGER_H