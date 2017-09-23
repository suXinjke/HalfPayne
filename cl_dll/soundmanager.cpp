#include "soundmanager.h"

#include "bass.h"
#include "bass_fx.h"

#include "cvardef.h"
#include "wrect.h"
#include "cl_dll.h"
#include "parsemsg.h"
#include <random>

#include <regex>

extern cl_enginefunc_t gEngfuncs;

extern cvar_t *slowmotion_effect_change_duration;

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
float MP3Volume_last_value = 0.0f;

int initialised = 0;
HSTREAM stream = 0;
BASS_CHANNELINFO channelInfo;
QWORD channelLength;
std::string loadedSoundPath;

HFX bqfEffect = 0;
BASS_BFX_BQF bqfParams;

void SM_Init() {
	initialised = BASS_Init( -1, 44100, 0, NULL, NULL );
	if ( !initialised ) {
		gEngfuncs.Con_Printf( "Failed to initialize BASS. Music playback won't be available.\n" );
	}

	BASS_SetConfig( BASS_CONFIG_UPDATEPERIOD, 5 );
	BASS_SetConfig( BASS_CONFIG_BUFFER, 64 );

	gEngfuncs.pfnHookUserMsg( "BassPlay", SM_OnBassPlay );
	gEngfuncs.pfnHookUserMsg( "BassStop", SM_OnBassStop );
	gEngfuncs.pfnHookUserMsg( "BassSlowmo", SM_OnBassSlowmo );
	
	MP3Volume = gEngfuncs.pfnGetCvarPointer( "MP3Volume" );
	MP3Volume_last_value = MP3Volume->value;

	lastIsPaused = isPaused;
}

void SM_Play( const char *soundPath, bool looping ) {
	if ( !initialised ) {
		return;
	}

	static std::regex backslashRegex( "\\\\+" );
	std::string sanitizedSoundPath = std::regex_replace( soundPath, backslashRegex, "/" );

	if ( stream ) {
		if ( loadedSoundPath == sanitizedSoundPath ) {
			return;
		}

		if ( !SM_Stop() ) {
			SM_CheckError();
			return;
		}
	}

	stream = BASS_StreamCreateFile( false, sanitizedSoundPath.c_str(), 0, 0, BASS_STREAM_DECODE | BASS_STREAM_PRESCAN );

	if ( !stream ) {
		SM_CheckError();
		return;
	}

	channelLength = BASS_ChannelGetLength( stream, BASS_POS_BYTE );
	if ( channelLength == -1 ) {
		SM_CheckError();
		return;
	}

	if ( !BASS_ChannelGetInfo( stream, &channelInfo ) ) {
		SM_CheckError();
		return;
	}
	
	bqfEffect = BASS_ChannelSetFX( stream, BASS_FX_BFX_BQF, 1 );
	if ( !bqfEffect ) {
		// TODO: show error that there won't be slowmo music effects
		SM_CheckError();
		return;
	} else {
		BASS_FXGetParameters( bqfEffect, &bqfParams );

		bqfParams.lFilter = BASS_BFX_BQF_LOWPASS;
		bqfParams.fCenter = ( float ) ( ( channelInfo.freq / 2 ) - 1 );
		bqfParams.fGain = 0.0f;
		bqfParams.fBandwidth = 1.0f;
	}

	stream = BASS_FX_TempoCreate( stream, BASS_FX_FREESOURCE );
	if ( !stream ) {
		SM_CheckError();
		return;
	}

	BASS_ChannelSetAttribute( stream, BASS_ATTRIB_VOL, MP3Volume->value );

	if ( !BASS_ChannelPlay( stream, false ) ) {
		SM_CheckError();
		return;
	}

	loadedSoundPath = sanitizedSoundPath;
}

