#include <map>
#include <random>
#include "gameplay_mod.h"
#include "player.h"
#include "cgm_gamerules.h"
GameplayModData gameplayModsData;

#ifdef CLIENT_DLL
#include "parsemsg.h"
int g_inverseControls = 0;
int g_musicSlowmotion = 0;
CustomGameModeConfig clientConfig( CONFIG_TYPE_CGM );
#else
int gmsgGmplayMod = 0;
int gmsgGmplayCfg = 0;

int gmsgGmplayFDC = 0;
int gmsgGmplayFD = 0;

int gmsgGmplayFEC = 0;
int gmsgGmplayFE = 0;

int gmsgGmplayPC = 0;
int gmsgGmplayP = 0;

int gmsgGmplayTC = 0;
int gmsgGmplayT = 0;
std::string lastConfigFile;
#endif

void GameplayModData::Init() {
#ifndef CLIENT_DLL
	if ( !gmsgGmplayMod ) {
		gmsgGmplayMod = REG_USER_MSG( "GmplayMod", -1 );
		gmsgGmplayCfg = REG_USER_MSG( "GmplayCfg", -1 );

		gmsgGmplayFDC = REG_USER_MSG( "GmplayFDC", 0 );
		gmsgGmplayFD = REG_USER_MSG( "GmplayFD", -1 );
		
		gmsgGmplayFEC = REG_USER_MSG( "GmplayFEC", 0 );
		gmsgGmplayFE = REG_USER_MSG( "GmplayFE", -1 );

		gmsgGmplayPC = REG_USER_MSG( "GmplayPC", 4 );
		gmsgGmplayP = REG_USER_MSG( "GmplayP", -1 );
		
		gmsgGmplayTC = REG_USER_MSG( "GmplayTC", 4 );
		gmsgGmplayT = REG_USER_MSG( "GmplayT", -1 );
	}
#else
	gEngfuncs.pfnHookUserMsg( "GmplayMod", []( const char *pszName, int iSize, void *pbuf ) -> int {
		BEGIN_READ( pbuf, iSize );
		gameplayModsData.reverseGravity = READ_BYTE();
		gameplayModsData.holdingTwinWeapons = READ_BYTE();
		g_inverseControls = READ_BYTE();
		g_musicSlowmotion = READ_BYTE();
		gameplayModsData.timeLeftUntilNextRandomGameplayMod = READ_FLOAT();

		return 1;
	} );

	gEngfuncs.pfnHookUserMsg( "GmplayCfg", []( const char *pszName, int iSize, void *pbuf ) -> int {
		BEGIN_READ( pbuf, iSize );
		std::string filePath = READ_STRING();
		if ( filePath.empty() ) {
			clientConfig.Reset();
		} else {
			clientConfig.ReadFile( filePath.c_str() );
		}

		return 1;
	} );

	gEngfuncs.pfnHookUserMsg( "GmplayFDC", []( const char *pszName, int iSize, void *pbuf ) -> int {
		BEGIN_READ( pbuf, iSize );
		gameplayMods::forceDisabledMods.clear();

		return 1;
	} );

	gEngfuncs.pfnHookUserMsg( "GmplayFD", []( const char *pszName, int iSize, void *pbuf ) -> int {
		BEGIN_READ( pbuf, iSize );
		std::string modName = READ_STRING();

		using namespace gameplayMods;
		if ( byString.find( modName ) != byString.end() ) {
			forceDisabledMods.insert( byString[modName] );
		}

		return 1;
	} );

	gEngfuncs.pfnHookUserMsg( "GmplayFEC", []( const char *pszName, int iSize, void *pbuf ) -> int {
		BEGIN_READ( pbuf, iSize );
		gameplayMods::forceEnabledMods.clear();

		return 1;
	} );

	gEngfuncs.pfnHookUserMsg( "GmplayFE", []( const char *pszName, int iSize, void *pbuf ) -> int {
		BEGIN_READ( pbuf, iSize );
		auto modString = aux::str::split( READ_STRING(), '|' );
		if ( modString.empty() ) {
			return 1;
		}
		auto modName = modString.at( 0 );
		auto argString = modString.size() >= 2 ? modString.at( 1 ) : "";

		using namespace gameplayMods;
		if ( byString.find( modName ) != byString.end() ) {
			auto mod = byString[modName];
			auto parsedArgs = mod->ParseStringArguments( argString );
			
			forceEnabledMods[mod] = parsedArgs;
		}

		return 1;
	} );

	gEngfuncs.pfnHookUserMsg( "GmplayPC", []( const char *pszName, int iSize, void *pbuf ) -> int {
		BEGIN_READ( pbuf, iSize );
		auto size = READ_LONG();
		gameplayMods::proposedGameplayModsClient.resize( size );

		return 1;
	} );

	gEngfuncs.pfnHookUserMsg( "GmplayP", []( const char *pszName, int iSize, void *pbuf ) -> int {
		BEGIN_READ( pbuf, iSize );

		size_t index = READ_LONG();
		auto votes = READ_LONG();
		auto voteDistribution = READ_FLOAT();
		auto modName = READ_STRING();

		using namespace gameplayMods;
		if ( index >= proposedGameplayModsClient.size() ) {
			return 1;
		}

		if ( byString.find( modName ) != byString.end() ) {
			auto &proposedMod = proposedGameplayModsClient[index];
			proposedMod.mod = byString[modName];
			proposedMod.votes = votes;
			proposedMod.voteDistributionPercent = voteDistribution;
		}

		return 1;
	} );

	gEngfuncs.pfnHookUserMsg( "GmplayTC", []( const char *pszName, int iSize, void *pbuf ) -> int {
		BEGIN_READ( pbuf, iSize );
		gameplayMods::timedGameplayMods.resize( READ_LONG() );

		return 1;
	} );

	gEngfuncs.pfnHookUserMsg( "GmplayT", []( const char *pszName, int iSize, void *pbuf ) -> int {
		BEGIN_READ( pbuf, iSize );

		size_t index = READ_LONG();
		auto time = READ_FLOAT();
		auto timeInitial = READ_FLOAT();
		
		auto modString = aux::str::split( READ_STRING(), '|' );
		if ( modString.empty() ) {
			return 1;
		}
		auto modName = modString.at( 0 );
		auto argString = modString.size() >= 2 ? modString.at( 1 ) : "";

		using namespace gameplayMods;
		if ( index >= timedGameplayMods.size() ) {
			return 1;
		}

		if ( byString.find( modName ) != byString.end() ) {
			auto mod = byString[modName];
			timedGameplayMods[index] = {
				mod,
				mod->ParseStringArguments( argString ),
				time,
				timeInitial
			};
		}

		return 1;
	} );
#endif

}

