#include "soundmanager.h"

#include "cvardef.h"
#include "wrect.h"
#include "cl_dll.h"
#include "parsemsg.h"
#include <random>
#include "cpp_aux.h"
#include "fs_aux.h"

#include <regex>

extern cl_enginefunc_t gEngfuncs;
extern int g_musicSlowmotion;

extern cvar_t *slowmotion_effect_change_duration;
extern cvar_t *sm_buffer;
float sm_buffer_last_value = 0.0f;

extern cvar_t *slowmotion_low_pass_cutoff;
float slowmotion_low_pass_cutoff_desired_last_value = 22049.0f;
float slowmotion_low_pass_cutoff_desired_value = 22049.0f;
float slowmotion_low_pass_cutoff_time_end = 0.0f;

extern cvar_t *slowmotion_negative_pitch;
float slowmotion_pitch_desired_last_value = 0;
float slowmotion_pitch_desired_value = 0;

extern int isPaused;
int lastIsPaused = 1;

cvar_t *MP3Volume = 0;
cvar_t *volume = 0;

int initialised = 0;

ManagedStream music;
ManagedStream commentary;

void SM_Init() {
	initialised = BASS_Init( -1, 44100, 0, NULL, NULL );
	if ( !initialised ) {
		gEngfuncs.Con_Printf( "Failed to initialize BASS. Music playback won't be available.\n" );
	}

	BASS_SetConfig( BASS_CONFIG_UPDATEPERIOD, 5 );
	BASS_SetConfig( BASS_CONFIG_BUFFER, ( DWORD ) sm_buffer->value );

	gEngfuncs.pfnHookUserMsg( "BassPlay", SM_OnBassPlay );
	gEngfuncs.pfnHookUserMsg( "BassStop", SM_OnBassStop );
	gEngfuncs.pfnHookUserMsg( "BassStopC", SM_OnBassStopC );
	gEngfuncs.pfnHookUserMsg( "BassComm", SM_OnBassComm );
	
	MP3Volume = gEngfuncs.pfnGetCvarPointer( "MP3Volume" );
	volume = gEngfuncs.pfnGetCvarPointer( "volume" );

	music.volume = MP3Volume->value;
	music.lastVolume = MP3Volume->value;

	commentary.volume = volume->value;
	commentary.lastVolume = volume->value;

	lastIsPaused = isPaused;
}

void SM_Play( ManagedStream &stream, const char *soundPath, int looping ) {
	if ( !initialised ) {
		return;
	}

	static std::regex backslashRegex( "\\\\+" );
	std::string sanitizedSoundPath = std::regex_replace( soundPath, backslashRegex, "/" );

	if ( stream.handle ) {
		if ( stream.loadedSoundPath == sanitizedSoundPath ) {
			return;
		}

		if ( !SM_Stop( stream ) ) {
			SM_CheckError();
			return;
		}
	}

	stream.handle = BASS_StreamCreateFile( false, sanitizedSoundPath.c_str(), 0, 0, BASS_STREAM_DECODE | BASS_STREAM_PRESCAN );
	stream.sliding = false;

	if ( !stream.handle ) {
		SM_CheckError();
		return;
	}

	stream.channelLength = BASS_ChannelGetLength( stream.handle, BASS_POS_BYTE );
	if ( stream.channelLength == -1 ) {
		SM_CheckError();
		return;
	}

	if ( !BASS_ChannelGetInfo( stream.handle, &stream.channelInfo ) ) {
		SM_CheckError();
		return;
	}
	
	stream.bqfEffect = BASS_ChannelSetFX( stream.handle, BASS_FX_BFX_BQF, 1 );
	if ( !stream.bqfEffect ) {
		// TODO: show error that there won't be slowmo music effects
		SM_CheckError();
		return;
	} else {
		BASS_FXGetParameters( stream.bqfEffect, &stream.bqfParams );

		stream.bqfParams.lFilter = BASS_BFX_BQF_LOWPASS;
		stream.bqfParams.fCenter = ( float ) ( ( stream.channelInfo.freq / 2 ) - 1 );
		stream.bqfParams.fGain = 0.0f;
		stream.bqfParams.fBandwidth = 1.0f;
	}

	stream.handle = BASS_FX_TempoCreate( stream.handle, BASS_FX_FREESOURCE );
	if ( !stream.handle ) {
		SM_CheckError();
		return;
	}

	BASS_ChannelSetAttribute( stream.handle, BASS_ATTRIB_VOL, MP3Volume->value );

	if ( !BASS_ChannelPlay( stream.handle, false ) ) {
		SM_CheckError();
		return;
	}

	stream.loadedSoundPath = sanitizedSoundPath;
}

