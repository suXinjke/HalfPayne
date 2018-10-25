#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"player.h"
#include	"client.h"
#include	"weapons.h"
#include	"cgm_gamerules.h"
#include	"custom_gamemode_config.h"
#include	<algorithm>
#include	<random>
#include	<regex>
#include	"monsters.h"
#include	"gameplay_mod.h"
#include	"util_aux.h"
#include	"../fmt/printf.h"
#include	<thread>
#include	<mutex>
#include	"../twitch/twitch.h"

Twitch *twitch = 0;
std::thread twitch_thread;

// Custom Game Mode Rules

int	gmsgEndActiv = 0;
int	gmsgEndTitle = 0;
int	gmsgEndTime  = 0;
int	gmsgEndScore = 0;
int	gmsgEndStat  = 0;

int	gmsgGLogDeact	= 0;
int	gmsgGLogMsg		= 0;
int	gmsgGLogWDeact	= 0;
int	gmsgGLogWMsg	= 0;
int gmsgTimerPause  = 0;
int gmsgTimerDeact = 0;
int gmsgTimerValue = 0;
int gmsgTimerCheat = 0;

int gmsgCountDeact = 0;
int gmsgCountLen   = 0;
int gmsgCountValue = 0;
int gmsgCountOffse = 0;
int gmsgCountCheat = 0;

int gmsgScoreDeact  = 0;
int gmsgScoreValue	= 0;
int gmsgScoreCheat	= 0;

int gmsgRandModLen = 0;
int gmsgRandModVal = 0;

int gmsgPropModLen = 0;
int gmsgPropModVal = 0;
int gmsgPropModVot = 0;
int gmsgPropModVin = 0;
int gmsgPropModAni = 0;

int gmsgCLabelVal  = 0;
int gmsgCLabelGMod  = 0;

int gmsgSayText2 = 0;

TwitchConnectionStatus twitchStatus = TWITCH_DISCONNECTED;
bool ShouldInitializeTwitch() {
	return
		CVAR_GET_FLOAT( "twitch_integration_random_gameplay_mods_voting" ) > 0.0f ||
		CVAR_GET_FLOAT( "twitch_integration_mirror_chat" ) > 0.0f ||
		CVAR_GET_FLOAT( "twitch_integration_say" ) > 0.0f ||
		CVAR_GET_FLOAT( "twitch_integration_random_kill_messages" ) > 0.0f;
}

// CGameRules were recreated each level change and there were no built-in saving method,
// that means we'd lose config file state on each level change.
//
// This has been changed and now CGameRules state is preseved during CHANGE_LEVEL calls,
// using g_changeLevelOccured like below and see also CWorld::Precache 

extern int g_changeLevelOccured;
extern Intermission g_latestIntermission;

#define COMBO_MULTIPLIER_DECAY_TIME 8.0f;

CCustomGameModeRules::CCustomGameModeRules( CONFIG_TYPE configType ) : config( configType )
{
	configs.push_back( &config );

	if ( !gmsgEndActiv ) {
		gmsgEndActiv = REG_USER_MSG( "EndActiv", 1 );
		gmsgEndTitle = REG_USER_MSG( "EndTitle", -1 );
		gmsgEndTime  = REG_USER_MSG( "EndTime", -1 );
		gmsgEndScore = REG_USER_MSG( "EndScore", -1 );
		gmsgEndStat  = REG_USER_MSG( "EndStat", -1 );


		gmsgGLogDeact = REG_USER_MSG( "GLogDeact", 0 );
		gmsgGLogMsg = REG_USER_MSG( "GLogMsg", -1 );
		gmsgGLogWDeact = REG_USER_MSG( "GLogWDeact", 0 );
		gmsgGLogWMsg = REG_USER_MSG( "GLogWMsg", -1 );
		gmsgTimerPause = REG_USER_MSG( "TimerPause", 1 );

		gmsgTimerDeact = REG_USER_MSG( "TimerDeact", 0 );
		gmsgTimerValue = REG_USER_MSG( "TimerValue", -1 );
		gmsgTimerCheat = REG_USER_MSG( "TimerCheat", 0 );

		gmsgCountDeact = REG_USER_MSG( "CountDeact", 0 );
		gmsgCountLen = REG_USER_MSG( "CountLen", 2 );
		gmsgCountValue = REG_USER_MSG( "CountValue", -1 );
		gmsgCountOffse = REG_USER_MSG( "CountOffse", 4 );
		gmsgCountCheat = REG_USER_MSG( "CountCheat", 0 );

		gmsgScoreDeact = REG_USER_MSG( "ScoreDeact", 0 );
		gmsgScoreValue = REG_USER_MSG( "ScoreValue", 16 );
		gmsgScoreCheat = REG_USER_MSG( "ScoreCheat", 0 );

		gmsgCLabelVal = REG_USER_MSG( "CLabelVal", -1 );
		gmsgCLabelGMod = REG_USER_MSG( "CLabelGMod", -1 );

		gmsgRandModLen = REG_USER_MSG( "RandModLen", 2 );
		gmsgRandModVal = REG_USER_MSG( "RandModVal", -1 );
		
		gmsgPropModLen = REG_USER_MSG( "PropModLen", 2 );
		gmsgPropModVal = REG_USER_MSG( "PropModVal", -1 );
		gmsgPropModVot = REG_USER_MSG( "PropModVot", 6 );
		gmsgPropModVin = REG_USER_MSG( "PropModVin", -1 );
		gmsgPropModAni = REG_USER_MSG( "PropModAni", 0 );

		gmsgSayText2 = REG_USER_MSG( "SayText2", -1 );
	}

	// Difficulty must be initialized separately and here, becuase entities are not yet spawned,
	// and they take some of the difficulty info at spawn (like CWallHealth)

	// Monster entities also have to be fetched at this moment for ClientPrecache.
	const char *configName = CVAR_GET_STRING( "gamemode_config" );
	config.ReadFile( configName );
	RefreshSkillData();

	endMarkersActive = false;

	cheatedMessageSent = false;
	startMapDoesntMatch = false;

	musicSwitchDelay = 0.0f;

	yOffset = 0;
	maxYOffset = 0;

	for ( const auto &spawner : config.entityRandomSpawners ) {
		entityRandomSpawnerControllers.push_back( EntityRandomSpawnerController( spawner ) );
	}

	if ( config.record.time == DEFAULT_TIME ) {
		config.record.time = 0.0f;
	}
}

void CCustomGameModeRules::RestartGame() {
	char mapName[256];
	sprintf( mapName, "%s", config.startMap.c_str() );

	// Change level to [startmap] and queue the 'restart' command
	// for CCustomGameModeRules::PlayerSpawn.
	// Would be better just to execute 'cgm' command directly somehow.
	config.markedForRestart = true;
	CHANGE_LEVEL( mapName, NULL );
}

std::mutex gameLogMutex;
void CCustomGameModeRules::SendGameLogMessage( CBasePlayer *pPlayer, const std::string &message, bool logToConsole ) {
	std::lock_guard<std::mutex> guard( gameLogMutex );

	MESSAGE_BEGIN( MSG_ONE, gmsgGLogMsg, NULL, pPlayer->pev );
		WRITE_STRING( message.c_str() );
		WRITE_LONG( maxYOffset );
	MESSAGE_END();

	if ( logToConsole ) {
		g_engfuncs.pfnServerPrint( message.c_str() );
	}
}

void CCustomGameModeRules::SendGameLogWorldMessage( CBasePlayer *pPlayer, const Vector &location, const std::string &message, const std::string &message2 ) {
	MESSAGE_BEGIN( MSG_ONE, gmsgGLogWMsg, NULL, pPlayer->pev );
		WRITE_COORD( location.x );
		WRITE_COORD( location.y );
		WRITE_COORD( location.z );

		// You cant send TWO strings - they overwrite each other, so split them with |
		WRITE_STRING( ( message + "|" + message2 ).c_str() );
	MESSAGE_END();
}

void CCustomGameModeRules::ApplyStartPositionToEntity( CBaseEntity *entity, const StartPosition &startPosition ) {

	if ( !std::isnan( startPosition.x ) ) {
		entity->pev->origin.x = startPosition.x;
		entity->pev->origin.y = std::isnan( startPosition.y ) ? 0 : startPosition.y;
		entity->pev->origin.z = std::isnan( startPosition.z ) ? 0 : startPosition.z;
	}

	if ( !std::isnan( startPosition.angle ) ) {
		entity->pev->angles[1] = startPosition.angle;
	}

}

