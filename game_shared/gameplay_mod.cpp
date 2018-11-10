#include <map>
#include <random>
#include "gameplay_mod.h"
#include "player.h"
#include "cgm_gamerules.h"
GameplayMods gameplayMods;

#ifdef CLIENT_DLL
#include "parsemsg.h"
float g_fovOffsetAmplitude = 0.0f;
float g_fovOffsetChangeFreqency = 0.0f;
int g_inverseControls = 0;
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
		gameplayMods.infiniteAmmoClip = READ_BYTE();
		gameplayMods.shootUnderwater = READ_BYTE();
		gameplayMods.holdingTwinWeapons = READ_BYTE();
		gameplayMods.gaussFastCharge = READ_BYTE();
		g_inverseControls = READ_BYTE();
		g_fovOffsetAmplitude = READ_FLOAT();
		g_fovOffsetChangeFreqency = READ_FLOAT();

		return 1;
	} );
#endif

}

#ifndef CLIENT_DLL

extern int g_autoSaved;

int GameplayMods::Save( CSave &save ) {
	AddArrayFieldDefinitions();

	if ( CCustomGameModeRules *cgm = dynamic_cast< CCustomGameModeRules * >( g_pGameRules ) ) {
		for ( int i = 0; i < min( cgm->config.endConditions.size(), ( size_t ) 64 ); i++ ) {
			auto &endCondition = cgm->config.endConditions.at( i );
			sprintf( gameplayMods.endConditionsHashes[i], "%s", endCondition.GetHash().c_str() );
			endConditionsActivationCounts[i] = endCondition.activations;
		}
	}

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
		WRITE_BYTE( infiniteAmmoClip );
		WRITE_BYTE( shootUnderwater );
		WRITE_BYTE( holdingTwinWeapons );
		WRITE_BYTE( gaussFastCharge );
		WRITE_BYTE( inverseControls );
		WRITE_FLOAT( fovOffsetAmplitude );
		WRITE_FLOAT( fovOffsetChangeFreqency );
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

		fields.push_back( DEFINE_ARRAY( GameplayMods, endConditionsActivationCounts, FIELD_INTEGER, 64 ) );
		for ( int i = 0; i < 64; i++ ) {
			fields.push_back( {
				FIELD_CHARACTER,
				"stuffToWrite",
				( int ) offsetof( GameplayMods, endConditionsHashes[i] ),
				128,
				0
			} );
		}

		addedAdditionalFields = true;
	}
}

void GameplayMods::SetGameplayModActiveByString( const std::string &line, bool isActive ) {
	auto args = aux::str::getCommandLineArguments( line );

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
	
		auto it = aux::rand::choice( gameplayMods.begin(), gameplayMods.end() );
		auto &randomGameplayMod = it->second;

		for ( auto &arg : randomGameplayMod.arguments ) {
			float min = arg.randomMin;
			float max = arg.randomMax;

			if ( arg.isNumber && !std::isnan( min ) && !std::isnan( max ) ) {
				arg.Init( std::to_string( aux::rand::uniformFloat( min, max ) ) );
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