void SM_PlayMusic( const char *soundPath, int looping ) {
	SM_Play( music, soundPath, looping );
}

void SM_PlayRandomMainMenuMusic() {
	std::vector<std::string> musicFiles = FS_GetAllFilesInDirectory( ".\\half_payne\\media\\menu" );

	if ( !musicFiles.empty() ) {
		SM_Play( music, aux::rand::choice( musicFiles ).c_str() );
	}
}

void SM_SetPaused( ManagedStream &stream, bool paused ) {
	if ( !stream.handle || !initialised ) {
		return;
	}

	if ( paused ) {
		if ( !BASS_ChannelPause( stream.handle ) ) {
			SM_CheckError();
			return;
		}
	} else {
		if ( !BASS_ChannelPlay( stream.handle, false ) ) {
			SM_CheckError();
			return;
		}
	}
}

bool SM_Stop( ManagedStream &stream ) {
	if ( !stream.handle || !initialised ) {
		return true;
	}

	BASS_ChannelStop( stream.handle );

	int success = BASS_StreamFree( stream.handle );
	stream.handle = 0;
	stream.loadedSoundPath = "";

	return success > 0;
}

bool SM_StopMusic() {
	return SM_Stop( music );
}

void SM_StopSmooth( ManagedStream &stream ) {
	if ( !stream.handle || !initialised ) {
		return;
	}

	BASS_ChannelSlideAttribute( stream.handle, BASS_ATTRIB_VOL, -1.0f, 2000 );
	stream.sliding = true;

	return;
}

void SM_Seek( ManagedStream &stream, double positionSeconds ) {
	if ( !stream.handle || !initialised ) {
		return;
	}

	QWORD audioPos = BASS_ChannelSeconds2Bytes( stream.handle, positionSeconds );
	if ( !BASS_ChannelSetPosition( stream.handle, audioPos, BASS_POS_BYTE ) ) {
		SM_CheckError();
		return;
	}
}

void SM_SeekMusic( double positionSeconds ) {
	SM_Seek( music, positionSeconds );
}

void SM_SetPitch( ManagedStream &stream, float pitch ) {
	if ( !stream.handle || !initialised ) {
		return;
	}

	if ( !BASS_ChannelSetAttribute( stream.handle, BASS_ATTRIB_TEMPO_PITCH, pitch ) ) {
		SM_CheckError();
		return;
	}
}

int SM_OnBassPlay( const char *pszName,  int iSize, void *pbuf ) {
	BEGIN_READ( pbuf, iSize );
	const char *soundPath = READ_STRING();
	float pos = READ_FLOAT();
	int slowmotion = READ_BYTE();
	int looping = READ_BYTE();

	SM_Play( music, soundPath, looping );
	if ( music.handle ) {
		if ( looping ) {
			BASS_ChannelFlags( music.handle, BASS_SAMPLE_LOOP, BASS_SAMPLE_LOOP );
		} else {
			BASS_ChannelFlags( music.handle, 0, BASS_SAMPLE_LOOP ); 
		}
	}
	SM_Seek( music, pos );
	SM_SetSlowmotion( music, slowmotion );

	if ( slowmotion ) {
		SM_ApplyLowPassEffects( music );
		SM_ApplyPitchEffects( music );
	}

	return 1;
}

