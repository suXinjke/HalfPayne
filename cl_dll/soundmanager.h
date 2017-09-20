#ifndef SOUNDMANAGER_H
#define SOUNDMANAGER_H

void SM_Init();

void SM_Play( const char *soundPath, bool looping = false );
void SM_PlayRandomMainMenuMusic();
void SM_SetPaused( bool paused );
bool SM_Stop();
void SM_Seek( double positionSeconds );
void SM_SetPitch( float pitch );
void SM_SetSlowmotion( bool slowmotion );
void SM_ApplyLowPassEffects();
void SM_ApplyPitchEffects();

void SM_Think( double time );

void SM_CheckError();

// Messages
int SM_OnBassPlay( const char *pszName, int iSize, void *pbuf );
int SM_OnBassStop( const char *pszName, int iSize, void *pbuf );
int SM_OnBassSlowmo( const char *pszName, int iSize, void *pbuf );

#endif // SOUNDMANAGER_H