void CCustomGameModeRules::PlayerSpawn( CBasePlayer *pPlayer )
{
	if ( config.markedForRestart ) {
		config.markedForRestart = false;
		SERVER_COMMAND( "restart\n" );
		return;
	}

	CHalfLifeRules::PlayerSpawn( pPlayer );

	gameplayMods.Reset();

	gameplayMods.activeGameMode = GAME_MODE_CUSTOM;

	// '\' slashes are getting eaten by ALLOC_STRING? must prevent this by replacing them with '/'
	std::string sanitizedConfigName = config.configName;
	std::transform( sanitizedConfigName.begin(), sanitizedConfigName.end(), sanitizedConfigName.begin(), []( auto &letter ) {
		return letter == '\\' ? '/' : letter;
	} );
	sprintf_s( gameplayMods.activeGameModeConfig, sanitizedConfigName.c_str() );

	for ( auto &pair : config.mods ) {
		auto &mod = pair.second;
		mod.Init( pPlayer );
	}

	pPlayer->SetEvilImpulse101( true );
	for ( const auto &item : config.loadout ) {

		for ( int i = 0 ; i < item.amount ; i++ ) {
			if ( item.name == "all" ) {
				pPlayer->GiveAll( true );
				pPlayer->SetEvilImpulse101( true ); // it was set false by GiveAll
			} else {
				if ( item.name == "item_healthkit" ) {
					pPlayer->TakePainkiller();
				} else if ( item.name == "item_suit" ) {
					pPlayer->pev->weapons |= ( 1 << WEAPON_SUIT );
				} else if ( item.name == "item_longjump" ) {
					pPlayer->m_fLongJump = TRUE;
					g_engfuncs.pfnSetPhysicsKeyValue( pPlayer->edict( ), "slj", "1" );
				} else {
					pPlayer->GiveNamedItem( allowedItems[CustomGameModeConfig::GetAllowedItemIndex( item.name.c_str( ) )], true );
				}
			}
		}
	}
	pPlayer->SetEvilImpulse101( false );
	pPlayer->loadoutReceived = true;

	if (
		config.IsGameplayModActive( GAMEPLAY_MOD_EMPTY_SLOWMOTION ) ||
		config.IsGameplayModActive( GAMEPLAY_MOD_NO_SLOWMOTION )
	) {
		pPlayer->slowMotionCharge = 0;
	}

	if ( config.startPosition.defined ) {
		ApplyStartPositionToEntity( pPlayer, config.startPosition );
	}

	bool spawningAfterIntermission = false;
	if ( g_latestIntermission.startPos.defined ) {
		ApplyStartPositionToEntity( pPlayer, g_latestIntermission.startPos );

		if ( g_latestIntermission.startPos.stripped ) {
			pPlayer->RemoveAllItems( false );
		}

		spawningAfterIntermission = true;
		g_latestIntermission.startPos.defined = false;
	}

	if ( config.musicPlaylist.size() > 0 ) {
		gameplayMods.noMapMusic = TRUE;
	}

	if ( config.hasEndMarkers || !config.endConditions.empty() ) {
		gameplayMods.noSaving = TRUE;
	}

	sprintf_s( gameplayMods.activeGameModeConfigHash, config.sha1.c_str() );
	
	// Do not let player cheat by not starting at the [startmap]
	if ( !spawningAfterIntermission ) {
		const char *actualMap = STRING( gpGlobals->mapname );
		startMapDoesntMatch = config.startMap != actualMap;
	}

	if ( !twitch ) {
		twitch = new Twitch();
		twitch->OnConnected = [this] {
			if ( CBasePlayer *pPlayer = dynamic_cast< CBasePlayer* >( CBasePlayer::Instance( g_engfuncs.pfnPEntityOfEntIndex( 1 ) ) ) ) {
				SendGameLogMessage( pPlayer, "Connected to Twitch chat", true );
				twitchStatus = TWITCH_CONNECTED;
			}
		};
		twitch->OnDisconnected = [this] {
			if ( CBasePlayer *pPlayer = dynamic_cast< CBasePlayer* >( CBasePlayer::Instance( g_engfuncs.pfnPEntityOfEntIndex( 1 ) ) ) ) {
				SendGameLogMessage( pPlayer, "Disconnected from Twitch chat", true );
				twitchStatus = TWITCH_DISCONNECTED;
			}
		};
		twitch->OnError = [this]( int errorCode, const std::string &error ) {
			if ( CBasePlayer *pPlayer = dynamic_cast< CBasePlayer* >( CBasePlayer::Instance( g_engfuncs.pfnPEntityOfEntIndex( 1 ) ) ) ) {
				if ( errorCode != -1 ) {
					SendGameLogMessage( pPlayer, fmt::sprintf( "Twitch chat error %d: %s", errorCode, error ).c_str(), true );
				} else {
					SendGameLogMessage( pPlayer, fmt::sprintf( "Twitch chat error: %s", error ).c_str(), true );
				}
			}
		};

		twitch->OnMessage = [this]( const std::string &sender, const std::string &message ) {
			if ( CBasePlayer *pPlayer = dynamic_cast< CBasePlayer* >( CBasePlayer::Instance( g_engfuncs.pfnPEntityOfEntIndex( 1 ) ) ) ) {
				if ( gameplayMods.AllowedToVoteOnRandomGameplayMods() ) {
					VoteForRandomGameplayMod( pPlayer, sender, message );
				}

				if ( CVAR_GET_FLOAT( "twitch_integration_mirror_chat" ) >= 1.0f ) {
					std::string trimmedMessage = message.substr( 0, 192 - sender.size() - 2 );
					MESSAGE_BEGIN( MSG_ONE, gmsgSayText2, NULL, pPlayer->pev );
						WRITE_STRING( ( sender + "|" + trimmedMessage ).c_str() );
					MESSAGE_END();
				}
			}
		};
	}
}

BOOL CCustomGameModeRules::CanHavePlayerItem( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon )
{
	if ( !gameplayMods.weaponRestricted ) {
		return CHalfLifeRules::CanHavePlayerItem( pPlayer, pWeapon );
	}

	if ( !pPlayer->HasWeapons() || !pPlayer->loadoutReceived ) {
		return CHalfLifeRules::CanHavePlayerItem( pPlayer, pWeapon );
	}

	return FALSE;
}