#ifndef CLIENT_DLL

extern int g_autoSaved;

int GameplayModData::Save( CSave &save ) {
	AddArrayFieldDefinitions();

	if ( CCustomGameModeRules *cgm = dynamic_cast< CCustomGameModeRules * >( g_pGameRules ) ) {
		for ( size_t i = 0; i < min( cgm->config.endConditions.size(), ( size_t ) 64 ); i++ ) {
			auto &endCondition = cgm->config.endConditions.at( i );
			sprintf( gameplayModsData.endConditionsHashes[i], "%s", endCondition.GetHash().c_str() );
			endConditionsActivationCounts[i] = endCondition.activations;
		}
	}

	gameplayModsData.forceDisconnect = gameplayMods::noSaving.isActive();
	auto saveResult = save.WriteFields( "GAMEPLAY_MODS", this, fields.data(), fields.size() );
	gameplayModsData.forceDisconnect = FALSE;
	g_autoSaved = FALSE;

	return saveResult;
}

int GameplayModData::Restore( CRestore &restore ) {
	AddArrayFieldDefinitions();
	return restore.ReadFields( "GAMEPLAY_MODS", this, fields.data(), fields.size() );
}

void GameplayModData::Reset() {
	gameplayModsData = GameplayModData();
	lastConfigFile = "";
}

