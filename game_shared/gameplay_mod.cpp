#include <map>
#include <random>
#include "gameplay_mod.h"
#include "string_aux.h"
#include "player.h"
#include "util_aux.h"
#include "cgm_gamerules.h"
GameplayMods gameplayMods;

#ifdef CLIENT_DLL
#include "parsemsg.h"
#else
int gmsgGmplayMod = 0;
#endif

void GameplayMods::Init() {
#ifndef CLIENT_DLL
	if ( !gmsgGmplayMod ) {
		gmsgGmplayMod = REG_USER_MSG( "GmplayMod", -1 );
	}
#else
	gEngfuncs.pfnHookUserMsg( "GmplayMod", []( const char *pszName, int iSize, void *pbuf ) -> int {
		BEGIN_READ( pbuf, iSize );
		gameplayMods.automaticShotgun = READ_BYTE();
		gameplayMods.instaGib = READ_BYTE();
		gameplayMods.noSecondaryAttack = READ_BYTE();
		gameplayMods.bulletPhysical = READ_BYTE();
		gameplayMods.snarkNuclear = READ_BYTE();

		return 1;
	} );
#endif

}

#ifndef CLIENT_DLL

extern int g_autoSaved;

int GameplayMods::Save( CSave &save ) {
	AddArrayFieldDefinitions();

	BOOL previousNoSavingValue = gameplayMods.noSaving;
	if ( gameplayMods.autosavesOnly && g_autoSaved ) {
		gameplayMods.noSaving = FALSE;
	}
	auto saveResult = save.WriteFields( "GAMEPLAY_MODS", this, fields.data(), fields.size() );
	if ( gameplayMods.autosavesOnly && g_autoSaved ) {
		gameplayMods.noSaving = previousNoSavingValue;
	}

	g_autoSaved = FALSE;

	return saveResult;
}

int GameplayMods::Restore( CRestore &restore ) {
	AddArrayFieldDefinitions();
	return restore.ReadFields( "GAMEPLAY_MODS", this, fields.data(), fields.size() );
}

void GameplayMods::Reset() {
	gameplayMods = GameplayMods();
}

void GameplayMods::SendToClient() {
	if ( !gmsgGmplayMod ) {
		return;
	}

	MESSAGE_BEGIN( MSG_ALL, gmsgGmplayMod );
		WRITE_BYTE( automaticShotgun );
		WRITE_BYTE( instaGib );
		WRITE_BYTE( noSecondaryAttack );
		WRITE_BYTE( bulletPhysical );
		WRITE_BYTE( snarkNuclear );
	MESSAGE_END();
}

#endif

bool GameplayMods::AllowedToVoteOnRandomGameplayMods() {
	return
		proposedGameplayMods.size() > 0 &&
		timeForRandomGameplayMod >= 10.0f &&
		timeLeftUntilNextRandomGameplayMod < timeForRandomGameplayModVoting;
}

void GameplayMods::AddArrayFieldDefinitions() {
	if ( !addedAdditionalFields ) {
		// HACK: didn't figure out simple macro to avoid defining these fields in this place
		fields.push_back( DEFINE_ARRAY( GameplayMods, activeGameModeConfig, FIELD_CHARACTER, 256 ) );
		fields.push_back( DEFINE_ARRAY( GameplayMods, activeGameModeConfigHash, FIELD_CHARACTER, 128 ) );
		fields.push_back( DEFINE_ARRAY( GameplayMods, teleportOnKillWeapon, FIELD_CHARACTER, 64 ) );

		addedAdditionalFields = true;
	}
}

void GameplayMods::SetGameplayModActiveByString( const std::string &line, bool isActive ) {
	auto args = NaiveCommandLineParse( line );

	std::string modName = args.at( 0 );
	bool activatedMod = false;

	for ( auto &pair : gameplayModDefs ) {
		auto mod = pair.second;
		if ( mod.id == modName ) {
			activatedMod = true;

			if ( CBasePlayer *pPlayer = ( CBasePlayer * ) CBasePlayer::Instance( g_engfuncs.pfnPEntityOfEntIndex( 1 ) ) ) {
				if ( isActive ) {
					auto argsWithoutModName = std::vector<std::string>( args.begin() + 1, args.end() );
					mod.Init( pPlayer, argsWithoutModName );
				} else {
					if ( mod.canBeDeactivated ) {
						mod.Deactivate( pPlayer );
					} else {
						ALERT( at_notice, "this mod cannot be deactivated\n" );
					}
				}
			}
			break;
		}
	}

	if ( !activatedMod ) {
		ALERT( at_notice, "incorrect mod specified: %s\n", modName.c_str() );
	}
}

std::vector<GameplayMod> GameplayMods::GetRandomGameplayMod( CBasePlayer *player, size_t modAmount, std::function<bool( const GameplayMod &mod )> filter ) {

	std::vector<GameplayMod> randomGameplayMods;

	auto gameplayMods = FilterGameplayMods( gameplayModDefs, player, filter );

	for ( size_t i = 0; i < modAmount && gameplayMods.size() > 0; i++ ) {
	
		int randomModIndex = UniformInt( 0, gameplayMods.size() - 1 );

		auto it = gameplayMods.begin();
		std::advance( it, randomModIndex );
		auto &randomGameplayMod = it->second;

		for ( auto &arg : randomGameplayMod.arguments ) {
			float min = arg.randomMin;
			float max = arg.randomMax;

			if ( arg.isNumber && !std::isnan( min ) && !std::isnan( max ) ) {
				arg.Init( std::to_string( UniformFloat( min, max ) ) );
			}
		}

		randomGameplayMods.push_back( randomGameplayMod );

		gameplayMods.erase( it );
	}

	return randomGameplayMods;
}

std::map<GAMEPLAY_MOD, GameplayMod> GameplayMods::FilterGameplayMods( std::map<GAMEPLAY_MOD, GameplayMod> gameplayMods, CBasePlayer *player, std::function<bool( const GameplayMod &mod )> filter ) {
	for ( auto it = gameplayMods.begin(); it != gameplayMods.end(); ) {
		if (
			( it->second.isEvent && !it->second.CanBeActivatedRandomly( player ) ) ||
			( !it->second.isEvent && !it->second.canBeDeactivated || it->second.cannotBeActivatedRandomly || !it->second.CanBeActivatedRandomly( player ) || !filter( it->second ) )
		) {
			it = gameplayMods.erase( it );
		} else {
			it++;
		}
	}

	return gameplayMods;
}

GameplayMod GameplayMods::GetRandomGameplayMod( CBasePlayer *player ) {
	return GetRandomGameplayMod( player, 1 ).at( 0 );
}