void CCustomGameModeRules::PlayerThink( CBasePlayer *pPlayer )
{
	CHalfLifeRules::PlayerThink( pPlayer );
	
	// This is terribly wrong, it would be better to reset lastGlobalTime on actual change level event
	// It was made to prevent timer messup during level changes, because each level has it's own local time
	float proposedTimeDelta = gpGlobals->time - gameplayMods.lastGlobalTime;
	float timeDelta = fabs( proposedTimeDelta ) <= 0.1 ? ( proposedTimeDelta ) : 0.0f;

	gameplayMods.lastGlobalTime = gpGlobals->time;

	if ( !gameplayMods.timerPaused && pPlayer->pev->deadflag == DEAD_NO ) {
	
		if ( gameplayMods.timerBackwards ) {
			gameplayMods.time -= timeDelta;
		} else {
			gameplayMods.time += timeDelta;
		}

		gameplayMods.realTime += timeDelta / pPlayer->desiredTimeScale;

		if ( pPlayer->slowMotionEnabled ) {
			gameplayMods.secondsInSlowmotion += timeDelta;
		}

		CheckForCheats( pPlayer );
	}

	for ( auto &spawner : entityRandomSpawnerControllers ) {
		spawner.Think( pPlayer );
	}

	size_t musicPlaylistSize = config.musicPlaylist.size();
	if (
		musicPlaylistSize > 0 &&
		CVAR_GET_FLOAT( "sm_current_pos" ) == 0.0f &&
		gpGlobals->time > musicSwitchDelay
	) {
		size_t musicIndexToPlay = pPlayer->currentMusicPlaylistIndex + 1;
		if ( config.musicPlaylistShuffle ) {
			musicIndexToPlay = UniformInt( 0, musicPlaylistSize - 1 );
		}

		if ( musicIndexToPlay >= musicPlaylistSize ) {
			musicIndexToPlay = 0;
		}
		
		pPlayer->SendPlayMusicMessage( config.musicPlaylist.at( musicIndexToPlay ).path );
		pPlayer->currentMusicPlaylistIndex = musicIndexToPlay;

		musicSwitchDelay = gpGlobals->time + 0.2f;
	}

	const int SPACING = 56;
	yOffset = 0;

	if ( gameplayMods.timerShown ) {
		MESSAGE_BEGIN( MSG_ONE, gmsgTimerValue, NULL, pPlayer->pev );
			WRITE_STRING(
				gameplayMods.timerShowReal ? "REAL TIME" :
				gameplayMods.timerBackwards ? "TIME LEFT" :
				"TIME"
			);
			WRITE_FLOAT( gameplayMods.timerShowReal ? gameplayMods.realTime : gameplayMods.time );
			WRITE_LONG( yOffset );
		MESSAGE_END();

		yOffset += SPACING;

		if ( gameplayMods.timerBackwards && gameplayMods.time <= 0.0f && pPlayer->pev->deadflag == DEAD_NO ) {
			ClientKill( ENT( pPlayer->pev ) );
		}
	}

	if ( gameplayMods.scoreAttack ) {
		MESSAGE_BEGIN( MSG_ONE, gmsgScoreValue, NULL, pPlayer->pev );
			WRITE_LONG( gameplayMods.score );
			WRITE_LONG( gameplayMods.comboMultiplier );
			WRITE_FLOAT( gameplayMods.comboMultiplierReset );
			WRITE_LONG( yOffset );
		MESSAGE_END();

		yOffset += SPACING;
	}

	int conditionsHeight = 0;

	MESSAGE_BEGIN( MSG_ONE, gmsgCountLen, NULL, pPlayer->pev );
		WRITE_SHORT( config.endConditions.size() );
	MESSAGE_END();

	for ( size_t i = 0 ; i < config.endConditions.size() ; i++ ) {
		auto &condition = config.endConditions.at( i );

		MESSAGE_BEGIN( MSG_ONE, gmsgCountValue, NULL, pPlayer->pev );
			WRITE_SHORT( i );
			WRITE_LONG( condition.activations );
			WRITE_LONG( condition.activationsRequired );
			WRITE_STRING( condition.objective.c_str() );
		MESSAGE_END();

		conditionsHeight += condition.activationsRequired > 1 ? SPACING : SPACING - 34;
	}
	
	MESSAGE_BEGIN( MSG_ONE, gmsgCountOffse, NULL, pPlayer->pev );
		WRITE_LONG( yOffset );
	MESSAGE_END();

	yOffset += conditionsHeight;
	maxYOffset = max( yOffset, maxYOffset );

	if ( gameplayMods.scoreAttack ) {
		gameplayMods.comboMultiplierReset -= timeDelta;
		if ( gameplayMods.comboMultiplierReset < 0.0f ) {
			gameplayMods.comboMultiplierReset = 0.0f;
			gameplayMods.comboMultiplier = 1;
		}
	}

	if ( gameplayMods.randomGameplayMods ) {
		MESSAGE_BEGIN( MSG_ONE, gmsgRandModLen, NULL, pPlayer->pev );
			WRITE_SHORT( gameplayMods.timedGameplayMods.size() );
		MESSAGE_END();

		int index = 0;

		for ( const auto &timedMod : gameplayMods.timedGameplayMods ) {

			MESSAGE_BEGIN( MSG_ONE, gmsgRandModVal, NULL, pPlayer->pev );
				WRITE_SHORT( index );
				WRITE_FLOAT( gameplayMods.timeForRandomGameplayMod );
				WRITE_FLOAT( timedMod.second );
				WRITE_STRING( timedMod.first.name.c_str() );
			MESSAGE_END();

			index++;
		}

		gameplayMods.timeLeftUntilNextRandomGameplayMod -= timeDelta;

		if ( gameplayMods.proposedGameplayMods.size() > 0 && gameplayMods.timeLeftUntilNextRandomGameplayMod < 0.0f ) {
			gameplayMods.timeLeftUntilNextRandomGameplayMod = gameplayMods.timeUntilNextRandomGameplayMod;

			GameplayMod randomGameplayMod;
			if ( std::any_of( gameplayMods.proposedGameplayMods.begin(), gameplayMods.proposedGameplayMods.end(), []( const GameplayMod &mod ) {
				return mod.votes.size() > 0;
			} ) ) {
				int totalVotes = 0;
				int maxVoteCount = 0;
				for ( auto &mod : gameplayMods.proposedGameplayMods ) {
					totalVotes += mod.votes.size();
					maxVoteCount = max( maxVoteCount, mod.votes.size() );
				}

				std::vector<double> voteDistributions;

				if ( std::string( CVAR_GET_STRING( "twitch_integration_random_gameplay_mods_voting_result" ) ) == "most_votes_more_likely" ) {
					for ( size_t i = 0; i < gameplayMods.proposedGameplayMods.size(); i++ ) {
						voteDistributions.push_back( gameplayMods.proposedGameplayMods.at( i ).votes.size() / ( double ) totalVotes );
					}
				} else {
					for ( size_t i = 0; i < gameplayMods.proposedGameplayMods.size(); i++ ) {
						voteDistributions.push_back( gameplayMods.proposedGameplayMods.at( i ).votes.size() == maxVoteCount ? 1.0f : 0.0f );
					}
				}

				int randomIndex = IndexFromDiscreteDistribution( voteDistributions );
				randomGameplayMod = gameplayMods.proposedGameplayMods.at( randomIndex );
			} else {
				randomGameplayMod = RandomFromVector( gameplayMods.proposedGameplayMods );
			}
			
			auto eventResults = randomGameplayMod.Init( pPlayer );

			if ( randomGameplayMod.isEvent ) {
				MESSAGE_BEGIN( MSG_ONE, gmsgCLabelVal, NULL, pPlayer->pev );
					WRITE_STRING( eventResults.first.c_str() );
					WRITE_STRING( eventResults.second.c_str() );
				MESSAGE_END();
			} else {
				gameplayMods.timedGameplayMods.push_back( { randomGameplayMod, gameplayMods.timeForRandomGameplayMod } );
				MESSAGE_BEGIN( MSG_ONE, gmsgCLabelGMod, NULL, pPlayer->pev );
					WRITE_STRING( randomGameplayMod.id.c_str() );
				MESSAGE_END();
			}
		
			gameplayMods.proposedGameplayMods.clear();
			MESSAGE_BEGIN( MSG_ONE, gmsgPropModLen, NULL, pPlayer->pev );
				WRITE_SHORT( 0 );
			MESSAGE_END();

		} else if ( gameplayMods.timeLeftUntilNextRandomGameplayMod < 3.0f && gameplayMods.timeForRandomGameplayMod >= 10.0f ) {
			MESSAGE_BEGIN( MSG_ONE, gmsgPropModAni, NULL, pPlayer->pev );
			MESSAGE_END();
		} 
		
		if ( gameplayMods.proposedGameplayMods.size() == 0 && gameplayMods.timeLeftUntilNextRandomGameplayMod <= gameplayMods.timeForRandomGameplayModVoting ) {
			
			do {
				gameplayMods.proposedGameplayMods = gameplayMods.GetRandomGameplayMod( pPlayer, 3, [this]( const GameplayMod &mod ) -> bool {
					if (
						std::any_of( gameplayMods.timedGameplayMods.begin(), gameplayMods.timedGameplayMods.end(), [&mod]( std::pair<GameplayMod, float> &timedGameplayMod ) {
							return timedGameplayMod.first.id == mod.id;
						} ) ||

						std::any_of( this->config.mods.begin(), this->config.mods.end(), [&mod]( auto &gameplayMod ) {
							return gameplayMod.second.id == mod.id;
						} )
					) {

						return false;
					}

					return true;
				} );
			} while ( gameplayMods.proposedGameplayMods.size() == 0 );

			if ( twitchStatus == TWITCH_CONNECTED && CVAR_GET_FLOAT( "twitch_integration_random_gameplay_mods_voting" ) ) {
				twitch->SendChatMessage( "VOTE FOR NEXT MOD" );
				for ( size_t i = 0; i < gameplayMods.proposedGameplayMods.size(); i++ ) {
					auto &mod = gameplayMods.proposedGameplayMods.at( i );
					twitch->SendChatMessage( fmt::sprintf( "%d: %s", i + 1, mod.name ) );
				}
			}
		}

		if ( gameplayMods.AllowedToVoteOnRandomGameplayMods() ) {
			MESSAGE_BEGIN( MSG_ONE, gmsgPropModLen, NULL, pPlayer->pev );
				WRITE_SHORT( gameplayMods.proposedGameplayMods.size() );
			MESSAGE_END();

			for ( size_t i = 0; i < gameplayMods.proposedGameplayMods.size(); i++ ) {
				MESSAGE_BEGIN( MSG_ONE, gmsgPropModVal, NULL, pPlayer->pev );
					WRITE_SHORT( i );
					WRITE_STRING( gameplayMods.proposedGameplayMods.at( gameplayMods.proposedGameplayMods.size() - 1 - i ).name.c_str() );
				MESSAGE_END();
			}
		}

		for ( auto i = gameplayMods.timedGameplayMods.begin(); i != gameplayMods.timedGameplayMods.end(); ) {
			i->second -= timeDelta;

			if ( i->second <= 0 ) {
				i->first.Deactivate( pPlayer );
				i = gameplayMods.timedGameplayMods.erase( i );
			} else {
				i++;
			}

		}
	}

	if ( ShouldInitializeTwitch() && twitchStatus == TWITCH_DISCONNECTED ) {


		auto twitch_credentials = ReadTwitchCredentialsFromFile();
		auto login = twitch_credentials.first;
		auto password = twitch_credentials.second;
		if ( !login.empty() && !password.empty() ) {
			SendGameLogMessage( pPlayer, "Connecting to Twitch chat...", true );

			twitch_thread = twitch->Connect( login, password );
			twitch_thread.detach();
			twitchStatus = TWITCH_CONNECTING;
		}

	} else if ( !ShouldInitializeTwitch() && TWITCH_CONNECTED ) {
		SendGameLogMessage( pPlayer, "Disconnecting from Twitch chat", true );
		twitch->Disconnect();
	}
}