void GameplayModData::SendToClient() {
	if ( !gmsgGmplayMod ) {
		return;
	}

	int musicSlowmotion = 0;
	if ( auto player = GetPlayer() ) {
		musicSlowmotion = player->slowMotionWasEnabled && !player->musicNoSlowmotionEffects;
	}

	MESSAGE_BEGIN( MSG_ALL, gmsgGmplayMod );
		WRITE_BYTE( reverseGravity );
		WRITE_BYTE( holdingTwinWeapons );
		WRITE_BYTE( gameplayMods::inverseControls.isActive() );
		WRITE_BYTE( musicSlowmotion );
		WRITE_FLOAT( timeLeftUntilNextRandomGameplayMod );
	MESSAGE_END();

	if ( auto randomGameplayMods = gameplayMods::randomGameplayMods.isActive<RandomGameplayModsInfo>() ) {
		using namespace gameplayMods;
		
		MESSAGE_BEGIN( MSG_ALL, gmsgGmplayPC );
			WRITE_LONG( proposedGameplayMods.size() );
		MESSAGE_END();

		for ( size_t i = 0; i < proposedGameplayMods.size(); i++ ) {
			auto &proposedMod = proposedGameplayMods.at( proposedGameplayMods.size() - 1 - i );
			MESSAGE_BEGIN( MSG_ALL, gmsgGmplayP );
				WRITE_LONG( i );
				WRITE_LONG( proposedMod.votes.size() );
				WRITE_FLOAT( proposedMod.voteDistributionPercent );
				WRITE_STRING( proposedMod.mod->id.c_str() );
			MESSAGE_END();
		}	

		MESSAGE_BEGIN( MSG_ALL, gmsgGmplayTC );
			WRITE_LONG( timedGameplayMods.size() );
		MESSAGE_END();

		for ( size_t i = 0; i < timedGameplayMods.size(); i++ ) {
			auto &timedMod = timedGameplayMods.at( i );
			
			std::string modString = timedMod.mod->id + "|";
			for ( auto &arg : timedMod.args ) {
				modString += arg.string + " ";
			}
			aux::str::rtrim( &modString );

			MESSAGE_BEGIN( MSG_ALL, gmsgGmplayT );
				WRITE_LONG( i );
				WRITE_FLOAT( timedMod.time );
				WRITE_FLOAT( timedMod.initialTime );
				WRITE_STRING( modString.c_str() );
			MESSAGE_END();
		}
	} else {

		MESSAGE_BEGIN( MSG_ALL, gmsgGmplayPC );
			WRITE_LONG( 0 );
		MESSAGE_END();

		MESSAGE_BEGIN( MSG_ALL, gmsgGmplayTC );
			WRITE_LONG( 0 );
		MESSAGE_END();
	}

	if ( CCustomGameModeRules *cgm = dynamic_cast< CCustomGameModeRules * >( g_pGameRules ) ) {
		if ( lastConfigFile != cgm->config.configName ) {
			lastConfigFile = cgm->config.configName;
			MESSAGE_BEGIN( MSG_ALL, gmsgGmplayCfg );
				WRITE_STRING( lastConfigFile.c_str() );
			MESSAGE_END();

			gameplayMods::timedGameplayMods.clear();
			gameplayMods::proposedGameplayMods.clear();
			gameplayMods::previouslyProposedRandomMods.clear();
		}
	}
}

#endif

bool gameplayMods::AllowedToVoteOnRandomGameplayMods() {
	auto randomGameplayMods = gameplayMods::randomGameplayMods.isActive<RandomGameplayModsInfo>();
	if ( !randomGameplayMods ) {
		return false;
	}

	return gameplayMods::proposedGameplayMods.size() > 0 &&
		randomGameplayMods->timeForRandomGameplayModVoting >= 10.0f &&
		gameplayModsData.timeLeftUntilNextRandomGameplayMod <= randomGameplayMods->timeForRandomGameplayModVoting;
}

bool gameplayMods::PaynedSoundsEnabled( bool isMonster ) {
	return
		( !isMonster && gameplayMods::paynedSoundsHumans.isActive() ) ||
		( isMonster && gameplayMods::paynedSoundsMonsters.isActive() ) ||
		gameplayMods::deusExSounds.isActive() ||
		gameplayMods::cncSounds.isActive();
}

