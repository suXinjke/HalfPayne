#include "custom_gamemode_record.h"
#include <fstream>

bool CustomGameModeRecord::Read( const std::string &directoryPath, const std::string &fileName ) {

	this->directoryPath = directoryPath;
	this->filePath = directoryPath + fileName;

	std::ifstream inp( filePath, std::ios::in | std::ios::binary );
	if ( inp.is_open() ) {
		inp.read( ( char * ) &time, sizeof( float ) );
		inp.read( ( char * ) &realTime, sizeof( float ) );
		inp.read( ( char * ) &realTimeMinusTime, sizeof( float ) );
		inp.read( ( char * ) &score, sizeof( int ) );

		inp.read( ( char * ) &secondsInSlowmotion, sizeof( float ) );
		inp.read( ( char * ) &kills, sizeof( int ) );
		inp.read( ( char * ) &headshotKills, sizeof( int ) );
		inp.read( ( char * ) &explosiveKills, sizeof( int ) );
		inp.read( ( char * ) &crowbarKills, sizeof( int ) );
		inp.read( ( char * ) &projectileKills, sizeof( int ) );

		return true;
	} else {
		return false;
	}
}

void CustomGameModeRecord::Save( CBasePlayer *player ) {

	// Create the directory if it's not there. Proceed only when directory exists
	if ( CreateDirectory( directoryPath.c_str(), NULL ) || GetLastError() == ERROR_ALREADY_EXISTS ) {

		float time = player->timerBackwards ?
			( player->time > this->time ? player->time : this->time ) :
			( player->time < this->time ? player->time : this->time );
		float realTime = player->realTime < this->realTime ? player->realTime : this->realTime;
		float realTimeMinusTime = max( 0.0f, realTime - time );
		if ( realTimeMinusTime > this->realTimeMinusTime ) {
			realTimeMinusTime = this->realTimeMinusTime;
		}

		int score = player->score > this->score ? player->score : this->score;

		std::ofstream out( filePath, std::ios::out | std::ios::binary );

		out.write( ( char * ) &time, sizeof( float ) );
		out.write( ( char * ) &realTime, sizeof( float ) );
		out.write( ( char * ) &realTimeMinusTime, sizeof( float ) );
		out.write( ( char * ) &score, sizeof( int ) );

		out.write( ( char * ) &player->secondsInSlowmotion, sizeof( float ) );
		out.write( ( char * ) &player->kills, sizeof( int ) );
		out.write( ( char * ) &player->headshotKills, sizeof( int ) );
		out.write( ( char * ) &player->explosiveKills, sizeof( int ) );
		out.write( ( char * ) &player->crowbarKills, sizeof( int ) );
		out.write( ( char * ) &player->projectileKills, sizeof( int ) );

		out.close();
	}
}