int SM_OnBassStop( const char *pszName,  int iSize, void *pbuf ) {
	BEGIN_READ( pbuf, iSize );

	int smooth = READ_BYTE();
	if ( smooth ) {
		SM_StopSmooth( music );
	} else {
		SM_Stop( music );
	}

	return 1;
}

int SM_OnBassStopC( const char *pszName,  int iSize, void *pbuf ) {
	BEGIN_READ( pbuf, iSize );

	SM_Stop( commentary );

	return 1;
}

int SM_OnBassComm( const char *pszName,  int iSize, void *pbuf ) {
	BEGIN_READ( pbuf, iSize );

	std::string soundPath = std::string( "./half_payne/sound/" ) + READ_STRING();

	SM_Play( commentary, soundPath.c_str() );
	BASS_ChannelSetAttribute( commentary.handle, BASS_ATTRIB_VOL, volume->value );
	commentary.loadedSoundPath = "";

	return 1;
}

void SM_SetSlowmotion( ManagedStream &stream, int slowmotion ) {
	if ( slowmotion ) {
		float value = slowmotion_negative_pitch->value;
		if ( value > 0 ) {
			value *= -1.0f;
		}

		slowmotion_pitch_desired_value = min( 0, max( -50, value ) );
		slowmotion_low_pass_cutoff_desired_value = slowmotion_low_pass_cutoff->value;
	} else {
		slowmotion_pitch_desired_value = 0;
		slowmotion_low_pass_cutoff_desired_value = ( float ) ( ( stream.channelInfo.freq / 2 ) - 1 );
	}
}

void SM_ApplyLowPassEffects( ManagedStream &stream ) {
	float absoluteTime = ( float ) gEngfuncs.GetAbsoluteTime();

	float c = slowmotion_low_pass_cutoff_desired_value - slowmotion_low_pass_cutoff_desired_last_value;
	float t = absoluteTime - ( slowmotion_low_pass_cutoff_time_end - slowmotion_effect_change_duration->value );
	float d = slowmotion_effect_change_duration->value;
	float b = slowmotion_low_pass_cutoff_desired_last_value;

	if ( c >= 0 ) {
		t /= d;
		stream.bqfParams.fCenter = c * t * t * t * t * t + b;
	} else {
		t /= d;
		t--;
		stream.bqfParams.fCenter = c * ( t * t * t * t * t + 1 ) + b;
	}

	float max_cutoff = ( float ) ( ( stream.channelInfo.freq / 2 ) - 1 );

	if ( absoluteTime >= slowmotion_low_pass_cutoff_time_end ) {
		slowmotion_low_pass_cutoff_time_end = 0.0f;
		slowmotion_low_pass_cutoff_desired_last_value = slowmotion_low_pass_cutoff_desired_value;
		stream.bqfParams.fCenter = slowmotion_low_pass_cutoff_desired_value;
	}

	if ( stream.bqfParams.fCenter < 1 ) {
		stream.bqfParams.fCenter = 1;
	} else if ( stream.bqfParams.fCenter > max_cutoff ) {
		stream.bqfParams.fCenter = max_cutoff;
	}

	stream.bqfParams.lChannel = stream.bqfParams.fCenter > 12000 ? BASS_BFX_CHANNONE : BASS_BFX_CHANALL;

	BASS_FXSetParameters( stream.bqfEffect, &stream.bqfParams );
}

void SM_ApplyPitchEffects( ManagedStream &stream ) {
	BASS_ChannelSlideAttribute( stream.handle, BASS_ATTRIB_TEMPO_PITCH, slowmotion_pitch_desired_value, ( DWORD ) slowmotion_effect_change_duration->value * 1000 );
	slowmotion_pitch_desired_last_value = slowmotion_pitch_desired_value;
}

void SM_Think( double time ) {
	if ( isPaused != lastIsPaused ) {
		if ( music.handle ) {
			SM_SetPaused( music, isPaused > 0 );
		}
		if ( commentary.handle ) {
			SM_SetPaused( commentary, isPaused > 0 );
		}
		lastIsPaused = isPaused;
	}

	{
		if ( sm_buffer->value != sm_buffer_last_value ) {
			BASS_SetConfig( BASS_CONFIG_BUFFER, ( DWORD ) sm_buffer->value );
			sm_buffer_last_value = sm_buffer->value;
		}
	}

	SM_ThinkMusic( time );
	SM_ThinkCommentary( time );
}