void GameplayModData::AddArrayFieldDefinitions() {
	if ( !addedAdditionalFields ) {
		// HACK: didn't figure out simple macro to avoid defining these fields in this place

		fields.push_back( DEFINE_ARRAY( GameplayModData, activeGameModeConfig, FIELD_CHARACTER, 256 ) );
		fields.push_back( DEFINE_ARRAY( GameplayModData, activeGameModeConfigHash, FIELD_CHARACTER, 128 ) );

		fields.push_back( DEFINE_ARRAY( GameplayModData, gungameWeapon, FIELD_CHARACTER, 128 ) );
		fields.push_back( DEFINE_ARRAY( GameplayModData, gungamePriorWeapon, FIELD_CHARACTER, 128 ) );

		fields.push_back( DEFINE_ARRAY( GameplayModData, endConditionsActivationCounts, FIELD_INTEGER, 64 ) );
		for ( int i = 0; i < 64; i++ ) {
			fields.push_back( {
				FIELD_CHARACTER,
				"stuffToWrite",
				( int ) offsetof( GameplayModData, endConditionsHashes[i] ),
				128,
				0
			} );
		}

		addedAdditionalFields = true;
	}
}



std::map<std::string, GameplayMod *> gameplayMods::byString;
std::set<GameplayMod *> gameplayMods::allowedForRandom;
std::set<GameplayMod *> gameplayMods::previouslyProposedRandomMods;
std::map<GameplayMod *, std::vector<Argument>> gameplayMods::forceEnabledMods;
std::set<GameplayMod *> gameplayMods::forceDisabledMods;
std::vector<ProposedGameplayMod> gameplayMods::proposedGameplayMods;
std::vector<ProposedGameplayModClient> gameplayMods::proposedGameplayModsClient;
std::vector<TimedGameplayMod> gameplayMods::timedGameplayMods;

GameplayMod& GameplayMod::Define( const std::string &id, const std::string &name ) {
	auto *mod = new GameplayMod( id, name );
	gameplayMods::byString[id] = mod;
	gameplayMods::allowedForRandom.insert( mod );
	return *mod;
}

GameplayMod& GameplayMod::Description( const std::string &description ) {
	this->description = description;
	return *this;
}

GameplayMod &GameplayMod::RandomGameplayModName( const std::string &name ) {
	this->randomGameplayModName = name;
	return *this;
}

GameplayMod &GameplayMod::RandomGameplayModDescription( const std::string &description ) {
	this->randomGameplayModDescription = description;
	return *this;
}

GameplayMod& GameplayMod::IsAlsoActiveWhen( const IsAlsoActiveWhenFunction &func ) {
	this->isAlsoActiveWhen = func;
	return *this;
}

GameplayMod &GameplayMod::IsExcludedFromChaosEdition() {
	this->excludedFromChaosEdition = true;
	return *this;
}

GameplayMod& GameplayMod::ForceDefaultArguments( const std::string &arguments ) {
	this->forcedDefaultArguments = arguments;
	return *this;
}

GameplayMod & GameplayMod::OnEventInit( const EventInitFunction &func ) {
	this->EventInit = func;
	this->isEvent = true;
	return *this;
}

GameplayMod& GameplayMod::Arguments( const std::vector<Argument> &args ) {
	this->arguments = args;
	return *this;
}

std::vector<Argument> GameplayMod::ParseStringArguments( const std::vector<std::string> &argsParsed ) {
	auto argumentsCopy = arguments;

	for ( size_t i = 0; i < min( argumentsCopy.size(), argsParsed.size() ); i++ ) {
		argumentsCopy.at( i ).Init( argsParsed.at( i ) );
	}

	return argumentsCopy;
}

std::vector<Argument> GameplayMod::ParseStringArguments( const std::string &args ) {
	return ParseStringArguments( aux::str::getCommandLineArguments( args ) );
}

