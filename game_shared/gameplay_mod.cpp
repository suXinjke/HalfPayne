#include "gameplay_mod.h"
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

	// HACK: didn't figure out simple macro to avoid defining these fields in this place
	fields.push_back( DEFINE_ARRAY( GameplayMods, activeGameModeConfig, FIELD_CHARACTER, 256 ) );
	fields.push_back( DEFINE_ARRAY( GameplayMods, activeGameModeConfigHash, FIELD_CHARACTER, 128 ) );
	fields.push_back( DEFINE_ARRAY( GameplayMods, teleportOnKillWeapon, FIELD_CHARACTER, 64 ) );
}

#ifndef CLIENT_DLL

int GameplayMods::Save( CSave &save ) {
	return save.WriteFields( "GAMEPLAY_MODS", this, fields.data(), fields.size() );
}

int GameplayMods::Restore( CRestore &restore ) {
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