void SM_SetConsoleVarState( double pos, const char *soundPath, int looping ) {
	gEngfuncs.Cvar_SetValue( "sm_current_pos", ( float ) pos );
	gEngfuncs.Cvar_Set( "sm_current_file", ( char * ) soundPath );
	gEngfuncs.Cvar_SetValue( "sm_looping", looping ? 1.0f : 0.0f );
}

void SM_ThinkMusic( double time ) {
	if ( !music.handle ) {
		SM_SetConsoleVarState( 0, "", false );
		return;
	}

	float absoluteTime = ( float ) gEngfuncs.GetAbsoluteTime();

	// CONSOLE VARS STATE
	{
		QWORD posInBytes = BASS_ChannelGetPosition( music.handle, BASS_POS_BYTE );
		DWORD looping = BASS_ChannelFlags( music.handle, 0, 0 ) & BASS_SAMPLE_LOOP;
		if ( posInBytes >= music.channelLength ) {
			SM_SetConsoleVarState( 0, "", false );
			if ( !looping ) {
				SM_Stop( music );
			}
		} else {
			float pos = ( float ) BASS_ChannelBytes2Seconds( music.handle, posInBytes );
			SM_SetConsoleVarState( pos, music.loadedSoundPath.c_str(), looping );
		}
	}

	// LOW PASS FILTER
	{
		if ( !slowmotion_low_pass_cutoff_time_end && slowmotion_low_pass_cutoff_desired_value != slowmotion_low_pass_cutoff_desired_last_value ) {
			slowmotion_low_pass_cutoff_time_end = absoluteTime + slowmotion_effect_change_duration->value;
		}

		if ( slowmotion_low_pass_cutoff_time_end ) {
			SM_ApplyLowPassEffects( music );
		}
	}

	// VOLUME
	{
		music.volume = MP3Volume->value;
		SM_ThinkVolume( music );
	}

	// PITCH
	{
		if ( slowmotion_pitch_desired_value != slowmotion_pitch_desired_last_value ) {
			SM_ApplyPitchEffects( music );
		}
	}

	// AFTER FADE OUT
	{
		if ( music.sliding && !BASS_ChannelIsSliding( music.handle, BASS_ATTRIB_VOL ) ) {
			music.sliding = false;
			SM_Stop( music );
		}
	}

	SM_SetSlowmotion( music, g_musicSlowmotion );
}

void SM_ThinkCommentary( double time ) {
	{
		QWORD posInBytes = BASS_ChannelGetPosition( commentary.handle, BASS_POS_BYTE );
		DWORD looping = BASS_ChannelFlags( commentary.handle, 0, 0 ) & BASS_SAMPLE_LOOP;
		if ( posInBytes >= commentary.channelLength ) {
			SM_Stop( commentary );
		}
	}
}

void SM_ThinkVolume( ManagedStream &stream, float min, float max ) {
	if ( stream.volume != stream.lastVolume ) {
		float appliedVolume = stream.volume;
		if ( appliedVolume < min ) {
			appliedVolume = min;
		}
		if ( appliedVolume > max ) {
			appliedVolume = max;
		}

		BASS_ChannelSlideAttribute( stream.handle, BASS_ATTRIB_VOL, appliedVolume, 500 );
		stream.lastVolume = stream.volume;
	}
}