std::vector<Argument> GameplayMod::GetRandomArguments() {
	auto randomArguments = this->arguments;

	for ( auto &arg : randomArguments ) {
		float min = arg.randomMin;
		float max = arg.randomMax;

		if ( arg.isNumber && !std::isnan( min ) && !std::isnan( max ) ) {
			arg.Init( std::to_string( aux::rand::uniformFloat( min, max ) ) );
		}
	}

	return randomArguments;
}

std::optional<std::vector<Argument>> GameplayMod::GetArgumentsFromCustomGameplayModeConfig() {
#ifndef CLIENT_DLL
	if ( CHalfLifeRules *rules = dynamic_cast< CHalfLifeRules * >( g_pGameRules ) ) {
		for ( auto it = rules->configs.rbegin(); it != rules->configs.rend(); it++ ) {
			auto &cfg = *( *it );
			if ( cfg.mods.find( this ) != cfg.mods.end() ) {
				return cfg.mods[this];
			}
		}
	}
#else
	if ( clientConfig.mods.find( this ) != clientConfig.mods.end() ) {
		return clientConfig.mods[this];
	}
#endif

	return std::nullopt;
}

#ifndef CLIENT_DLL
void GameplayModData::ToggleForceEnabledGameplayMod( const std::string &mod ) {
	using namespace gameplayMods;

	if ( auto modWithArguments = gameplayMods::GetModAndParseArguments( mod ) ) {

		auto mod = modWithArguments->first;
		auto &args = modWithArguments->second;

		if ( forceEnabledMods.find( mod ) != forceEnabledMods.end() ) {
			forceEnabledMods.erase( mod );
			ALERT( at_notice, "Removed force enabled mod %s\n", mod->id.c_str() );
		} else {
			forceEnabledMods[mod] = args;
			ALERT( at_notice, "Added force enabled mod %s\n", mod->id.c_str() );
		}

		MESSAGE_BEGIN( MSG_ALL, gmsgGmplayFEC, NULL );
		MESSAGE_END();

		for ( auto &enabledMod : forceEnabledMods ) {
			std::string modString = enabledMod.first->id + "|";
			for ( auto &arg : args ) {
				modString += arg.string + " ";
			}
			aux::str::rtrim( &modString );

			MESSAGE_BEGIN( MSG_ALL, gmsgGmplayFE, NULL );
				WRITE_STRING( modString.c_str() );
			MESSAGE_END();
		}
	} else {
		ALERT( at_notice, "Mod doesn't exist: %s\n", mod.c_str() );
	}
}

void GameplayModData::ToggleForceDisabledGameplayMod( const std::string &mod ) {
	using namespace gameplayMods;

	if ( auto modWithArguments = gameplayMods::GetModAndParseArguments( mod ) ) {

		auto mod = modWithArguments->first;

		if ( forceDisabledMods.find( mod ) != forceDisabledMods.end() ) {
			forceDisabledMods.erase( mod );
			ALERT( at_notice, "Removed force disabled mod %s\n", mod->id.c_str() );
		} else {
			forceDisabledMods.insert( mod );
			ALERT( at_notice, "Added force disabled mod %s\n", mod->id.c_str() );
		}

		MESSAGE_BEGIN( MSG_ALL, gmsgGmplayFDC, NULL );
		MESSAGE_END();

		for ( auto &disabledMod : forceDisabledMods ) {
			MESSAGE_BEGIN( MSG_ALL, gmsgGmplayFD, NULL );
				WRITE_STRING( disabledMod->id.c_str() );
			MESSAGE_END();
		}
	} else {
		ALERT( at_notice, "Mod doesn't exist: %s\n", mod.c_str() );
	}
}
#endif

std::optional<std::vector<Argument>> GameplayMod::getActiveArguments( bool discountForcedArguments ) {
	using namespace gameplayMods;

	if ( aux::ctr::includes( forceDisabledMods, this ) ) {
		return std::nullopt;
	}

	if ( auto arguments = this->isAlsoActiveWhen() ) {
		return ParseStringArguments( *arguments );
	}

	if ( forceEnabledMods.find( this ) != forceEnabledMods.end() ) {
		return forceEnabledMods[this];
	}

	for ( auto &timedMod : timedGameplayMods ) {
		if ( timedMod.mod == this ) {
			return timedMod.args;
		}
	}

	if ( auto args = GetArgumentsFromCustomGameplayModeConfig() ) {
		return args;
	}

	if ( forcedDefaultArguments.empty() || discountForcedArguments ) {
		return std::nullopt;
	} else {
		return ParseStringArguments( forcedDefaultArguments );
	}
}