void CCustomGameModeRules::OnKilledEntityByPlayer( CBasePlayer *pPlayer, CBaseEntity *victim, KILLED_ENTITY_TYPE killedEntity, BOOL isHeadshot, BOOL killedByExplosion, BOOL killedByCrowbar ) {

	gameplayMods.kills++;

	if ( killedByExplosion ) {
		gameplayMods.explosiveKills++;
	} else if ( killedByCrowbar ) {
		gameplayMods.crowbarKills++;
	} else if ( isHeadshot ) {
		gameplayMods.headshotKills++;
	} else if ( killedEntity == KILLED_ENTITY_GRENADE ) {
		gameplayMods.projectileKills++;
	}

	auto teleportOnKillWeapon = std::string( gameplayMods.teleportOnKillWeapon );
	auto activeItem = pPlayer->m_pActiveItem;
	if (
		gameplayMods.teleportOnKill &&
		( ( teleportOnKillWeapon.empty() ) || ( activeItem && std::string( STRING( activeItem->pev->classname ) ) == teleportOnKillWeapon ) )
	) {

		TraceResult tr;
		bool canTeleport = false;
		Vector destination = victim->pev->origin;
		for ( int i = 0; i < 73 && !canTeleport; i++ ) {
			UTIL_TraceHull( destination, destination, dont_ignore_monsters, human_hull, victim->edict(), &tr );
			if ( tr.fAllSolid ) {
				destination.z += 1.0f;
			} else {
				canTeleport = true;
			}
		}

		if ( canTeleport ) {
			UTIL_SetOrigin( pPlayer->pev, destination );
			if ( CBaseMonster *monster = dynamic_cast<CBaseMonster *>( victim ) ) {
				monster->GibMonster();
			}
		}

	}

	if ( twitchStatus == TWITCH_CONNECTED && CVAR_GET_FLOAT( "twitch_integration_random_kill_messages" ) > 0.0f && twitch->killfeedMessages.size() > 0 ) {
		Vector deathPos = victim->pev->origin;
		deathPos.z += victim->pev->size.z + 5.0f;
		auto it = RandomFromContainer( twitch->killfeedMessages.begin(), twitch->killfeedMessages.end() );
		if ( it != twitch->killfeedMessages.end() ) {
			SendGameLogWorldMessage( pPlayer, deathPos, *it );
			twitch->killfeedMessages.erase( it );
		}
	}

	if ( gameplayMods.blackMesaMinute ) {
		CalculateScoreForBlackMesaMinute( pPlayer, victim, killedEntity, isHeadshot, killedByExplosion, killedByCrowbar );
	}

	if ( gameplayMods.scoreAttack && pPlayer->pev->deadflag == DEAD_NO ) {
		CalculateScoreForScoreAttack( pPlayer, victim, killedEntity, isHeadshot, killedByExplosion, killedByCrowbar );
	}

	CHalfLifeRules::OnKilledEntityByPlayer( pPlayer, victim, killedEntity, isHeadshot, killedByExplosion, killedByCrowbar );
}

void CCustomGameModeRules::CalculateScoreForBlackMesaMinute( CBasePlayer *pPlayer, CBaseEntity *victim, KILLED_ENTITY_TYPE killedEntity, BOOL isHeadshot, BOOL killedByExplosion, BOOL killedByCrowbar ) {
	int timeToAdd = 0;
	std::string message;

	if ( killedByExplosion ) {
		timeToAdd = 10;
		message = "EXPLOSION BONUS";
	} else if ( killedByCrowbar ) {
		timeToAdd = 10;
		message = "MELEE BONUS";
	} else if ( isHeadshot ) {
		timeToAdd = 6;
		message = "HEADSHOT BONUS";
	} else if ( killedEntity == KILLED_ENTITY_GRENADE ) {
		timeToAdd = 1;
		message = "PROJECTILE BONUS";
	} else {
		timeToAdd = 5;
		message = "TIME BONUS";
	}

	Vector deathPos = victim->pev->origin;
	deathPos.z += victim->pev->size.z + 5.0f;

	switch ( killedEntity ) {
		case KILLED_ENTITY_SENTRY:
			deathPos = victim->EyePosition() + Vector( 0, 0, 20 );
			break;

		case KILLED_ENTITY_SNIPER:
			deathPos = victim->pev->origin + ( victim->pev->mins + victim->pev->maxs ) * 0.5;
			break;

		case KILLED_ENTITY_NIHILANTH_CRYSTAL:
			deathPos = victim->pev->origin + ( victim->pev->mins + victim->pev->maxs ) * 0.5;
			timeToAdd = 10;
			break;

		case KILLED_ENTITY_GONARCH_SACK:
			timeToAdd = 10;
			break;

		case KILLED_ENTITY_SCIENTIST:
		case KILLED_ENTITY_BARNEY:
			timeToAdd = 0;

		default:
			break;
	}

	IncreaseTime( pPlayer, deathPos, timeToAdd, message.c_str() );
}