void SM_CheckError() {
	int errorCode = BASS_ErrorGetCode();

	switch ( errorCode ) {
		case 0:
			break;

		case 1:
			gEngfuncs.pfnConsolePrint( "BASS_ERROR_MEM\n" );
			break;

		case 2:
			gEngfuncs.pfnConsolePrint( "BASS_ERROR_FILEOPEN\n" );
			break;

		case 3:
			gEngfuncs.pfnConsolePrint( "BASS_ERROR_DRIVER\n" );
			break;

		case 4:
			gEngfuncs.pfnConsolePrint( "BASS_ERROR_BUFLOST\n" );
			break;

		case 5:
			gEngfuncs.pfnConsolePrint( "BASS_ERROR_HANDLE\n" );
			break;

		case 6:
			gEngfuncs.pfnConsolePrint( "BASS_ERROR_FORMAT\n" );
			break;

		case 7:
			gEngfuncs.pfnConsolePrint( "BASS_ERROR_POSITION\n" );
			break;

		case 8:
			gEngfuncs.pfnConsolePrint( "BASS_ERROR_INIT\n" );
			break;

		case 9:
			gEngfuncs.pfnConsolePrint( "BASS_ERROR_START\n" );
			break;

		case 14:
			gEngfuncs.pfnConsolePrint( "BASS_ERROR_ALREADY\n" );
			break;

		case 18:
			gEngfuncs.pfnConsolePrint( "BASS_ERROR_NOCHAN\n" );
			break;

		case 19:
			gEngfuncs.pfnConsolePrint( "BASS_ERROR_ILLTYPE\n" );
			break;

		case 20:
			gEngfuncs.pfnConsolePrint( "BASS_ERROR_ILLPARAM\n" );
			break;

		case 21:
			gEngfuncs.pfnConsolePrint( "BASS_ERROR_NO3D\n" );
			break;

		case 22:
			gEngfuncs.pfnConsolePrint( "BASS_ERROR_NOEAX\n" );
			break;

		case 23:
			gEngfuncs.pfnConsolePrint( "BASS_ERROR_DEVICE\n" );
			break;

		case 24:
			gEngfuncs.pfnConsolePrint( "BASS_ERROR_NOPLAY\n" );
			break;

		case 25:
			gEngfuncs.pfnConsolePrint( "BASS_ERROR_FREQ\n" );
			break;

		case 27:
			gEngfuncs.pfnConsolePrint( "BASS_ERROR_NOTFILE\n" );
			break;

		case 29:
			gEngfuncs.pfnConsolePrint( "BASS_ERROR_NOHW\n" );
			break;

		case 31:
			gEngfuncs.pfnConsolePrint( "BASS_ERROR_EMPTY\n" );
			break;

		case 32:
			gEngfuncs.pfnConsolePrint( "BASS_ERROR_NONET\n" );
			break;

		case 33:
			gEngfuncs.pfnConsolePrint( "BASS_ERROR_CREATE\n" );
			break;

		case 34:
			gEngfuncs.pfnConsolePrint( "BASS_ERROR_NOFX\n" );
			break;

		case 37:
			gEngfuncs.pfnConsolePrint( "BASS_ERROR_NOTAVAIL\n" );
			break;

		case 38:
			gEngfuncs.pfnConsolePrint( "BASS_ERROR_DECODE\n" );
			break;

		case 39:
			gEngfuncs.pfnConsolePrint( "BASS_ERROR_DX\n" );
			break;

		case 40:
			gEngfuncs.pfnConsolePrint( "BASS_ERROR_TIMEOUT\n" );
			break;

		case 41:
			gEngfuncs.pfnConsolePrint( "BASS_ERROR_FILEFORM\n" );
			break;

		case 42:
			gEngfuncs.pfnConsolePrint( "BASS_ERROR_SPEAKER\n" );
			break;

		case 43:
			gEngfuncs.pfnConsolePrint( "BASS_ERROR_VERSION\n" );
			break;

		case 44:
			gEngfuncs.pfnConsolePrint( "BASS_ERROR_CODEC\n" );
			break;

		case 45:
			gEngfuncs.pfnConsolePrint( "BASS_ERROR_ENDED\n" );
			break;

		case 46:
			gEngfuncs.pfnConsolePrint( "BASS_ERROR_BUSY\n" );
			break;

		case -1:
			gEngfuncs.pfnConsolePrint( "BASS_ERROR_UNKNOWN\n" );
			break;

		default:
			gEngfuncs.pfnConsolePrint( "BASS_ERROR_UNKNOWN\n" );
			break;
	}
}