bool GameplayMod::isActive( bool discountForcedArguments ) {
	auto args = getActiveArguments( discountForcedArguments );
	return args.has_value();
}

GameplayMod& GameplayMod::CannotBeActivatedRandomly() {
	this->CanBeActivatedRandomly = []() { return false; };
	gameplayMods::allowedForRandom.erase( this );
	return *this;
}

GameplayMod& GameplayMod::CanOnlyBeActivatedRandomlyWhen( const std::function<bool()> &randomActivateCondition ) {
	this->CanBeActivatedRandomly = randomActivateCondition;
	return *this;
}

GameplayMod &GameplayMod::CanBeCancelledAfterChangeLevel() {
	this->canBeCancelledAfterChangeLevel = true;
	return *this;
}

bool gameplayMods::IsSlowmotionEnabled() {
	if ( gameplayMods::superHot.isActive() ) {
		return false;
	}

	auto timescale_multiplier = *gameplayMods::timescale.isActive<float>() + gameplayModsData.timescaleAdditive;

	float sys_timescale = SHARED_CVAR_GET_FLOAT( "sys_timescale" );
	bool using_sys_timescale = sys_timescale != 0.0f; // dirty way

	if ( using_sys_timescale ) {
		return sys_timescale <= ( timescale_multiplier / 4.0f );
	} else {
		float host_framerate = SHARED_CVAR_GET_FLOAT( "host_framerate" );
		float g_fps_max = SHARED_CVAR_GET_FLOAT( "fps_max" );
		float base = ( 1000.0f / g_fps_max ) / 1000.0f;

		return host_framerate > 0.0f && host_framerate < base;
	}
}

std::optional<std::pair<GameplayMod*, std::vector<Argument>>> gameplayMods::GetModAndParseArguments( const std::string & line ) {
	auto contents = aux::str::split( line, ' ' );
	std::string modName = contents.at( 0 );

	if ( gameplayMods::byString.find( modName ) != gameplayMods::byString.end() ) {
		auto mod = gameplayMods::byString[modName];

		contents.erase( contents.begin() );
		auto parsedArgs = mod->ParseStringArguments( contents );

		return std::optional<std::pair<GameplayMod*, std::vector<Argument>>>( { mod, parsedArgs } );
	}

	return std::nullopt;
}

bool gameplayMods::PlayerShouldProducePhysicalBullets() {
	auto bulletPhysicsMode = *gameplayMods::bulletPhysics.isActive<BulletPhysicsMode>();
	return bulletPhysicsMode == BulletPhysicsMode::Enabled ||
			( bulletPhysicsMode == BulletPhysicsMode::ForEnemiesAndPlayerDuringSlowmotion && gameplayMods::IsSlowmotionEnabled() );

	return false;
}

std::set<GameplayMod*> gameplayMods::GetFilteredRandomGameplayMods() {
#ifndef CLIENT_DLL
	if ( CCustomGameModeRules *cgm = dynamic_cast< CCustomGameModeRules * >( g_pGameRules ) ) {
		return aux::ctr::filter( gameplayMods::allowedForRandom, [cgm]( GameplayMod *mod ) {

			if ( aux::ctr::includes( gameplayMods::previouslyProposedRandomMods, mod ) ) {
				return false;
			}

			if ( !cgm->config.randomModsWhitelist.empty() && !aux::ctr::includes( cgm->config.randomModsWhitelist, mod->id ) ) {
				return false;
			}

			if ( aux::ctr::includes( cgm->config.randomModsBlacklist, mod->id ) ) {
				return false;
			}

			if ( mod->isActive( true ) ) {
				return false;
			}

			if ( mod->excludedFromChaosEdition && gameplayMods::chaosEdition.isActive() ) {
				return false;
			}

			return mod->CanBeActivatedRandomly();
		} );
	}
#endif

	return {};
}