void CCustomGameModeRules::CalculateScoreForScoreAttack( CBasePlayer *pPlayer, CBaseEntity *victim, KILLED_ENTITY_TYPE killedEntity, BOOL isHeadshot, BOOL killedByExplosion, BOOL killedByCrowbar ) {
	int scoreToAdd = -1;
	float additionalMultiplier = 0.0f;

	switch ( killedEntity ) {
		case KILLED_ENTITY_BIG_MOMMA:
		case KILLED_ENTITY_GARGANTUA:
		case KILLED_ENTITY_ARMORED_VEHICLE:
		case KILLED_ENTITY_ICHTYOSAUR:
		case KILLED_ENTITY_NIHILANTH_CRYSTAL:
		case KILLED_ENTITY_GONARCH_SACK:
			scoreToAdd = 1000;
			break;

		case KILLED_ENTITY_ALIEN_CONTROLLER:
		case KILLED_ENTITY_ALIEN_GRUNT:
		case KILLED_ENTITY_HUMAN_ASSASSIN:
		case KILLED_ENTITY_HUMAN_GRUNT:
		case KILLED_ENTITY_SNIPER:
		case KILLED_ENTITY_ALIEN_SLAVE:
			scoreToAdd = 100;
			break;

		case KILLED_ENTITY_MINITURRET:
		case KILLED_ENTITY_SENTRY:
			scoreToAdd = 50;
			break;

		case KILLED_ENTITY_BULLSQUID:
		case KILLED_ENTITY_ZOMBIE:
			scoreToAdd = 30;
			break;

		case KILLED_ENTITY_HEADCRAB:
		case KILLED_ENTITY_HOUNDEYE:
		case KILLED_ENTITY_SNARK:
			scoreToAdd = 20;
			break;

		case KILLED_ENTITY_BARNACLE:
			scoreToAdd = 5;
			break;

		case KILLED_ENTITY_BABYCRAB:
		case KILLED_ENTITY_GRENADE:
			scoreToAdd = 0;
			break;

		default:
			return;
	}

	std::string message;

	if ( killedByExplosion ) {
		message = "EXPLOSION BONUS";
		additionalMultiplier = 1.0f;
	} else if ( killedByCrowbar ) {
		message = "MELEE BONUS";
		additionalMultiplier = 1.0f;
	} else if ( isHeadshot ) {
		message = "HEADSHOT BONUS";
		additionalMultiplier = 0.5f;
	} else if ( killedEntity == KILLED_ENTITY_GRENADE ) {
		message = "PROJECTILE BONUS";
	} else {
		message = "SCORE BONUS";
	}

	Vector deathPos = victim->pev->origin;
	deathPos.z += victim->pev->size.z + 5.0f;

	switch ( killedEntity ) {
	case KILLED_ENTITY_SENTRY:
		deathPos = victim->EyePosition() + Vector( 0, 0, 20 );
		break;

	case KILLED_ENTITY_SNIPER:
		deathPos = victim->pev->origin + ( victim->pev->mins + victim->pev->maxs ) * 0.5;
		break;

	case KILLED_ENTITY_NIHILANTH_CRYSTAL:
		deathPos = victim->pev->origin + ( victim->pev->mins + victim->pev->maxs ) * 0.5;
		break;

	default:
		break;
	}

	gameplayMods.score += scoreToAdd * ( gameplayMods.comboMultiplier + additionalMultiplier );

	if ( scoreToAdd != -1 ) {

		if ( scoreToAdd > 0 ) {
			SendGameLogMessage( pPlayer, message );

			float comboMultiplier = gameplayMods.comboMultiplier + additionalMultiplier;
			bool comboMultiplierIsInteger = abs( comboMultiplier - std::lround( comboMultiplier ) ) < 0.00000001f;

			char upperString[128];
			if ( comboMultiplierIsInteger ) {
				std::sprintf( upperString, "%d x%.0f", scoreToAdd, comboMultiplier );
			} else {
				std::sprintf( upperString, "%d x%.1f", scoreToAdd, comboMultiplier );
			}

			if ( twitchStatus != TWITCH_CONNECTED || CVAR_GET_FLOAT( "twitch_integration_random_kill_messages" ) == 0.0f ) {
				if ( gameplayMods.blackMesaMinute ) {
					SendGameLogWorldMessage( pPlayer, deathPos, "", std::string( upperString ) + " / " + std::to_string( ( int ) ( scoreToAdd * comboMultiplier ) ) );
				} else {
					SendGameLogWorldMessage( pPlayer, deathPos, std::to_string( ( int ) ( scoreToAdd * comboMultiplier ) ), upperString );
				}
			}
		}

		switch ( killedEntity ) {
			case KILLED_ENTITY_BABYCRAB:
			case KILLED_ENTITY_BARNACLE:
			case KILLED_ENTITY_GRENADE:
				// don't add to combo multiplier
				break;

			default:
				gameplayMods.comboMultiplier++;
				break;
		}

		gameplayMods.comboMultiplierReset = COMBO_MULTIPLIER_DECAY_TIME;
	}
}

void CCustomGameModeRules::VoteForRandomGameplayMod( CBasePlayer *pPlayer, const std::string &voter, int modIndex ) {
	if ( modIndex < 0 || modIndex > gameplayMods.proposedGameplayMods.size() - 1 ) {
		return;
	}

	for ( auto &proposedMod : gameplayMods.proposedGameplayMods ) {
		proposedMod.votes.erase( voter );
	}

	gameplayMods.proposedGameplayMods.at( modIndex ).votes.insert( voter );

	for ( size_t i = 0; i < gameplayMods.proposedGameplayMods.size(); i++ ) {
		MESSAGE_BEGIN( MSG_ONE, gmsgPropModVot, NULL, pPlayer->pev );
			WRITE_SHORT( i );
			WRITE_LONG( gameplayMods.proposedGameplayMods.at( gameplayMods.proposedGameplayMods.size() - 1 - i ).votes.size() );
		MESSAGE_END();
	}

	MESSAGE_BEGIN( MSG_ONE, gmsgPropModVin, NULL, pPlayer->pev );
		WRITE_SHORT( gameplayMods.proposedGameplayMods.size() - 1 - modIndex );
		WRITE_STRING( voter.c_str() );
	MESSAGE_END();
}

void CCustomGameModeRules::VoteForRandomGameplayMod( CBasePlayer *pPlayer, const std::string &voter, const std::string &modStringIndex ) {
	std::regex vote_index_regex( "^[#!0]*([1-9]{1}).*" );
	std::smatch base_match;
	if ( std::regex_match( modStringIndex, base_match, vote_index_regex ) ) {
		if ( base_match.size() == 2 ) {
			int modIndex = std::stoi( base_match[1].str() ) - 1;
			VoteForRandomGameplayMod( pPlayer, voter, modIndex );
		}
	}
}

void CCustomGameModeRules::ActivateEndMarkers( CBasePlayer *pPlayer ) {
	if ( !endMarkersActive ) {
		endMarkersActive = true;
		if ( pPlayer ) {
			EMIT_SOUND( ENT( pPlayer->pev ), CHAN_STATIC, "var/end_marker.wav", 1, ATTN_NORM, true );
			SendGameLogMessage( pPlayer, "HEAD TO THE GOAL" );
		}
	}

	tasks.push_back( { 0.0f, []( CBasePlayer *pPlayer ) {
		CBaseEntity *pEntity = NULL;
		while ( ( pEntity = UTIL_FindEntityInSphere( pEntity, Vector( 0, 0, 0 ), 8192 ) ) != NULL ) {
			if ( FStrEq( STRING( pEntity->pev->classname ), "end_marker" ) ) {
				pEntity->Use( NULL, NULL, USE_ON, 0.0f );
			}
		}
	} } );
}

void CCustomGameModeRules::CheckForCheats( CBasePlayer *pPlayer )
{
	if ( gameplayMods.cheated && cheatedMessageSent || ended ) {
		return;
	}

	if ( gameplayMods.cheated ) {
		OnCheated( pPlayer );
		return;
	}

	if ( ( pPlayer->pev->flags & FL_GODMODE && !gameplayMods.godConstant ) ||
		 ( pPlayer->pev->flags & FL_NOTARGET && !gameplayMods.noTargetConstant ) ||
		 ( pPlayer->pev->movetype & MOVETYPE_NOCLIP ) ||
		gameplayMods.usedCheat || startMapDoesntMatch ||
		 std::string( gameplayMods.activeGameModeConfigHash ) != config.sha1
	) {
		SendGameLogMessage( pPlayer, "YOU'VE BEEN CHEATING - RESULTS WON'T BE SAVED" );
		gameplayMods.cheated = true;
	}

}

void CCustomGameModeRules::OnCheated( CBasePlayer *pPlayer ) {
	cheatedMessageSent = true;

	MESSAGE_BEGIN( MSG_ONE, gmsgTimerCheat, NULL, pPlayer->pev );
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_ONE, gmsgCountCheat, NULL, pPlayer->pev );
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_ONE, gmsgScoreCheat, NULL, pPlayer->pev );
	MESSAGE_END();
}

