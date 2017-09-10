#ifndef CUSTOM_GAMEMODE_RECORD_H
#define CUSTOM_GAMEMODE_RECORD_H

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"

const float DEFAULT_TIME = 59999.0f; // 999:59.00

class CustomGameModeRecord {
public:

	std::string directoryPath;
	std::string filePath;

	bool Read( const std::string &directoryPath, const std::string &fileName );
	void Save( CBasePlayer *player );

	float time = DEFAULT_TIME;
	float realTime = DEFAULT_TIME;
	float realTimeMinusTime = DEFAULT_TIME;
	int score = 0;

	float secondsInSlowmotion = 0.0f;
	int kills = 0;
	int headshotKills = 0;
	int explosiveKills = 0;
	int crowbarKills = 0;
	int projectileKills = 0;

};

#endif // CUSTOM_GAMEMODE_RECORD_H