void SM_PlayRandomMainMenuMusic() {
	std::vector<std::string> backgroundFolders;

	WIN32_FIND_DATA fdFile;
	HANDLE hFind = NULL;

	const char *root = ".\\half_payne\\media\\menu";

	char sPath[2048];
	sprintf( sPath, "%s\\*.*", root );

	if ( ( hFind = FindFirstFile( sPath, &fdFile ) ) == INVALID_HANDLE_VALUE ) {
		return;
	}

	do {
		if ( strcmp( fdFile.cFileName, "." ) != 0 && strcmp( fdFile.cFileName, ".." ) != 0 ) {

			sprintf( sPath, "%s\\%s", root, fdFile.cFileName );
			backgroundFolders.push_back( sPath );
		}
	} while ( FindNextFile( hFind, &fdFile ) );

	FindClose( hFind );

	static std::random_device rd;
	static std::mt19937 gen( rd() );
	std::uniform_int_distribution<> dis( 0, backgroundFolders.size() - 1 );

	SM_Play( backgroundFolders.at( dis( gen ) ).c_str() );
}

void SM_SetPaused( bool paused ) {
	if ( !stream || !initialised ) {
		return;
	}

	if ( paused ) {
		if ( !BASS_ChannelPause( stream ) ) {
			SM_CheckError();
			return;
		}
	} else {
		if ( !BASS_ChannelPlay( stream, false ) ) {
			SM_CheckError();
			return;
		}
	}
}

bool SM_Stop() {
	if ( !stream || !initialised ) {
		return true;
	}

	BASS_ChannelStop( stream );

	BOOL success = BASS_StreamFree( stream );
	stream = 0;
	loadedSoundPath = "";

	return success > 0;
}

void SM_Seek( double positionSeconds ) {
	if ( !stream || !initialised ) {
		return;
	}

	QWORD audioPos = BASS_ChannelSeconds2Bytes( stream, positionSeconds );
	if ( !BASS_ChannelSetPosition( stream, audioPos, BASS_POS_BYTE ) ) {
		SM_CheckError();
		return;
	}
}