void CCustomGameModeRules::OnEnd( CBasePlayer *pPlayer ) {
	PauseTimer( pPlayer );

	if ( !config.gameFinishedOnce && gameplayMods.timerBackwards ) {
		config.record.time = 0.0f;
	}

	MESSAGE_BEGIN( MSG_ONE, gmsgGLogDeact, NULL, pPlayer->pev );
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_ONE, gmsgGLogWDeact, NULL, pPlayer->pev );
	MESSAGE_END();
	
	MESSAGE_BEGIN( MSG_ONE, gmsgEndTitle, NULL, pPlayer->pev );
		WRITE_STRING( config.ConfigTypeToGameModeName( true ).c_str() );
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_ONE, gmsgEndTitle, NULL, pPlayer->pev );
		WRITE_STRING(
			config.name.empty() ?
			"LEVEL COMPLETE" :
			( config.name + " - COMPLETE" ).c_str()
		);
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_ONE, gmsgTimerDeact, NULL, pPlayer->pev );
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_ONE, gmsgEndTime, NULL, pPlayer->pev );
		WRITE_STRING( "TIME SCORE|PERSONAL BESTS" );
		WRITE_FLOAT( gameplayMods.time );
		WRITE_FLOAT( config.record.time );
		WRITE_BYTE( gameplayMods.timerBackwards ? gameplayMods.time > config.record.time : gameplayMods.time < config.record.time  );
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_ONE, gmsgEndTime, NULL, pPlayer->pev );
		WRITE_STRING( "REAL TIME" );
		WRITE_FLOAT( gameplayMods.realTime );
		WRITE_FLOAT( config.record.realTime );
		WRITE_BYTE( gameplayMods.realTime < config.record.realTime );
	MESSAGE_END();

	if ( gameplayMods.timerBackwards ) {
		float realTimeMinusTime = max( 0.0f, gameplayMods.realTime - gameplayMods.time );
		MESSAGE_BEGIN( MSG_ONE, gmsgEndTime, NULL, pPlayer->pev );
			WRITE_STRING( "REAL TIME MINUS SCORE" );
			WRITE_FLOAT( realTimeMinusTime );
			WRITE_FLOAT( config.record.realTimeMinusTime );
			WRITE_BYTE( realTimeMinusTime < config.record.realTimeMinusTime );
		MESSAGE_END();
	}

	int secondsInSlowmotion = ( int ) roundf( gameplayMods.secondsInSlowmotion );
	if ( secondsInSlowmotion > 0 ) {
		MESSAGE_BEGIN( MSG_ONE, gmsgEndStat, NULL, pPlayer->pev );
			WRITE_STRING( ( "SECONDS IN SLOWMOTION|" + std::to_string( secondsInSlowmotion ) ).c_str() );
		MESSAGE_END();
	}

	if ( gameplayMods.kills > 0 ) {
		MESSAGE_BEGIN( MSG_ONE, gmsgEndStat, NULL, pPlayer->pev );
			WRITE_STRING( ( "TOTAL KILLS|" + std::to_string( gameplayMods.kills ) ).c_str() );
		MESSAGE_END();
	}

	if ( gameplayMods.headshotKills > 0 ) {
		MESSAGE_BEGIN( MSG_ONE, gmsgEndStat, NULL, pPlayer->pev );
			WRITE_STRING( ( "HEADSHOT KILLS|" + std::to_string( gameplayMods.headshotKills ) ).c_str() );
		MESSAGE_END();
	}

	if ( gameplayMods.explosiveKills > 0 ) {
		MESSAGE_BEGIN( MSG_ONE, gmsgEndStat, NULL, pPlayer->pev );
			WRITE_STRING( ( "EXPLOSION KILLS|" + std::to_string( gameplayMods.explosiveKills ) ).c_str() );
		MESSAGE_END();
	}

	if ( gameplayMods.crowbarKills > 0 ) {
		MESSAGE_BEGIN( MSG_ONE, gmsgEndStat, NULL, pPlayer->pev );
			WRITE_STRING( ( "MELEE KILLS|" + std::to_string( gameplayMods.crowbarKills ) ).c_str() );
		MESSAGE_END();
	}

	if ( gameplayMods.projectileKills > 0 ) {
		MESSAGE_BEGIN( MSG_ONE, gmsgEndStat, NULL, pPlayer->pev );
			WRITE_STRING( ( "PROJECTILES DESTROYED KILLS|" + std::to_string( gameplayMods.projectileKills ) ).c_str() );
		MESSAGE_END();
	}

	if ( gameplayMods.scoreAttack ) {
		MESSAGE_BEGIN( MSG_ONE, gmsgScoreDeact, NULL, pPlayer->pev );
		MESSAGE_END();

		MESSAGE_BEGIN( MSG_ONE, gmsgEndScore, NULL, pPlayer->pev );
			WRITE_STRING( "SCORE|PERSONAL BEST" );
			WRITE_LONG( gameplayMods.score );
			WRITE_LONG( config.record.score );
			WRITE_BYTE( gameplayMods.score > config.record.score );
		MESSAGE_END();
	}

	MESSAGE_BEGIN( MSG_ONE, gmsgEndActiv, NULL, pPlayer->pev );
		WRITE_BYTE( gameplayMods.cheated );
	MESSAGE_END();

	if ( !gameplayMods.cheated ) {
		config.record.Save( pPlayer );
	}
}

void CCustomGameModeRules::OnHookedModelIndex( CBasePlayer *pPlayer, CBaseEntity *activator, int modelIndex, const std::string &className, const std::string &targetName, bool firstTime )
{
	CHalfLifeRules::OnHookedModelIndex( pPlayer, activator, modelIndex, className, targetName, firstTime );
	
	if ( config.endTrigger.Fits( modelIndex, className, targetName, firstTime ) ) {
		End( pPlayer );
	}

	for ( const auto &potentialIntermission : config.intermissions ) {
		if ( potentialIntermission.Fits( modelIndex, className, targetName, firstTime ) ) {
			g_latestIntermission = potentialIntermission;
			CHANGE_LEVEL( ( char * ) g_latestIntermission.entityName.c_str(), NULL );
			// after that, g_latestIntermission becomes undefined in PlayerSpawn function
			break;
		}
	}

	if (
		config.IsGameplayModActive( GAMEPLAY_MOD_SNARK_PENGUINS ) && 
		config.IsGameplayModActive( GAMEPLAY_MOD_SNARK_FRIENDLY_TO_PLAYER ) && 
		config.IsGameplayModActive( GAMEPLAY_MOD_SNARK_FRIENDLY_TO_ALLIES ) &&
		std::string( STRING( gpGlobals->mapname ) ) == "c1a0e"
	) {
		CBaseEntity *list[512];
		int amount = UTIL_MonstersInSphere( list, 512, Vector( 0, 0, 0 ), 8192.0f );

		if ( targetName == "se_motor_sound" ) {
			
			for ( int i = 0 ; i < amount ; i++ ) {
				if ( CSqueakGrenade *penguin = dynamic_cast<CSqueakGrenade *>( list[i] ) ) {
					if ( !penguin->isPanic ) {
						penguin->pev->velocity = Vector( 0, 0, 0 );
						penguin->pev->angles = UTIL_VecToAngles( activator->pev->origin - penguin->pev->origin );
						penguin->pev->angles[0] = 0;
						penguin->pev->sequence = 1;
						penguin->isStill = true;
					}
				}
			}
		} else if ( targetName == "speaker_ohno" ) {

			for ( int i = 0 ; i < amount ; i++ ) {
				if ( CSqueakGrenade *penguin = dynamic_cast<CSqueakGrenade *>( list[i] ) ) {
					penguin->pev->sequence = 2;
					penguin->isStill = false;
					penguin->isPanic = true;
				}
			}
		}
	}

	if ( targetName == "on_map_start" && gameplayMods.preventMonsterSpawn ) {

		for ( int i = 0 ; i < 1024 ; i++ ) {
			edict_t *edict = g_engfuncs.pfnPEntityOfEntIndex( i );
			if ( !edict ) {
				continue;
			}

			if ( CBaseEntity *entity = CBaseEntity::Instance( edict ) ) {
				if ( entity->pev->spawnflags & SF_MONSTER_PRESERVE ) {
					continue;
				}

				if ( std::string( STRING( entity->pev->classname ) ).find( "monster_" ) == 0 ) {
					entity->pev->flags |= FL_KILLME;
				}
			}
		}
	}

	for ( const auto &timerPause : config.timerPauses ) {
		if ( timerPause.Fits( modelIndex, className, targetName, firstTime ) ) {
			PauseTimer( pPlayer );
		}
	}

	for ( const auto &timerResume : config.timerResumes ) {
		if ( timerResume.Fits( modelIndex, className, targetName, firstTime ) ) {
			ResumeTimer( pPlayer );
		}
	}

	for ( auto &condition : config.endConditions ) {
		if ( condition.Fits( modelIndex, className, targetName, firstTime ) ) {
			condition.activations++;

			if ( condition.activations >= condition.activationsRequired ) {

				bool allComplete = true;

				for ( const auto &condition : config.endConditions ) {
					if ( condition.activations < condition.activationsRequired ) {
						allComplete = false;
						break;
					}
				}

				if ( allComplete ) {
					if ( config.hasEndMarkers ) {
						ActivateEndMarkers( pPlayer );
					} else {
						End( pPlayer );
					}
				}
			}
		}
	}

	for ( const auto &teleporterRedirect : config.teleports ) {
		if ( teleporterRedirect.Fits( modelIndex, className, targetName, true ) ) {
			ApplyStartPositionToEntity( pPlayer, teleporterRedirect.pos );
		}
	}

}

