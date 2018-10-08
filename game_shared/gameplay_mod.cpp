#include <map>
#include "gameplay_mod.h"
#include "string_aux.h"
#include "player.h"
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

int GameplayMods::Save( CSave &save ) {
	AddArrayFieldDefinitions();
	return save.WriteFields( "GAMEPLAY_MODS", this, fields.data(), fields.size() );
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
	std::vector<Argument> parsedArgs = {
		Argument( "mod" ),
		Argument( "param1" ).IsOptional().CanBeNumber(),
		Argument( "param2" ).IsOptional().CanBeNumber(),
		Argument( "param3" ).IsOptional().CanBeNumber(),
		Argument( "param4" ).IsOptional().CanBeNumber(),
		Argument( "param5" ).IsOptional().CanBeNumber()
	};

	for ( size_t i = 0; i < min( parsedArgs.size(), args.size() ); i++ ) {
		auto value = args.at( i );
		auto parsingError = parsedArgs.at( i ).Init( value );
		if ( !parsingError.empty() ) {
			// TODO: replace with proper engine function call
			ALERT( at_notice, "Error getting gameplay mod by string: %s\n", parsingError.c_str() );
		}
	}

	std::string modName = parsedArgs.at( 0 ).string;
	bool activatedMod = false;

	for ( auto &pair : gameplayModDefs ) {
		auto mod = pair.second;
		if ( mod.id == modName ) {
			activatedMod = true;

			for ( size_t i = 0; i < min( mod.arguments.size(), parsedArgs.size() - 1 ); i++ ) {
				auto parsingError = mod.arguments.at( i ).Init( parsedArgs.at( i + 1 ).string );

				if ( !parsingError.empty() ) {
					ALERT( at_notice, "%s, argument %d: %s", modName.c_str(), i + 1, parsingError.c_str() );
					return;
				}
			}

			if ( CBasePlayer *pPlayer = ( CBasePlayer * ) CBasePlayer::Instance( g_engfuncs.pfnPEntityOfEntIndex( 1 ) ) ) {
				if ( isActive ) {
					mod.init( pPlayer, mod.arguments );
				} else {
					if ( mod.canBeDeactivated ) {
						mod.deactivate( pPlayer, mod.arguments );
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