void SM_SetPitch( float pitch ) {
	if ( !stream || !initialised ) {
		return;
	}

	if ( !BASS_ChannelSetAttribute( stream, BASS_ATTRIB_TEMPO_PITCH, pitch ) ) {
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

	SM_Play( soundPath, looping );
	if ( stream ) {
		if ( looping ) {
			BASS_ChannelFlags( stream, BASS_SAMPLE_LOOP, BASS_SAMPLE_LOOP );
		} else {
			BASS_ChannelFlags( stream, 0, BASS_SAMPLE_LOOP ); 
		}
	}
	SM_Seek( pos );
	SM_SetSlowmotion( slowmotion );

	if ( slowmotion ) {
		SM_ApplyLowPassEffects();
		SM_ApplyPitchEffects();
	}

	return 1;
}

int SM_OnBassStop( const char *pszName,  int iSize, void *pbuf ) {
	BEGIN_READ( pbuf, iSize );

	SM_Stop();

	return 1;
}

int SM_OnBassSlowmo( const char *pszName,  int iSize, void *pbuf ) {
	BEGIN_READ( pbuf, iSize );

	int slowmotion = READ_BYTE();

	if ( !stream ) {
		return 1;
	}

	SM_SetSlowmotion( ( bool ) slowmotion );

	return 1;
}

void SM_SetSlowmotion( bool slowmotion ) {
	if ( slowmotion ) {
		float value = slowmotion_negative_pitch->value;
		if ( value > 0 ) {
			value *= -1.0f;
		}

		slowmotion_pitch_desired_value = min( 0, max( -50, value ) );
		slowmotion_low_pass_cutoff_desired_value = slowmotion_low_pass_cutoff->value;
	} else {
		slowmotion_pitch_desired_value = 0;
		slowmotion_low_pass_cutoff_desired_value = ( float ) ( ( channelInfo.freq / 2 ) - 1 );
	}
}

void SM_ApplyLowPassEffects() {
	float absoluteTime = ( float ) gEngfuncs.GetAbsoluteTime();

	float c = slowmotion_low_pass_cutoff_desired_value - slowmotion_low_pass_cutoff_desired_last_value;
	float t = absoluteTime - ( slowmotion_low_pass_cutoff_time_end - slowmotion_effect_change_duration->value );
	float d = slowmotion_effect_change_duration->value;
	float b = slowmotion_low_pass_cutoff_desired_last_value;

	if ( c >= 0 ) {
		t /= d;
		bqfParams.fCenter = c * t * t * t * t * t + b;
	} else {
		t /= d;
		t--;
		bqfParams.fCenter = c * ( t * t * t * t * t + 1 ) + b;
	}

	float max_cutoff = ( float ) ( ( channelInfo.freq / 2 ) - 1 );

	if ( absoluteTime >= slowmotion_low_pass_cutoff_time_end ) {
		slowmotion_low_pass_cutoff_time_end = 0.0f;
		slowmotion_low_pass_cutoff_desired_last_value = slowmotion_low_pass_cutoff_desired_value;
		bqfParams.fCenter = slowmotion_low_pass_cutoff_desired_value;
	}

	if ( bqfParams.fCenter < 1 ) {
		bqfParams.fCenter = 1;
	} else if ( bqfParams.fCenter > max_cutoff ) {
		bqfParams.fCenter = max_cutoff;
	}

	bqfParams.lChannel = bqfParams.fCenter > 12000 ? BASS_BFX_CHANNONE : BASS_BFX_CHANALL;

	BASS_FXSetParameters( bqfEffect, &bqfParams );
}

void SM_ApplyPitchEffects() {
	BASS_ChannelSlideAttribute( stream, BASS_ATTRIB_TEMPO_PITCH, slowmotion_pitch_desired_value, ( DWORD ) slowmotion_effect_change_duration->value * 1000 );
	slowmotion_pitch_desired_last_value = slowmotion_pitch_desired_value;
}

void SM_SetConsoleVarState( double pos, const char *soundPath, bool looping ) {
	gEngfuncs.Cvar_SetValue( "sm_current_pos", ( float ) pos );
	gEngfuncs.Cvar_Set( "sm_current_file", ( char * ) soundPath );
	gEngfuncs.Cvar_SetValue( "sm_looping", looping ? 1.0f : 0.0f );
}

void SM_Think( double time ) {
	if ( !stream ) {
		SM_SetConsoleVarState( 0, "", false );
		return;
	}

	float absoluteTime = ( float ) gEngfuncs.GetAbsoluteTime();

	// CONSOLE VARS STATE
	{
		QWORD posInBytes = BASS_ChannelGetPosition( stream, BASS_POS_BYTE );
		bool looping = BASS_ChannelFlags( stream, 0, 0 ) & BASS_SAMPLE_LOOP;
		if ( posInBytes >= channelLength ) {
			SM_SetConsoleVarState( 0, "", false );
			if ( !looping ) {
				SM_Stop();
			}
		} else {
			float pos = ( float ) BASS_ChannelBytes2Seconds( stream, posInBytes );
			SM_SetConsoleVarState( pos, loadedSoundPath.c_str(), looping );
		}
	}

	// PAUSED
	{
		if ( isPaused != lastIsPaused ) {
			SM_SetPaused( isPaused > 0 );
			lastIsPaused = isPaused;
		}
	}

	// LOW PASS FILTER
	{
		if ( !slowmotion_low_pass_cutoff_time_end && slowmotion_low_pass_cutoff_desired_value != slowmotion_low_pass_cutoff_desired_last_value ) {
			slowmotion_low_pass_cutoff_time_end = absoluteTime + slowmotion_effect_change_duration->value;
		}

		if ( slowmotion_low_pass_cutoff_time_end ) {
			SM_ApplyLowPassEffects();
		}
	}

	// PITCH
	{
		if ( slowmotion_pitch_desired_value != slowmotion_pitch_desired_last_value ) {
			SM_ApplyPitchEffects();
		}
	}

	// VOLUME
	{
		if ( MP3Volume->value != MP3Volume_last_value ) {
			float appliedValue = MP3Volume->value;
			if ( appliedValue < 0.0f ) {
				appliedValue = 0.0f;
			}
			if ( appliedValue > 1.0f ) {
				appliedValue = 1.0f;
			}

			BASS_ChannelSlideAttribute( stream, BASS_ATTRIB_VOL, appliedValue, 500 );
			MP3Volume_last_value = appliedValue;
		}
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