void CCustomGameModeRules::OnChangeLevel() {
	CHalfLifeRules::OnChangeLevel();
	musicSwitchDelay = 0.0f;

	for ( auto &spawner : entityRandomSpawnerControllers ) {
		spawner.ResetSpawnPeriod();
	}

	if ( endMarkersActive ) {
		ActivateEndMarkers();
	}

	if ( gameplayMods.proposedGameplayMods.size() > 0 ) {
		tasks.push_back( { 0.0f, []( CBasePlayer *pPlayer ) {
			for ( auto mod = gameplayMods.proposedGameplayMods.begin(); mod != gameplayMods.proposedGameplayMods.end(); ) {
				if ( mod->canBeCancelledAfterChangeLevel && !mod->CanBeActivatedRandomly( pPlayer ) ) {
					mod = gameplayMods.proposedGameplayMods.erase( mod );
				} else {
					mod++;
				}
			}
		} } );
	}
}

extern int g_autoSaved;
void CCustomGameModeRules::OnSave( CBasePlayer *pPlayer ) {
	if ( gameplayMods.noSaving && !g_autoSaved ) {
		SendGameLogMessage( pPlayer, "SAVING IS ACTUALLY DISABLED" );
	}
	CHalfLifeRules::OnSave( pPlayer );
}

void CCustomGameModeRules::PauseTimer( CBasePlayer *pPlayer )
{
	if ( gameplayMods.timerPaused ) {
		return;
	}

	gameplayMods.timerPaused = true;

	MESSAGE_BEGIN( MSG_ONE, gmsgTimerPause, NULL, pPlayer->pev );
		WRITE_BYTE( true );
	MESSAGE_END();
}

void CCustomGameModeRules::ResumeTimer( CBasePlayer *pPlayer )
{
	if ( !gameplayMods.timerPaused ) {
		return;
	}

	gameplayMods.timerPaused = false;

	MESSAGE_BEGIN( MSG_ONE, gmsgTimerPause, NULL, pPlayer->pev );
		WRITE_BYTE( false );
	MESSAGE_END();
}

void CCustomGameModeRules::IncreaseTime( CBasePlayer *pPlayer, const Vector &eventPos, int timeToAdd, const char *message ) {
	if ( gameplayMods.timerPaused || timeToAdd <= 0 ) {
		return;
	}

	gameplayMods.time += timeToAdd;

	SendGameLogMessage( pPlayer, message );

	char timeAddedCString[6];
	sprintf( timeAddedCString, "00:%02d", timeToAdd ); // mm:ss
	const std::string timeAddedString = std::string( timeAddedCString );

	if ( twitchStatus != TWITCH_CONNECTED || CVAR_GET_FLOAT( "twitch_integration_random_kill_messages" ) == 0.0f ) {
		SendGameLogWorldMessage( pPlayer, eventPos, timeAddedString );
	}
}

