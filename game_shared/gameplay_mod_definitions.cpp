#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "custom_gamemode_config.h"
#include "weapons.h"
#include <algorithm>
#include <iterator>
#include "gamerules.h"

std::map<GAMEPLAY_MOD, GameplayMod> gameplayModDefs = {
	
	{ GAMEPLAY_MOD_ALWAYS_GIB, GameplayMod( "always_gib", "Always gib" )
		.Description( "Kills will always try to result in gibbing." )
		.Toggles( &gameplayMods.gibsAlways )
	},	

	{ GAMEPLAY_MOD_AUTOSAVES_ONLY, GameplayMod( "autosaves_only", "Autosaves only" )
		.Description( "Only autosaves are allowed. You are not allowed to quicksave." )
		.Toggles( { &gameplayMods.autosavesOnly, &gameplayMods.noSaving } )
		.CannotBeActivatedRandomly()
	},

	{ GAMEPLAY_MOD_BLACK_MESA_MINUTE, GameplayMod( "black_mesa_minute", "Black Mesa Minute" )
		.Description( "Time-based game mode - rush against a minute, kill enemies to get more time." )
		.OnInit( []( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.blackMesaMinute = true;
			gameplayMods.timerShown = true;
			gameplayMods.time = 60.0f;
			gameplayMods.timerBackwards = true;
		} )
		.CannotBeActivatedRandomly()
	},

	{ GAMEPLAY_MOD_BLEEDING, GameplayMod( "bleeding", "Bleeding" )
		.Description( "After your last painkiller take, you start to lose health." )
		.Arguments( {
			Argument( "bleeding_handicap" ).IsOptional().MinMax( 0, 99 ).RandomMinMax( 10, 50 ).Default( "20" ).Description( []( const std::string string, float value ) {
				return "Bleeding until " + std::to_string( value ) + "%% health left\n";
			} ),
			Argument( "bleeding_update_period" ).IsOptional().MinMax( 0.01 ).RandomMinMax( 0.1, 0.2 ).Default( "1" ).Description( []( const std::string string, float value ) {
				return "Bleed update period: " + std::to_string( value ) + " sec \n";
			} ),
			Argument( "bleeding_immunity_period" ).IsOptional().MinMax( 0.05 ).RandomMinMax( 3, 10 ).Default( "10" ).Description( []( const std::string string, float value ) {
				return "Bleed again after healing in: " + std::to_string( value ) + " sec \n";
			} )
		} )
		.OnInit( []( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.bleeding = TRUE;
			gameplayMods.bleedingHandicap = args.at( 0 ).number;
			gameplayMods.bleedingUpdatePeriod = args.at( 1 ).number;
			gameplayMods.bleedingImmunityPeriod = args.at( 2 ).number;
		} )
		.OnDeactivation( []( CBasePlayer *player ) {
			gameplayMods.bleeding = FALSE;
		} )
	},

	{ GAMEPLAY_MOD_BULLET_DELAY_ON_SLOWMOTION, GameplayMod( "bullet_delay_on_slowmotion", "Bullet delay on slowmotion" )
		.Description( "When slowmotion is activated, physical bullets shot by you will move slowly until you turn off the slowmotion." )
		.Toggles( &gameplayMods.bulletDelayOnSlowmotion )
		.CanOnlyBeActivatedRandomlyWhen( []( CBasePlayer *player ) -> bool {
			return gameplayMods.bulletPhysicsMode == BULLET_PHYSICS_ENEMIES_AND_PLAYER_ON_SLOWMOTION ||
				gameplayMods.bulletPhysicsMode == BULLET_PHYSICS_CONSTANT;
		} )
	},

	{ GAMEPLAY_MOD_BULLET_PHYSICS_DISABLED, GameplayMod( "bullet_physics_disabled", "Bullet physics disabled" )
		.Description( "Self explanatory." )
		.OnInit( []( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.bulletPhysicsMode = BULLET_PHYSICS_DISABLED;
		} )
		.CannotBeActivatedRandomly()
	},

	{ GAMEPLAY_MOD_BULLET_PHYSICS_CONSTANT, GameplayMod( "bullet_physics_constant", "Bullet physics constant" )
		.Description( "Bullet physics is always present, even when slowmotion is NOT present." )
		.OnInit( []( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.bulletPhysicsMode = BULLET_PHYSICS_CONSTANT;
		} )
		.CannotBeActivatedRandomly()
	},

	{ GAMEPLAY_MOD_BULLET_PHYSICS_ENEMIES_AND_PLAYER_ON_SLOWMOTION, GameplayMod( "bullet_physics_enemies_and_player_on_slowmotion", "Bullet physics for enemies and player on slowmotion" )
		.Description( "Bullet physics will be active for both enemies and player only when slowmotion is present." )
		.OnInit( []( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.bulletPhysicsMode = BULLET_PHYSICS_ENEMIES_AND_PLAYER_ON_SLOWMOTION;
		} )
		.CannotBeActivatedRandomly()
	},

	{ GAMEPLAY_MOD_BULLET_RICOCHET, GameplayMod( "bullet_ricochet", "Bullet ricochet" )
		.Description( "Physical bullets ricochet off the walls." )
		.Arguments( {
			Argument( "bullet_ricochet_count" ).MinMax( 0 ).RandomMinMax( 2, 20 ).Default( "2" ).Description( []( const std::string string, float value ) {
				return "Max ricochets: " + ( value <= 0 ? "Infinite" : std::to_string( value ) ) + "\n";
			} ),
			Argument( "bullet_ricochet_max_degree" ).MinMax( 1, 90 ).RandomMinMax( 30, 90 ).Default( "45" ).Description( []( const std::string string, float value ) {
				return "Max angle for ricochet: " + std::to_string( value ) + " deg \n";
			} ),
		} )
		.OnInit( []( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.bulletRicochetCount = args.at( 0 ).number;
			gameplayMods.bulletRicochetMaxDotProduct = args.at( 1 ).number / 90.0f;
			// TODO
			//gameplayMods.bulletRicochetError = 5;
		} )
		.OnDeactivation( []( CBasePlayer *player ) {
			gameplayMods.bulletRicochetCount = 0;
		} )
		.CanOnlyBeActivatedRandomlyWhen( []( CBasePlayer *player ) -> bool {
			return gameplayMods.bulletPhysicsMode == BULLET_PHYSICS_ENEMIES_AND_PLAYER_ON_SLOWMOTION ||
				gameplayMods.bulletPhysicsMode == BULLET_PHYSICS_CONSTANT;
		} )
	},

	{ GAMEPLAY_MOD_BULLET_SELF_HARM, GameplayMod( "bullet_self_harm", "Bullet self harm" )
		.Description( "Physical bullets shot by player can harm back (ricochet mod is required)." )
		.Toggles( &gameplayMods.bulletSelfHarm )
		.CanOnlyBeActivatedRandomlyWhen( []( CBasePlayer *player ) -> bool {
			return gameplayMods.bulletRicochetCount > 0;
		} )
	},

	{ GAMEPLAY_MOD_BULLET_TRAIL_CONSTANT, GameplayMod( "bullet_trail_constant", "Bullet trail constant" )
		.Description( "Physical bullets always get a trail, regardless if slowmotion is present." )
		.Toggles( &gameplayMods.bulletTrailConstant )
		.CanOnlyBeActivatedRandomlyWhen( []( CBasePlayer *player ) -> bool {
			return gameplayMods.bulletPhysicsMode == BULLET_PHYSICS_ENEMIES_AND_PLAYER_ON_SLOWMOTION ||
				gameplayMods.bulletPhysicsMode == BULLET_PHYSICS_CONSTANT;
		} )
	},

	{ GAMEPLAY_MOD_CONSTANT_SLOWMOTION, GameplayMod( "constant_slowmotion", "Constant slowmotion" )
		.Description( "You start in slowmotion, it's infinite and you can't turn it off." )
		.OnInit( []( CBasePlayer *player, const std::vector<Argument> &args ) {
	#ifndef CLIENT_DLL
			player->TakeSlowmotionCharge( 100 ),
			player->SetSlowMotion( true ),
	#endif
			gameplayMods.slowmotionInfinite = TRUE;
			gameplayMods.slowmotionConstant = TRUE;
		} )
		.OnDeactivation( []( CBasePlayer *player ) {
	#ifndef CLIENT_DLL
			player->SetSlowMotion( false ),
	#endif
			gameplayMods.slowmotionInfinite = FALSE;
			gameplayMods.slowmotionConstant = FALSE;
		} )
		.CannotBeActivatedRandomly()
	},

	{ GAMEPLAY_MOD_CROSSBOW_EXPLOSIVE_BOLTS, GameplayMod( "crossbow_explosive_bolts", "Crossbow explosive bolts" )
		.Description( "Crossbow bolts explode when they hit the wall." )
		.Toggles( &gameplayMods.crossbowExplosiveBolts )
		.CanOnlyBeActivatedRandomlyWhen( []( CBasePlayer *player ) -> bool {
#ifndef CLIENT_DLL
			return player->HasNamedPlayerItem( "weapon_crossbow" ) == TRUE;
#else
			return true;
#endif
		} )
	},

	{ GAMEPLAY_MOD_DETACHABLE_TRIPMINES, GameplayMod( "detachable_tripmines", "Detachable tripmines" )
		.Description( "Pressing USE button on attached tripmines will detach them." )
		.Arguments( {
			Argument( "detach_tripmines_instantly" ).IsOptional().Description( []( const std::string string, float value ) {
				return string == "instantly" ? "Tripmines will be INSTANTLY added to your inventory if possible" : "";
			} )
		} )
		.OnInit( []( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.detachableTripmines = TRUE;
			gameplayMods.detachableTripminesInstantly = args.at( 0 ).string == "instantly";
		} )
		.OnDeactivation( []( CBasePlayer *player ) {
			gameplayMods.detachableTripmines = FALSE;
		} )
		.CannotBeActivatedRandomly()
	},

	{ GAMEPLAY_MOD_DIVING_ALLOWED_WITHOUT_SLOWMOTION, GameplayMod( "diving_allowed_without_slowmotion", "Diving allowed without slowmotion" )
		.Description(
			"You're still allowed to dive even if you have no slowmotion charge.\n"
			"In that case you will dive without going into slowmotion."
		)
		.Toggles( &gameplayMods.divingAllowedWithoutSlowmotion )
		.CannotBeActivatedRandomly()
	},

	{ GAMEPLAY_MOD_DIVING_ONLY, GameplayMod( "diving_only", "Diving only" )
		.Description(
			"The only way to move around is by diving.\n"
			"This enables Infinite slowmotion by default.\n"
			"You can dive even when in crouch-like position, like when being in vents etc."
		)
		.Toggles( { &gameplayMods.divingOnly, &gameplayMods.slowmotionInfinite } )
	},

	{ GAMEPLAY_MOD_DRUNK_AIM, GameplayMod( "drunk_aim", "Drunk aim" )
		.Description( "Your aim becomes wobbly." )
		.Arguments( {
			Argument( "max_horizontal_wobble" ).IsOptional().MinMax( 0, 25.5 ).RandomMinMax( 2, 25.5 ).Default( "20" ).Description( []( const std::string string, float value ) {
				return "Max horizontal wobble: " + std::to_string( value ) + " deg\n";
			} ),
			Argument( "max_vertical_wobble" ).IsOptional().MinMax( 0, 25.5 ).RandomMinMax( 2, 25.5 ).Default( "5" ).Description( []( const std::string string, float value ) {
				return "Max vertical wobble: " + std::to_string( value ) + " deg\n";
			} ),
			Argument( "wobble_frequency" ).IsOptional().MinMax( 0.01 ).RandomMinMax( 0.05, 90 ).Default( "1" ).Description( []( const std::string string, float value ) {
				return "Wobble frequency: " + std::to_string( value ) + "\n";
			} ),
		} )
		.OnInit( []( CBasePlayer *player, const std::vector<Argument> &args ) { 
			gameplayMods.aimOffsetMaxX = args.at( 0 ).number;
			gameplayMods.aimOffsetMaxY = args.at( 1 ).number;
			gameplayMods.aimOffsetChangeFreqency = args.at( 2 ).number;
		} )
		.OnDeactivation( []( CBasePlayer *player ) { 
			gameplayMods.aimOffsetMaxX = 0;
			gameplayMods.aimOffsetMaxY = 0;
			gameplayMods.aimOffsetChangeFreqency = 0;
		} )
	},

	{ GAMEPLAY_MOD_DRUNK_LOOK, GameplayMod( "drunk_look", "Drunk look" )
		.Description( "Camera view becomes wobbly and makes aim harder." )
		.Arguments( {
			Argument( "drunkiness" ).IsOptional().MinMax( 0.1, 100 ).RandomMinMax( 25, 100 ).Default( "25" ).Description( []( const std::string string, float value ) {
				return "Drunkiness: " + std::to_string( value ) + "%%\n";
			} ),
		} )
		.OnInit( []( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.drunkiness = ( args.at( 0 ).number / 100.0f ) * 255;
		} )
		.OnDeactivation( []( CBasePlayer *player ) {
			gameplayMods.drunkiness = 0;
		} )
	},

	{ GAMEPLAY_MOD_EASY, GameplayMod( "easy", "Easy difficulty" )
		.Description( "Sets up easy level of difficulty." )
		.CannotBeActivatedRandomly()
	},

	{ GAMEPLAY_MOD_EDIBLE_GIBS, GameplayMod( "edible_gibs", "Edible gibs" )
		.Description( "Allows you to eat gibs by pressing USE when aiming at the gib, which restore your health by 5." )
		.Toggles( &gameplayMods.gibsEdible )
	},

	{ GAMEPLAY_MOD_EMPTY_SLOWMOTION, GameplayMod( "empty_slowmotion", "Empty slowmotion" )
		.Description( "Start with no slowmotion charge." )
		.CannotBeActivatedRandomly()
	},

	{ GAMEPLAY_MOD_FADING_OUT, GameplayMod( "fading_out", "Fading out" )
		.Description(
			"View is fading out, or in other words it's blacking out until you can't see almost anything.\n"
			"Take painkillers to restore the vision.\n"
			"Allows to take painkillers even when you have 100 health and enough time have passed since the last take."
		)
		.Arguments( {
			Argument( "fade_out_percent" ).IsOptional().MinMax( 0, 100 ).RandomMinMax( 50, 90 ).Default( "90" ).Description( []( const std::string string, float value ) {
				return "Fade out intensity: " + std::to_string( value ) + "%%\n";
			} ),
			Argument( "fade_out_update_period" ).IsOptional().MinMax( 0.01 ).RandomMinMax( 0.2, 0.5 ).Default( "0.5" ).Description( []( const std::string string, float value ) {
				return "Fade out update period: " + std::to_string( value ) + " sec \n";
			} )
		} )
		.OnInit( []( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.fadeOut = TRUE;
			gameplayMods.fadeOutThreshold = 255 - ( args.at( 0 ).number / 100.0f ) * 255;
			gameplayMods.fadeOutUpdatePeriod = args.at( 1 ).number;
		} )
		.OnDeactivation( []( CBasePlayer *player ) {
			gameplayMods.fadeOut = FALSE;
		} )
	},

	{ GAMEPLAY_MOD_FRICTION, GameplayMod( "friction", "Friction" )
		.Description( "Changes player's friction." )
		.Arguments( {
			Argument( "friction" ).IsOptional().MinMax( 0 ).RandomMinMax( 0, 1 ).Default( "4" ).Description( []( const std::string string, float value ) {
				return "Friction: " + std::to_string( value ) + "\n";
			} )
		} )
		.OnInit( []( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.frictionOverride = args.at( 0 ).number;
		} )
		.OnDeactivation( []( CBasePlayer *player ) {
			gameplayMods.frictionOverride = -1.0f;
		} )
	},

	{ GAMEPLAY_MOD_GARBAGE_GIBS, GameplayMod( "garbage_gibs", "Garbage gibs" )
		.Description( "Replaces all gibs with garbage." )
		.Toggles( &gameplayMods.gibsGarbage )
	},

	{ GAMEPLAY_MOD_GOD, GameplayMod( "god", "God mode" )
		.Description( "You are invincible and it doesn't count as a cheat." )
		.OnInit( []( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.godConstant = TRUE;
		} )
		.OnDeactivation( []( CBasePlayer *player ) {
			gameplayMods.godConstant = FALSE;
			player->pev->flags &= ~FL_GODMODE;
		} )
		.CannotBeActivatedRandomly()
	},

	{ GAMEPLAY_MOD_HARD, GameplayMod( "hard", "Hard difficulty" )
		.Description( "Sets up hard level of difficulty." )
		.CannotBeActivatedRandomly()
	},

	{ GAMEPLAY_MOD_HEADSHOTS, GameplayMod( "headshots", "Headshots" )
		.Description( "Headshots dealt to enemies become much more deadly." )
	},

	{ GAMEPLAY_MOD_INFINITE_AMMO, GameplayMod( "infinite_ammo", "Infinite ammo" )
		.Description( "All weapons get infinite ammo." )
		.Toggles( &gameplayMods.infiniteAmmo )
	},

	{ GAMEPLAY_MOD_INFINITE_AMMO_CLIP, GameplayMod( "infinite_ammo_clip", "Infinite ammo clip" )
		.Description( "Most weapons get an infinite ammo clip and need no reloading." )
		.OnInit( []( CBasePlayer *player, const std::vector<Argument> &args ) {
#ifndef CLIENT_DLL
			for ( int i = 0; i < MAX_ITEM_TYPES; i++ ) {
				CBasePlayerItem *pItem = player->m_rgpPlayerItems[i];

				while ( pItem ) {
					if ( CBasePlayerWeapon *weapon = dynamic_cast< CBasePlayerWeapon * >( pItem ) ) {
						if ( weapon->iMaxClip() > 0 ) {
							UTIL_SetWeaponClip( weapon->m_iId, weapon->iMaxClip() );
						}
						if ( weapon->iMaxClip2() > 0 ) {
							UTIL_SetWeaponClip( weapon->m_iId, weapon->iMaxClip(), weapon->iMaxClip2() );
						}
					}

					pItem = pItem->m_pNext;
				}

			}
#endif
			gameplayMods.infiniteAmmoClip = TRUE;
		} )
		.OnDeactivation( []( CBasePlayer *player ) {
			gameplayMods.infiniteAmmoClip = FALSE;
		} )
		.CanOnlyBeActivatedRandomlyWhen( []( CBasePlayer *player ) -> bool {
			static std::vector<std::string> clipBasedWeapons = {
				"weapon_9mmhandgun",
				"weapon_9mmhandgun_twin",
				"weapon_357",
				"weapon_9mmAR",
				"weapon_shotgun",
				"weapon_crossbow",
				"weapon_ingram",
				"weapon_ingram_twin"
			};

			for ( int i = 0; i < MAX_ITEM_TYPES; i++ ) {
				if (
					player->m_rgpPlayerItems[i] &&
					aux::ctr::includes( clipBasedWeapons, STRING( player->m_rgpPlayerItems[i]->pev->classname ) )
				) {
					return true;
				}
			}

			return false;
		} )
	},

	{ GAMEPLAY_MOD_INFINITE_PAINKILLERS, GameplayMod( "infinite_painkillers", "Infinite painkillers" )
		.Description( "Self explanatory." )
		.Toggles( &gameplayMods.painkillersInfinite )
		.CannotBeActivatedRandomly()
	},

	{ GAMEPLAY_MOD_INFINITE_SLOWMOTION, GameplayMod( "infinite_slowmotion", "Infinite slowmotion" )
		.Description( "You have infinite slowmotion charge and it's not considered as cheat." )
		.OnInit( []( CBasePlayer *player, const std::vector<Argument> &args ) {
	#ifndef CLIENT_DLL
			player->TakeSlowmotionCharge( 100 ),
	#endif
			gameplayMods.slowmotionInfinite = TRUE;
		} )
		.OnDeactivation( []( CBasePlayer *player ) {
			gameplayMods.slowmotionInfinite = FALSE;
		} )
		.CanOnlyBeActivatedRandomlyWhen( []( CBasePlayer *player ) -> bool {
			return !gameplayMods.slowmotionForbidden;
		} )
	},

	{ GAMEPLAY_MOD_INITIAL_CLIP_AMMO, GameplayMod( "initial_clip_ammo", "Initial ammo clip" )
		.Description( "All weapons will have specified ammount of ammo in the clip when first picked up." )
		.Arguments( {
			Argument( "initial_clip_ammo" ).IsOptional().MinMax( 1 ).Default( "4" ).Description( []( const std::string string, float value ) {
				return "Initial ammo in the clip: " + std::to_string( value ) + "\n";
			} )
		} )
		.OnInit( []( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.initialClipAmmo = args.at( 0 ).number;
		} )
		.CannotBeActivatedRandomly()
	},

	{ GAMEPLAY_MOD_INSTAGIB, GameplayMod( "instagib", "Instagib" )
		.Description(
			"Gauss Gun becomes much more deadly with 9999 damage, also gets red beam and slower rate of fire.\n"
			"More gibs come out."
		)
		.Toggles( &gameplayMods.instaGib )
		.CanOnlyBeActivatedRandomlyWhen( []( CBasePlayer *player ) -> bool {
#ifndef CLIENT_DLL
			return player->HasNamedPlayerItem( "weapon_gauss" ) == TRUE;
#else
			return true;
#endif
		} )
	},

	{ GAMEPLAY_MOD_HEALTH_REGENERATION, GameplayMod( "health_regeneration", "Health regeneration" )
		.Description( "Allows for health regeneration options." )
		.Arguments( {
			Argument( "regeneration_max" ).IsOptional().MinMax( 0 ).Default( "20" ).Description( []( const std::string string, float value ) {
				return "Regenerate up to: " + std::to_string( value ) + " HP\n";
			} ),
			Argument( "regeneration_delay" ).IsOptional().MinMax( 0 ).Default( "3" ).Description( []( const std::string string, float value ) {
				return std::to_string( value ) + " sec after last damage\n";
			} ),
			Argument( "regeneration_frequency" ).IsOptional().MinMax( 0.01 ).Default( "0.2" ).Description( []( const std::string string, float value ) {
				return "Regenerating: " + std::to_string( 1 / value ) + " HP/sec\n";
			} )
		} )
		.OnInit( []( CBasePlayer *player, const std::vector<Argument> &args ) {
			player->pev->max_health = args.at( 0 ).number;
			gameplayMods.regenerationMax = args.at( 0 ).number;
			gameplayMods.regenerationDelay = args.at( 1 ).number;
			gameplayMods.regenerationFrequency = args.at( 2 ).number;
		} )
		.CannotBeActivatedRandomly()
	},

	{ GAMEPLAY_MOD_HEAL_ON_KILL, GameplayMod( "heal_on_kill", "Heal on kill" )
		.Description( "Your health will be replenished after killing an enemy." )
		.Arguments( {
			Argument( "max_health_taken_percent" ).IsOptional().MinMax( 1, 5000 ).RandomMinMax( 100, 110 ).Default( "25" ).Description( []( const std::string string, float value ) {
				return "Victim's max health taken after kill: " + std::to_string( value ) + "%%\n";
			} ),
		} )
		.OnInit( []( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.healOnKill = TRUE;
			gameplayMods.healOnKillMultiplier = args.at( 0 ).number / 100.0f;
		} )
		.OnDeactivation( []( CBasePlayer *player ) {
			gameplayMods.healOnKill = FALSE;
		} )
	},

	{ GAMEPLAY_MOD_NO_FALL_DAMAGE, GameplayMod( "no_fall_damage", "No fall damage" )
		.Description( "Self explanatory." )
		.Toggles( &gameplayMods.noFallDamage )
	},

	{ GAMEPLAY_MOD_NO_MAP_MUSIC, GameplayMod( "no_map_music", "No map music" )
		.Description( "Music which is defined by map will not be played.\nOnly the music defined in gameplay config files will play." )
		.Toggles( &gameplayMods.noMapMusic )
		.CannotBeActivatedRandomly()
	},

	{ GAMEPLAY_MOD_NO_HEALING, GameplayMod( "no_healing", "No healing" )
		.Description( "Don't allow to heal in any way, including Xen healing pools." )
		.Toggles( &gameplayMods.noHealing )
		.CannotBeActivatedRandomly()
	},

	{ GAMEPLAY_MOD_NO_JUMPING, GameplayMod( "no_jumping", "No jumping" )
		.Description( "Don't allow to jump." )
		.Toggles( &gameplayMods.noJumping )
		.CannotBeActivatedRandomly()
	},

	{ GAMEPLAY_MOD_NO_PILLS, GameplayMod( "no_pills", "No painkillers" )
		.Description( "Don't allow to take painkillers." )
		.Toggles( &gameplayMods.painkillersForbidden )
		.CannotBeActivatedRandomly()
	},

	{ GAMEPLAY_MOD_NO_SAVING, GameplayMod( "no_saving", "No saving" )
		.Description( "Don't allow to load saved files." )
		.Toggles( &gameplayMods.noSaving )
		.CannotBeActivatedRandomly()
	},

	{ GAMEPLAY_MOD_NO_SECONDARY_ATTACK, GameplayMod( "no_secondary_attack", "No secondary attack" )
		.Description( "Disables the secondary attack on all weapons." )
		.Toggles( &gameplayMods.noSecondaryAttack )
		.CanOnlyBeActivatedRandomlyWhen( []( CBasePlayer *player ) -> bool {
#ifndef CLIENT_DLL
			static std::vector<const char *> weaponsWithSecondaryAttack = { 
				"weapon_shotgun",
				"weapon_9mmAR",
				"weapon_crossbow",
				"weapon_gauss"
			};
			return std::any_of( weaponsWithSecondaryAttack.begin(), weaponsWithSecondaryAttack.end(), [player]( const char *weapon ) {
				return player->HasNamedPlayerItem( weapon );
			} );
#else
			return true;
#endif
		} )
	},

	{ GAMEPLAY_MOD_NO_SELF_GAUSS, GameplayMod( "no_self_gauss", "No self gauss" )
		.Description( "Prevents self gauss effect." )
		.Toggles( &gameplayMods.noSelfGauss )
		.CannotBeActivatedRandomly()
	},

	{ GAMEPLAY_MOD_NO_SLOWMOTION, GameplayMod( "no_slowmotion", "No slowmotion" )
		.Description( "You're not allowed to use slowmotion." )
		.OnInit( []( CBasePlayer *player, const std::vector<Argument> &args ) {
#ifndef CLIENT_DLL
			player->DeactivateSlowMotion();
			player->slowMotionCharge = 0;
#endif
			gameplayMods.slowmotionForbidden = TRUE;
		} )
		.OnDeactivation( []( CBasePlayer *player ) {
			gameplayMods.slowmotionForbidden = FALSE;
		} )
		.CanOnlyBeActivatedRandomlyWhen( []( CBasePlayer *player ) -> bool {
			return !gameplayMods.slowmotionInfinite;
		} )
	},

	{ GAMEPLAY_MOD_NO_SMG_GRENADE_PICKUP, GameplayMod( "no_smg_grenade_pickup", "No SMG grenade pickup" )
		.Description( "You're not allowed to pickup and use SMG (MP5) grenades." )
		.Toggles( &gameplayMods.noSmgGrenadePickup )
		.CannotBeActivatedRandomly()
	},

	{ GAMEPLAY_MOD_NO_TARGET, GameplayMod( "no_target", "No target" )
		.Description( "You are invisible to everyone and it doesn't count as a cheat." )
		.OnInit( []( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.noTargetConstant = TRUE;
		} )
		.OnDeactivation( []( CBasePlayer *player ) {
			gameplayMods.noTargetConstant = FALSE;
			player->pev->flags &= ~FL_NOTARGET;
		} )
		.CannotBeActivatedRandomly()
	},

	{ GAMEPLAY_MOD_NO_WALKING, GameplayMod( "no_walking", "No walking" )
		.Description( "Don't allow to walk, swim, dive, climb ladders." )
		.Toggles( &gameplayMods.noWalking )
		.CannotBeActivatedRandomly()
	},

	{ GAMEPLAY_MOD_ONE_HIT_KO, GameplayMod( "one_hit_ko", "One hit KO" )
		.Description(
			"Any hit from an enemy will kill you instantly.\n"
			"You still get proper damage from falling and environment."
		)
		.Toggles( &gameplayMods.oneHitKO )
	},

	{ GAMEPLAY_MOD_ONE_HIT_KO_FROM_PLAYER, GameplayMod( "one_hit_ko_from_player", "One hit KO from player" )
		.Description( "All enemies die in one hit." )
		.Toggles( &gameplayMods.oneHitKOFromPlayer )
		.CannotBeActivatedRandomly()
	},

	{ GAMEPLAY_MOD_PREVENT_MONSTER_MOVEMENT, GameplayMod( "prevent_monster_movement", "Prevent monster movement" )
		.Description( "Monsters will always stay at spawn spot." )
		.Toggles( &gameplayMods.preventMonsterMovement )
		.CannotBeActivatedRandomly()
	},

	{ GAMEPLAY_MOD_PREVENT_MONSTER_SPAWN, GameplayMod( "prevent_monster_spawn", "Prevent monster spawn" )
		.Description(
			"Don't spawn predefined monsters (NPCs) when visiting a new map.\n"
			"This doesn't affect dynamic monster_spawners."
		)
		.Toggles( &gameplayMods.preventMonsterSpawn )
		.CannotBeActivatedRandomly()
	},

	{ GAMEPLAY_MOD_PREVENT_MONSTER_STUCK_EFFECT, GameplayMod( "prevent_monster_stuck_effect", "Prevent monster stuck effect" )
		.Description( "If monsters get stuck after spawning, they won't have the usual yellow particles effect." )
		.Toggles( &gameplayMods.preventMonsterStuckEffect )
		.CannotBeActivatedRandomly()
	},

	{ GAMEPLAY_MOD_PREVENT_MONSTER_DROPS, GameplayMod( "prevent_monster_drops", "Prevent monster drops" )
		.Description( "Monsters won't drop anything when dying." )
		.Toggles( &gameplayMods.preventMonsterDrops )
		.CannotBeActivatedRandomly()
	},

	{ GAMEPLAY_MOD_SCORE_ATTACK, GameplayMod( "score_attack", "Score attack" )
		.Description( "Kill enemies to get as much score as possible. Build combos to get even more score." )
		.Toggles( &gameplayMods.scoreAttack )
		.CannotBeActivatedRandomly()
	},

	{ GAMEPLAY_MOD_RANDOM_GAMEPLAY_MODS, GameplayMod( "random_gameplay_mods", "Random gameplay mods" )
		.Description( "Random gameplay mods." )
		.Arguments( {
			Argument( "time_for_random_gameplay_mod" ).IsOptional().MinMax( 1 ).Default( "60" ).Description( []( const std::string string, float value ) {
				return "Random mod will last for " + std::to_string( value ) + " sec \n";
			} ),
			Argument( "time_until_next_random_gameplay_mod" ).IsOptional().MinMax( 2 ).Default( "180" ).Description( []( const std::string string, float value ) {
				return "Random mods will be applied every: " + std::to_string( value ) + " sec \n";
			} ),
			Argument( "time_for_random_gameplay_mod_voting" ).IsOptional().MinMax( 0 ).Default( "30" ).Description( []( const std::string string, float value ) {
				return "Upcoming random mods will be shown for: " + std::to_string( value ) + " sec \n";
			} )
		} )
		.OnInit( []( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.randomGameplayMods = TRUE;
			gameplayMods.timeForRandomGameplayMod = args.at( 0 ).number;
			gameplayMods.timeUntilNextRandomGameplayMod = args.at( 1 ).number;
			gameplayMods.timeLeftUntilNextRandomGameplayMod = args.at( 1 ).number;
			gameplayMods.timeForRandomGameplayModVoting = min( gameplayMods.timeUntilNextRandomGameplayMod, args.at( 2 ).number );
		} )
		.CannotBeActivatedRandomly()
	},

	{ GAMEPLAY_MOD_SHOTGUN_AUTOMATIC, GameplayMod( "shotgun_automatic", "Automatic shotgun" )
		.Description( "Shotgun only fires single shots and doesn't have to be reloaded after each shot." )
		.Toggles( &gameplayMods.automaticShotgun )
		.CanOnlyBeActivatedRandomlyWhen( []( CBasePlayer *player ) -> bool {
#ifndef CLIENT_DLL
			return player->HasNamedPlayerItem( "weapon_shotgun" ) == TRUE;
#else
			return true;
#endif
		} )
	},

	{ GAMEPLAY_MOD_SHOOT_UNDERWATER, GameplayMod( "shoot_underwater", "Shoot underwater" )
		.Description( "All weapons now can shoot underwater" )
		.Toggles( &gameplayMods.shootUnderwater )
		.CannotBeActivatedRandomly()
	},

	{ GAMEPLAY_MOD_SHOW_TIMER, GameplayMod( "show_timer", "Show timer" )
		.Description( "Timer will be shown. Time is affected by slowmotion." )
		.Toggles( &gameplayMods.timerShown )
		.CannotBeActivatedRandomly()
	},

	{ GAMEPLAY_MOD_SHOW_TIMER_REAL_TIME, GameplayMod( "show_timer_real_time", "Show timer with real time" )
		.Description( "Time will be shown and it's not affected by slowmotion, which is useful for speedruns." )
		.Toggles( { &gameplayMods.timerShown, &gameplayMods.timerShowReal } )
		.CannotBeActivatedRandomly()
	},

	{ GAMEPLAY_MOD_SLOWMOTION_FAST_WALK, GameplayMod( "slowmotion_fast_walk", "Fast walk in slowmotion" )
		.Description( "You still walk and run almost as fast as when slowmotion is not active." )
		.Toggles( &gameplayMods.slowmotionFastWalk )
		.CanOnlyBeActivatedRandomlyWhen( []( CBasePlayer *player ) -> bool {
			return !gameplayMods.slowmotionForbidden;
		} )
	},

	{ GAMEPLAY_MOD_SLOWMOTION_ON_DAMAGE, GameplayMod( "slowmotion_on_damage", "Slowmotion on damage" )
		.Description( "You get slowmotion charge when receiving damage." )
		.Toggles( &gameplayMods.slowmotionOnDamage )
		.CanOnlyBeActivatedRandomlyWhen( []( CBasePlayer *player ) -> bool {
			return !gameplayMods.slowmotionForbidden;
		} )
	},

	{ GAMEPLAY_MOD_SLOWMOTION_ONLY_DIVING, GameplayMod( "slowmotion_only_diving", "Slowmotion only when diving" )
		.Description( "You're allowed to go into slowmotion only by diving." )
		.Toggles( &gameplayMods.slowmotionOnlyDiving )
		.CanOnlyBeActivatedRandomlyWhen( []( CBasePlayer *player ) -> bool {
			return !gameplayMods.slowmotionForbidden;
		} )
	},

	{ GAMEPLAY_MOD_SLOW_PAINKILLERS, GameplayMod( "slow_painkillers", "Slow painkillers" )
		.Description( "Painkillers take time to have an effect, like in original Max Payne." )
		.Arguments( {
			Argument( "healing_period" ).IsOptional().MinMax( 0.01 ).RandomMinMax( 0.1, 0.3 ).Default( "0.2" ).Description( []( const std::string string, float value ) {
				return "Healing period " + std::to_string( value ) + " sec\n";
			} )
		} )
		.OnInit( []( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.painkillersSlow = TRUE;
			player->nextPainkillerEffectTimePeriod = args.at( 0 ).number;
		} )
		.OnDeactivation( []( CBasePlayer *player ) {
			gameplayMods.painkillersSlow = FALSE;
		} )
	},

	{ GAMEPLAY_MOD_SNARK_FRIENDLY_TO_ALLIES, GameplayMod( "snark_friendly_to_allies", "Snarks friendly to allies" )
		.Description( "Snarks won't attack player's allies." )
		.Toggles( &gameplayMods.snarkFriendlyToAllies )
		.CannotBeActivatedRandomly()
	},

	{ GAMEPLAY_MOD_SNARK_FRIENDLY_TO_PLAYER, GameplayMod( "snark_friendly_to_player", "Snarks friendly to player" )
		.Description( "Snarks won't attack player." )
		.Toggles( &gameplayMods.snarkFriendlyToPlayer )
		.CanOnlyBeActivatedRandomlyWhen( []( CBasePlayer *player ) -> bool {
#ifndef CLIENT_DLL
			return gameplayMods.snarkInfestation || gameplayMods.snarkFromExplosion || player->HasNamedPlayerItem( "weapon_snark" );
#else
			return true;
#endif
		} )
	},

	{ GAMEPLAY_MOD_SNARK_FROM_EXPLOSION, GameplayMod( "snark_from_explosion", "Snark from explosion" )
		.Description( "Snarks will spawn in the place of explosion." )
		.Toggles( &gameplayMods.snarkFromExplosion )
	},

	{ GAMEPLAY_MOD_SNARK_INCEPTION, GameplayMod( "snark_inception", "Snark inception" )
		.Description( "Killing snark splits it into two snarks." )
		.Arguments( {
			Argument( "inception_depth" ).IsOptional().MinMax( 1, 100 ).RandomMinMax( 1, 1 ).Default( "10" ).Description( []( const std::string string, float value ) {
				return "Inception depth: " + std::to_string( value ) + " snarks\n";
			} )
		} )
		.OnInit( []( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.snarkInception = TRUE;
			gameplayMods.snarkInceptionDepth = args.at( 0 ).number;
		} )
		.OnDeactivation( []( CBasePlayer *player ) {
			gameplayMods.snarkInception = FALSE;
		} )
		.CanOnlyBeActivatedRandomlyWhen( []( CBasePlayer *player ) -> bool {
#ifndef CLIENT_DLL
			return gameplayMods.snarkInfestation || gameplayMods.snarkFromExplosion || player->HasNamedPlayerItem( "weapon_snark" );
#else
			return true;
#endif
		} )
	},

	{ GAMEPLAY_MOD_SNARK_INFESTATION, GameplayMod( "snark_infestation", "Snark infestation" )
		.Description(
			"Snark will spawn in the body of killed monster (NPC).\n"
			"Even more snarks spawn if monster's corpse has been gibbed."
		)
		.Toggles( &gameplayMods.snarkInfestation )
	},

	{ GAMEPLAY_MOD_SNARK_NUCLEAR, GameplayMod( "snark_nuclear", "Snark nuclear" )
		.Description( "Killing snark produces a grenade-like explosion." )
		.Toggles( &gameplayMods.snarkNuclear )
		.CanOnlyBeActivatedRandomlyWhen( []( CBasePlayer *player ) -> bool {
#ifndef CLIENT_DLL
			return gameplayMods.snarkInfestation || gameplayMods.snarkFromExplosion || player->HasNamedPlayerItem( "weapon_snark" );
#else
			return true;
#endif
		} )
	},

	{ GAMEPLAY_MOD_SNARK_PENGUINS, GameplayMod( "snark_penguins", "Snark penguins" )
		.Description( "Replaces snarks with penguins from Opposing Force.\n" )
		.Toggles( &gameplayMods.snarkPenguins )
		.CanOnlyBeActivatedRandomlyWhen( []( CBasePlayer *player ) -> bool {
#ifndef CLIENT_DLL
			return gameplayMods.snarkInfestation || gameplayMods.snarkFromExplosion || player->HasNamedPlayerItem( "weapon_snark" );
#else
			return true;
#endif
		} )
	},

	{ GAMEPLAY_MOD_SNARK_STAY_ALIVE, GameplayMod( "snark_stay_alive", "Snark stay alive" )
		.Description( "Snarks will never die on their own, they must be shot." )
		.Toggles( &gameplayMods.snarkStayAlive )
		.CanOnlyBeActivatedRandomlyWhen( []( CBasePlayer *player ) -> bool {
#ifndef CLIENT_DLL
			return gameplayMods.snarkInfestation || gameplayMods.snarkFromExplosion || player->HasNamedPlayerItem( "weapon_snark" );
#else
			return true;
#endif
		} )
	},

	{ GAMEPLAY_MOD_STARTING_HEALTH, GameplayMod( "starting_health", "Starting Health" )
		.Description( "Start with specified health amount." )
		.Arguments( {
			Argument( "health_amount" ).IsOptional().MinMax( 1 ).Default( "100" ).Description( []( const std::string string, float value ) {
				return "Health amount: " + std::to_string( value ) + "\n";
			} )
		} )
		.OnInit( []( CBasePlayer *player, const std::vector<Argument> &args ) {
			player->pev->health = args.at( 0 ).number;
		} )
		.CannotBeActivatedRandomly()
	},

	{ GAMEPLAY_MOD_SUPERHOT, GameplayMod( "superhot", "SUPERHOT" )
		.Description(
			"Time moves forward only when you move around.\n"
			"Inspired by the game SUPERHOT."
		)
		.OnInit( []( CBasePlayer *player, const std::vector<Argument> &args ) {
#ifndef CLIENT_DLL
			player->SetSlowMotion( false );
#endif
			gameplayMods.superHot = TRUE;
		} )
		.OnDeactivation( []( CBasePlayer *player ) {
#ifndef CLIENT_DLL
			player->SetSlowMotion( false );
			player->desiredTimeScale = 1.0f;
#endif
			gameplayMods.superHot = FALSE;
		} )
	},

	{ GAMEPLAY_MOD_SWEAR_ON_KILL, GameplayMod( "swear_on_kill", "Swear on kill" )
		.Description( "Max will swear when killing an enemy. He will still swear even if Max's commentary is turned off." )
		.Toggles( &gameplayMods.swearOnKill )
	},

	{ GAMEPLAY_MOD_UPSIDE_DOWN, GameplayMod( "upside_down", "Upside down" )
		.Description( "View becomes turned on upside down." )
		.OnInit( []( CBasePlayer *player, const std::vector<Argument> &args ) {
#ifndef CLIENT_DLL
			player->upsideDownMessageSent = false;
#endif
			gameplayMods.upsideDown = TRUE;
		} )
		.OnDeactivation( []( CBasePlayer *player ) {
#ifndef CLIENT_DLL
			player->upsideDownMessageSent = false;
#endif
			gameplayMods.upsideDown = FALSE;
		} )
	},

	{ GAMEPLAY_MOD_TELEPORT_MAINTAIN_VELOCITY, GameplayMod( "teleport_maintain_velocity", "Teleport maintain velocity" )
		.Description( "Your velocity will be preserved after going through teleporters." )
		.Toggles( &gameplayMods.teleportMaintainVelocity )
		.CannotBeActivatedRandomly()
	},

	{ GAMEPLAY_MOD_TELEPORT_ON_KILL, GameplayMod( "teleport_on_kill", "Teleport on kill" )
		.Description( "You will be teleported to the enemy you kill." )
		.Arguments( {
			Argument( "teleport_weapon" ).IsOptional().Description( []( const std::string string, float value ) {
				return "Weapon that causes teleport: " + ( string.empty() ? "any" : string ) + "\n";
			} )
		} )
		.OnInit( []( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.teleportOnKill = TRUE;

			std::string weapon = args.at( 0 ).string;
			if ( !weapon.empty() ) {
				snprintf( gameplayMods.teleportOnKillWeapon, 64, "%s", weapon.c_str() );
			}
		} )
		.OnDeactivation( []( CBasePlayer *player ) {
			gameplayMods.teleportOnKill = FALSE;
		} )
	},

	{ GAMEPLAY_MOD_TIME_RESTRICTION, GameplayMod( "time_restriction", "Time restriction" )
		.Description( "You are killed if time runs out." )
		.Arguments( {
			Argument( "time" ).IsOptional().MinMax( 1 ).Default( "60" ).Description( []( const std::string string, float value ) {
				return std::to_string( value ) + " seconds\n";
			} ),
		} )
		.OnInit( []( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.timerShown = TRUE;
			gameplayMods.timerBackwards = TRUE;
			gameplayMods.time = args.at( 0 ).number;
		} )
		.OnDeactivation( []( CBasePlayer *player ) {
			gameplayMods.timerShown = FALSE;
			gameplayMods.timerBackwards = FALSE;
		} )
		.CannotBeActivatedRandomly()
	},

	{ GAMEPLAY_MOD_VVVVVV, GameplayMod( "vvvvvv", "VVVVVV" )
		.Description(
			"Pressing jump reverses gravity for player.\n"
			"Inspired by the game VVVVVV."
		)
		.Toggles( &gameplayMods.vvvvvv )
		.CannotBeActivatedRandomly()
	},

	{ GAMEPLAY_MOD_WEAPON_IMPACT, GameplayMod( "weapon_impact", "Weapon impact" )
		.Description(
			"Taking damage means to be pushed back"
		)
		.Arguments( {
			Argument( "impact" ).IsOptional().MinMax( 1 ).RandomMinMax( 3, 8 ).Default( "1" ).Description( []( const std::string string, float value ) {
				return "Impact multiplier: " + std::to_string( value ) + "\n";
			} ),
		} )
		.OnInit( []( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.weaponImpact = args.at( 0 ).number;
		} )
		.OnDeactivation( []( CBasePlayer *player ) {
			gameplayMods.weaponImpact = 0.0f;
		} )
	},

	{ GAMEPLAY_MOD_WEAPON_PUSH_BACK, GameplayMod( "weapon_push_back", "Weapon push back" )
		.Description(
			"Shooting weapons pushes you back."
		)
		.Arguments( {
			Argument( "weapon_push_back_multiplier" ).IsOptional().MinMax( 0.1 ).RandomMinMax( 0.8, 1.2 ).Default( "1" ).Description( []( const std::string string, float value ) {
				return "Push back multiplier: " + std::to_string( value ) + "\n";
			} ),
		} )
		.OnInit( []( CBasePlayer *player, const std::vector<Argument> &args ) {
			gameplayMods.weaponPushBack = TRUE;
			gameplayMods.weaponPushBackMultiplier = args.at( 0 ).number;
		} )
		.OnDeactivation( []( CBasePlayer *player ) {
			gameplayMods.weaponPushBack = FALSE;
		} )
	},

	{ GAMEPLAY_MOD_WEAPON_RESTRICTED, GameplayMod( "weapon_restricted", "Weapon restricted" )
		.Description(
			"If you have no weapons - you are allowed to pick only one.\n"
			"You can have several weapons at once if they are specified in [loadout] section.\n"
			"Weapon stripping doesn't affect you."
		)
		.Toggles( &gameplayMods.weaponRestricted )
		.CannotBeActivatedRandomly()
	},



	{ GAMEPLAY_MOD_EVENT_GIVE_RANDOM_WEAPON, GameplayMod( "event_give_random_weapon", "Give random weapon" )
		.OnEventInit( []( CBasePlayer *player, const std::vector<Argument> &args ) -> std::pair<std::string, std::string> {
#ifndef CLIENT_DLL
			static std::vector<std::pair<const char *, const char *>> allowedRandomWeapons = {
				{ "weapon_9mmhandgun", "Beretta" },
				{ "weapon_shotgun", "Shotgun" },
				{ "weapon_9mmAR", "SMG" },
				{ "weapon_handgrenade", "Hand grenades" },
				{ "weapon_tripmine", "Tripmine" },
				{ "weapon_357", "Deagle" },
				{ "weapon_crossbow", "Crossbow" },
				{ "weapon_egon", "Gluon gun" },
				{ "weapon_gauss", "Gauss gun" },
				{ "weapon_rpg", "RPG" },
				{ "weapon_satchel", "Remote explosive" },
				{ "weapon_snark", "Snarks" },
				{ "weapon_ingram", "Ingram MAC-10" }
			};

			float hud_autoswitch = CVAR_GET_FLOAT( "hud_autoswitch" );
			auto &weapon = aux::rand::choice( allowedRandomWeapons );
			CVAR_SET_FLOAT( "hud_autoswitch", 0.0f );
			player->GiveNamedItem( weapon.first, true );
			CVAR_SET_FLOAT( "hud_autoswitch", hud_autoswitch );

			return { "Received weapon", weapon.second };
#else
			return { "", "" };
#endif
		} )
	},

	{ GAMEPLAY_MOD_EVENT_SPAWN_RANDOM_MONSTERS, GameplayMod( "event_spawn_random_monsters", "Spawn random monsters" )
		.OnEventInit( []( CBasePlayer *player, const std::vector<Argument> &args ) -> std::pair<std::string, std::string> {
#ifndef CLIENT_DLL

			static std::map<std::string, std::string> formattedMonsterNames = {
				{ "monster_alien_controller", "Alien controllers" },
				{ "monster_alien_grunt", "Alien grunts" },
				{ "monster_alien_slave", "Vortiguants" },
				{ "monster_bullchicken", "Bullsquids" },
				{ "monster_headcrab", "Headcrabs" },
				{ "monster_houndeye", "Houndeyes" },
				{ "monster_human_assassin", "Assassins" },
				{ "monster_human_grunt", "Grunts" },
				{ "monster_snark", "Snarks" },
				{ "monster_zombie", "Zombies" }
			};

			std::set<std::string> allowedRandomMonsters;
			for ( auto &monsterName : formattedMonsterNames ) {
				allowedRandomMonsters.insert( monsterName.first );
			}

			std::set<std::string> existingEntities;
			std::set<std::string> spawnedEntities;

			CBaseEntity *pEntity = NULL;
			while ( ( pEntity = UTIL_FindEntityInSphere( pEntity, player->pev->origin, 8192.0f ) ) != NULL ) {
				existingEntities.insert( STRING( pEntity->pev->classname ) );
			}

			std::vector<std::string> allowedMonstersToSpawn;
			std::set_intersection(
				allowedRandomMonsters.begin(), allowedRandomMonsters.end(),
				existingEntities.begin(), existingEntities.end(),
				std::back_inserter( allowedMonstersToSpawn )
			);

			if ( allowedMonstersToSpawn.empty() ) {
				allowedMonstersToSpawn.push_back( "monster_snark" );
			}

			int amountOfMonsters = aux::rand::uniformInt( 3, 10 );
			for ( int i = 0; i < amountOfMonsters; i++ ) {
				auto randomMonsterName = aux::rand::choice( allowedMonstersToSpawn );
				if ( randomMonsterName == "monster_human_grunt" ) {
					auto choice = aux::rand::choice<std::vector, std::string>( {
						"monster_human_grunt",
						"monster_human_grunt_shotgun",
						"monster_human_grunt_grenade_launcher"
					} );
				}

				if ( CHalfLifeRules *rules = dynamic_cast< CHalfLifeRules * >( g_pGameRules ) ) {
					CBaseEntity *entity = NULL;
					EntitySpawnData spawnData;
					spawnData.name = randomMonsterName;
					spawnData.UpdateSpawnFlags();
					do {
						spawnData.DetermineBestSpawnPosition( player );
						
						entity = rules->SpawnBySpawnData( spawnData );
					} while ( !entity );
					spawnedEntities.insert( spawnData.name );
				}
			}

			std::string description;
			for ( auto i = spawnedEntities.begin(); i != spawnedEntities.end(); ) {
				description += formattedMonsterNames[*i];
				i++;
				if ( i != spawnedEntities.end() ) {
					description += ", ";
				}
			}

			return { "Some monsters have been spawned", description };
#else
			return { "", "" };
#endif
		} )
		.CanOnlyBeActivatedRandomlyWhen( []( CBasePlayer *player ) -> bool {
#ifndef CLIENT_DLL
			CBaseEntity *pEntity = NULL;
			while ( ( pEntity = UTIL_FindEntityInSphere( pEntity, player->pev->origin, 8192.0f ) ) != NULL ) {
				if ( std::string( STRING( pEntity->pev->classname ) ) == "player_loadsaved" ) {
					return false;
				}
			}
#endif
			return true;
		} )
		.CanBeCancelledAfterChangeLevel()
	},
};