// Hardcoded values so it won't depend on console variables
void CCustomGameModeRules::RefreshSkillData() 
{
	gSkillData.barneyHealth = 35;
	gSkillData.slaveDmgClawrake = 25.0f;

	gSkillData.leechHealth = 2.0f;
	gSkillData.leechDmgBite = 2.0f;

	gSkillData.scientistHealth = 20.0f;

	gSkillData.snarkHealth = 2.0f;
	gSkillData.snarkDmgBite = 10.0f;
	gSkillData.snarkDmgPop = 5.0f;

	gSkillData.plrDmgCrowbar = 10.0f;
	gSkillData.plrDmg9MM = 8.0f;
	gSkillData.plrDmg357 = 40.0f;
	gSkillData.plrDmgMP5 = 5.0f;
	gSkillData.plrDmgM203Grenade = 100.0f;
	gSkillData.plrDmgBuckshot = 5.0f;
	gSkillData.plrDmgCrossbowClient = 10.0f;
	gSkillData.plrDmgCrossbowMonster = 50.0f;
	gSkillData.plrDmgRPG = 100.0f;
	gSkillData.plrDmgGauss = 20.0f;
	gSkillData.plrDmgEgonNarrow = 6.0f;
	gSkillData.plrDmgEgonWide = 14.0f;
	gSkillData.plrDmgHornet = 7;
	gSkillData.plrDmgHandGrenade = 100.0f;
	gSkillData.plrDmgSatchel = 150.0f;
	gSkillData.plrDmgTripmine = 150.0f;

	gSkillData.healthkitCapacity = 15.0f; // doesn't matter - it's painkiller
	gSkillData.scientistHeal = 25.0f;

	if ( config.IsGameplayModActive( GAMEPLAY_MOD_HEADSHOTS ) ) {
		gSkillData.monHead = 10.0f;
	} else {
		gSkillData.monHead = 3.0f;
	}
	gSkillData.monChest = 1.0f;
	gSkillData.monStomach = 1.0f;
	gSkillData.monLeg = 1.0f;
	gSkillData.monArm = 1.0f;

	gSkillData.plrHead = 3.0f;
	gSkillData.plrChest = 1.0f;
	gSkillData.plrStomach = 1.0f;
	gSkillData.plrLeg = 1.0f;
	gSkillData.plrArm = 1.0f;

	if ( config.IsGameplayModActive( GAMEPLAY_MOD_EASY ) ) {
		
		gSkillData.iSkillLevel = 1;

		gSkillData.agruntHealth = 60.0f;
		gSkillData.agruntDmgPunch = 10.0f;

		gSkillData.apacheHealth = 150.0f;
	
		gSkillData.bigmommaHealthFactor = 1.0f;
		gSkillData.bigmommaDmgSlash = 50.0f;
		gSkillData.bigmommaDmgBlast = 100.0f;
		gSkillData.bigmommaRadiusBlast = 250.0f;

		gSkillData.bullsquidHealth = 40.0f;
		gSkillData.bullsquidDmgBite = 15.0f;
		gSkillData.bullsquidDmgWhip = 25.0f;
		gSkillData.bullsquidDmgSpit = 10.0f;

		gSkillData.gargantuaHealth = 800.0f;
		gSkillData.gargantuaDmgSlash = 10.0f;
		gSkillData.gargantuaDmgFire = 3.0f;
		gSkillData.gargantuaDmgStomp = 50.0f;

		gSkillData.hassassinHealth = 30.0f;

		gSkillData.headcrabHealth = 10.0f;
		gSkillData.headcrabDmgBite = 5.0f;

		gSkillData.hgruntHealth = 50.0f;
		gSkillData.hgruntDmgKick = 5.0f;
		gSkillData.hgruntShotgunPellets = 3.0f;
		gSkillData.hgruntGrenadeSpeed = 400.0f;

		gSkillData.houndeyeHealth = 20.0f;
		gSkillData.houndeyeDmgBlast = 10.0f;

		gSkillData.slaveHealth = 30.0f;
		gSkillData.slaveDmgClaw = 8.0f;
		gSkillData.slaveDmgZap = 10.0f;

		gSkillData.ichthyosaurHealth = 200.0f;
		gSkillData.ichthyosaurDmgShake = 20.0f;

		gSkillData.controllerHealth = 60.0f;
		gSkillData.controllerDmgZap = 15.0f;
		gSkillData.controllerSpeedBall = 650.0f;
		gSkillData.controllerDmgBall = 3.0f;

		gSkillData.nihilanthHealth = 800.0f;
		gSkillData.nihilanthZap = 30.0f;
	
		gSkillData.zombieHealth = 50.0f;
		gSkillData.zombieDmgOneSlash = 10.0f;
		gSkillData.zombieDmgBothSlash = 25.0f;

		gSkillData.turretHealth = 50.0f;
		gSkillData.miniturretHealth = 40.0f;
		gSkillData.sentryHealth = 40.0f;

		gSkillData.monDmg12MM = 8.0f;
		gSkillData.monDmgMP5 = 3.0f;
		gSkillData.monDmg9MM = 5.0f;
		
		gSkillData.monDmgHornet = 4.0f;

		gSkillData.suitchargerCapacity = 75.0f;
		gSkillData.batteryCapacity = 15.0f;
		gSkillData.healthchargerCapacity = 50.0f;
		
	} else if ( config.IsGameplayModActive( GAMEPLAY_MOD_HARD ) ) {
		gSkillData.iSkillLevel = 3;

		gSkillData.agruntHealth = 120.0f;
		gSkillData.agruntDmgPunch = 20.0f;

		gSkillData.apacheHealth = 400.0f;
	
		gSkillData.bigmommaHealthFactor = 2.0f;
		gSkillData.bigmommaDmgSlash = 70.0f;
		gSkillData.bigmommaDmgBlast = 160.0f;
		gSkillData.bigmommaRadiusBlast = 275.0f;

		gSkillData.bullsquidHealth = 40.0f;
		gSkillData.bullsquidDmgBite = 25.0f;
		gSkillData.bullsquidDmgWhip = 35.0f;
		gSkillData.bullsquidDmgSpit = 10.0f;

		gSkillData.gargantuaHealth = 1000.0f;
		gSkillData.gargantuaDmgSlash = 30.0f;
		gSkillData.gargantuaDmgFire = 5.0f;
		gSkillData.gargantuaDmgStomp = 100.0f;

		gSkillData.hassassinHealth = 50.0f;

		gSkillData.headcrabHealth = 20.0f;
		gSkillData.headcrabDmgBite = 10.0f;

		gSkillData.hgruntHealth = 80.0f;
		gSkillData.hgruntDmgKick = 10.0f;
		gSkillData.hgruntShotgunPellets = 6.0f;
		gSkillData.hgruntGrenadeSpeed = 800.0f;

		gSkillData.houndeyeHealth = 30.0f;
		gSkillData.houndeyeDmgBlast = 15.0f;

		gSkillData.slaveHealth = 60.0f;
		gSkillData.slaveDmgClaw = 10.0f;
		gSkillData.slaveDmgZap = 15.0f;

		gSkillData.ichthyosaurHealth = 400.0f;
		gSkillData.ichthyosaurDmgShake = 50.0f;

		gSkillData.controllerHealth = 100.0f;
		gSkillData.controllerDmgZap = 35.0f;
		gSkillData.controllerSpeedBall = 1000.0f;
		gSkillData.controllerDmgBall = 5.0f;

		gSkillData.nihilanthHealth = 1000.0f;
		gSkillData.nihilanthZap = 50.0f;
	
		gSkillData.zombieHealth = 100.0f;
		gSkillData.zombieDmgOneSlash = 20.0f;
		gSkillData.zombieDmgBothSlash = 40.0f;

		gSkillData.turretHealth = 60.0f;
		gSkillData.miniturretHealth = 50.0f;
		gSkillData.sentryHealth = 50.0f;

		gSkillData.monDmg12MM = 10.0f;
		gSkillData.monDmgMP5 = 5.0f;
		gSkillData.monDmg9MM = 8.0f;
		
		gSkillData.monDmgHornet = 8.0f;

		gSkillData.suitchargerCapacity = 35.0f;
		gSkillData.batteryCapacity = 10.0f;
		gSkillData.healthchargerCapacity = 25.0f;
	} else {
		gSkillData.iSkillLevel = 2;

		gSkillData.agruntHealth = 90.0f;
		gSkillData.agruntDmgPunch = 20.0f;

		gSkillData.apacheHealth = 250.0f;

		gSkillData.bigmommaHealthFactor = 1.5f;
		gSkillData.bigmommaDmgSlash = 60.0f;
		gSkillData.bigmommaDmgBlast = 120.0f;
		gSkillData.bigmommaRadiusBlast = 250.0f;

		gSkillData.bullsquidHealth = 40.0f;
		gSkillData.bullsquidDmgBite = 25.0f;
		gSkillData.bullsquidDmgWhip = 35.0f;
		gSkillData.bullsquidDmgSpit = 10.0f;

		gSkillData.gargantuaHealth = 800.0f;
		gSkillData.gargantuaDmgSlash = 30.0f;
		gSkillData.gargantuaDmgFire = 5.0f;
		gSkillData.gargantuaDmgStomp = 100.0f;

		gSkillData.hassassinHealth = 50.0f;

		gSkillData.headcrabHealth = 10.0f;
		gSkillData.headcrabDmgBite = 10.0f;

		gSkillData.hgruntHealth = 50.0f;
		gSkillData.hgruntDmgKick = 10.0f;
		gSkillData.hgruntShotgunPellets = 5.0f;
		gSkillData.hgruntGrenadeSpeed = 600.0f;

		gSkillData.houndeyeHealth = 20.0f;
		gSkillData.houndeyeDmgBlast = 15.0f;

		gSkillData.slaveHealth = 30.0f;
		gSkillData.slaveDmgClaw = 10.0f;
		gSkillData.slaveDmgZap = 10.0f;

		gSkillData.ichthyosaurHealth = 200.0f;
		gSkillData.ichthyosaurDmgShake = 35.0f;

		gSkillData.controllerHealth = 60.0f;
		gSkillData.controllerDmgZap = 25.0f;
		gSkillData.controllerSpeedBall = 800.0f;
		gSkillData.controllerDmgBall = 4.0f;

		gSkillData.nihilanthHealth = 800.0f;
		gSkillData.nihilanthZap = 30.0f;

		gSkillData.zombieHealth = 50.0f;
		gSkillData.zombieDmgOneSlash = 20.0f;
		gSkillData.zombieDmgBothSlash = 40.0f;

		gSkillData.turretHealth = 50.0f;
		gSkillData.miniturretHealth = 40.0f;
		gSkillData.sentryHealth = 40.0f;

		gSkillData.monDmg12MM = 10.0f;
		gSkillData.monDmgMP5 = 4.0f;
		gSkillData.monDmg9MM = 5.0f;

		gSkillData.monDmgHornet = 5.0f;

		gSkillData.suitchargerCapacity = 50.0f;
		gSkillData.batteryCapacity = 15.0f;
		gSkillData.healthchargerCapacity = 40.0f;
	}
}

EntityRandomSpawnerController::EntityRandomSpawnerController( const EntityRandomSpawner &entityRandomSpawner ) :
	spawnData( entityRandomSpawner.entity ),
	maxAmount( entityRandomSpawner.maxAmount ),
	spawnPeriod( entityRandomSpawner.spawnPeriod ),
	nextSpawn( gpGlobals->time + entityRandomSpawner.spawnPeriod )
{ }

void EntityRandomSpawnerController::Think( CBasePlayer *pPlayer ) {
	if ( gpGlobals->time > nextSpawn ) {
		Spawn( pPlayer );
		ResetSpawnPeriod();
	}
}

void EntityRandomSpawnerController::ResetSpawnPeriod() {
	nextSpawn = gpGlobals->time + spawnPeriod;
}

void EntityRandomSpawnerController::Spawn( CBasePlayer *pPlayer ) {
	CBaseEntity *list[1024] = { NULL };
	int amountOfMonsters = UTIL_MonstersInSphere( list, 1024, Vector( 0, 0, 0 ), 8192.0f );
	int amountOfMonstersOfThisType = 0;
	for ( int i = 0 ; i < amountOfMonsters ; i++ ) {
		CBaseEntity *entity = list[i];
		if (
			( entity->pev->spawnflags & SF_MONSTER_PRESERVE ) &&
		   !( entity->pev->deadflag & DEAD_DEAD ) &&
			FStrEq( STRING( entity->pev->classname ), spawnData.name.c_str() )
		) {
			amountOfMonstersOfThisType++;
		}
	}

	if ( amountOfMonstersOfThisType >= maxAmount ) {
		return;
	}

	spawnData.DetermineBestSpawnPosition( pPlayer );
	bool spawnPositionDecided = false;
	CCustomGameModeRules::SpawnBySpawnData( spawnData );
}

