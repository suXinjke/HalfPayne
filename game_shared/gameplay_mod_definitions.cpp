#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "custom_gamemode_config.h"
#include "weapons.h"
#include <algorithm>
#include <iterator>
#include "gamerules.h"
#include "../fmt/printf.h"

#ifdef CLIENT_DLL
extern CustomGameModeConfig clientConfig;
#endif

using namespace gameplayMods;
int g_autoSaved = 0;

static std::vector<const char *> weaponsWithBullets = {
	"weapon_9mmhandgun",
	"weapon_9mmhandgun_twin",
	"weapon_357",
	"weapon_shotgun",
	"weapon_9mmAR",
	"weapon_ingram",
	"weapon_ingram_dual",
	"weapon_m249"
};

GameplayMod &::accuracy = GameplayMod::Define( "accuracy", "Accuracy" )
.Description( "Some of your weapons become less or more accurate" )
.Arguments( {
	Argument( "accuracy" ).MinMax( 0.01, 10 ).Default( "1" ).Description( []( const std::string string, float value ) {
		return fmt::sprintf( "Spread is multiplied by: %.1f", value );
	} ),
} )
.IsAlsoActiveWhen( []() -> std::optional<std::string> {
	if ( ::accuracyLow.isActive() ) {
		return "1.66";
	} else if ( ::accuracyHigh.isActive() ) {
		return "0.33";
	}

	return std::nullopt;
} )
.CannotBeActivatedRandomly();

GameplayMod &::accuracyLow = GameplayMod::Define( "low_accuracy", "Low accuracy" )
.Description( "Some of your weapons become inaccurate" )
.CanOnlyBeActivatedRandomlyWhen( []() -> bool {
	bool hasBulletBasedWeapon = false;
	if ( auto player = GetPlayer() ) {
		for ( auto weapon : weaponsWithBullets ) {
			if ( player->HasNamedPlayerItem( weapon, true ) ) {
				hasBulletBasedWeapon = true;
				break;
			}
		}
	}

	return hasBulletBasedWeapon && !::accuracyHigh.isActive();
} );

GameplayMod &::accuracyHigh = GameplayMod::Define( "high_accuracy", "Super accuracy" )
.Description( "Some of your weapons become super accurate" )
.CanOnlyBeActivatedRandomlyWhen( []() -> bool {
	bool hasBulletBasedWeapon = false;
	if ( auto player = GetPlayer() ) {
		for ( auto weapon : weaponsWithBullets ) {
			if ( player->HasNamedPlayerItem( weapon, true ) ) {
				hasBulletBasedWeapon = true;
				break;
			}
		}
	}

	return hasBulletBasedWeapon && !::accuracyLow.isActive();
} );

GameplayMod &::autoSavesOnly = GameplayMod::Define( "autosaves_only", "Autosaves only" )
.Description( "Only autosaves are allowed" )
.CannotBeActivatedRandomly();

GameplayMod &::blackMesaMinute = GameplayMod::Define( "black_mesa_minute", "Black Mesa Minute" )
.Description( "Time-based game mode - rush against a minute and kill enemies to get more time" )
.CannotBeActivatedRandomly();;

GameplayMod &::bleeding = GameplayMod::Define( "bleeding", "Bleeding" )
.Description( "You're gradually losing health until certain limit" )
.Arguments( {
	Argument( "bleeding_handicap" ).IsOptional().MinMax( 0, 99 ).RandomMinMax( 10, 50 ).Default( "20" ).Description( []( const std::string string, float value ) {
		return fmt::sprintf( "Bleeding until %.0f health left", value );
	} ),
	Argument( "bleeding_update_period" ).IsOptional().MinMax( 0.01 ).RandomMinMax( 0.1, 0.2 ).Default( "1" ).Description( []( const std::string string, float value ) {
		return fmt::sprintf( "Bleed update period: %.2f sec", value );
	} ),
	Argument( "bleeding_immunity_period" ).IsOptional().MinMax( 0.05 ).RandomMinMax( 3, 10 ).Default( "10" ).Description( []( const std::string string, float value ) {
		return fmt::sprintf( "Bleed again after healing in: %.1f sec", value );
	} )
} )
.IsExcludedFromChaosEdition();

GameplayMod &::bulletPhysics = GameplayMod::Define( "bullet_physics", "Bullet physics mode" )
.Arguments( {
	Argument( "bullet_physics_mode" ).IsOptional().Default( "for_enemies_on_slowmotion" ).Description( []( const std::string string, float value ) {
		if ( string == "disabled" ) {
			return "Bullet physics is disabled";
		} else if ( string == "for_enemies_and_player_on_slowmotion" ) {
			return "Bullet physics will be active for both enemies and player only when slowmotion is present";
		} else if ( string == "constant" ) {
			return "Bullet physics is always present, even when slowmotion is NOT present";
		} else {
			return "Bullet physics will be active only for enemies when slowmotion is present";
		}
	} ),
} )
.IsAlsoActiveWhen( []() -> std::optional<std::string> {
	if ( ::randomGameplayMods.isActive() && ::superHot.isActive() ) {
		return "constant";
	}

	auto bulletPhysicsFromConfig = ::bulletPhysics.GetArgumentsFromCustomGameplayModeConfig();

	if ( ::bulletDelayOnSlowmotion.isActive() ) {
		if ( bulletPhysicsFromConfig.has_value() && bulletPhysicsFromConfig->at( 0 ).string == "for_enemies_and_player_on_slowmotion" ) {
			return "for_enemies_and_player_on_slowmotion";
		}

		return "constant";
	}

	if ( ::bulletRicochet.isActive() && !bulletPhysicsFromConfig.has_value() ) {
		return "constant";
	}

	return std::nullopt;
} )
.ForceDefaultArguments( "for_enemies_on_slowmotion" )
.CannotBeActivatedRandomly();

GameplayMod &::bulletDelayOnSlowmotion = GameplayMod::Define( "bullet_delay_on_slowmotion", "Bullet delay on slowmotion" )
.Description( "Your bullets almost stay still during slowmotion, and go at once when you disable it" )
.CanOnlyBeActivatedRandomlyWhen( []() {
	return !::slowmotionForbidden.isActive();
} );

GameplayMod &::bulletRicochet = GameplayMod::Define( "bullet_ricochet", "Bullet ricochet" )
.Description( "Physical bullets ricochet off the walls" )
.Arguments( {
	Argument( "bullet_ricochet_count" ).MinMax( 0 ).RandomMinMax( 2, 20 ).Default( "2" ).Description( []( const std::string string, float value ) {
		return value <= 0 ? "Max ricochets: Infinite" : fmt::sprintf( "Max ricochets: %.0f", value );
	} ),
	Argument( "bullet_ricochet_max_degree" ).MinMax( 1, 90 ).RandomMinMax( 30, 90 ).Default( "45" ).Description( []( const std::string string, float value ) {
		return fmt::sprintf( "Max angle for ricochet: %.0f deg", value );
	} ),
} );

GameplayMod &::bulletSelfHarm = GameplayMod::Define( "bullet_self_harm", "Bullet self harm" )
.Description( "Physical bullets shot by player can harm back" )
.IsAlsoActiveWhen( []() -> std::optional<std::string> {
	if ( ::randomGameplayMods.isActive() && ( ::bulletRicochet.isActive() || ::bulletDelayOnSlowmotion.isActive() ) ) {
		return "";
	}

	return std::nullopt;
} )
.CannotBeActivatedRandomly();

GameplayMod &::bulletTrail = GameplayMod::Define( "bullet_trail_constant", "Bullet trail constant" )
.Description( "Physical bullets always get a trail, regardless if slowmotion is present" )
.IsAlsoActiveWhen( []() -> std::optional<std::string> {
	if ( auto player = GetPlayer() ) {
		if ( gameplayMods::IsSlowmotionEnabled() ) {
			return "";
		}
	}

	if ( ::randomGameplayMods.isActive() && ( ::bulletRicochet.isActive() || ::superHot.isActive() ) ) {
		return "";
	}

	return std::nullopt;
} )
.CannotBeActivatedRandomly();

GameplayMod &::chaosEdition = GameplayMod::Define( "chaos_edition", "Chaos edition" )
.Description( "Rapidly changing mods" )
.OnInit([] {
	gameplayMods::previouslyProposedRandomModsCopy = gameplayMods::previouslyProposedRandomMods;

	auto randomGameplayMods = gameplayMods::randomGameplayMods.isActive<RandomGameplayModsInfo>();
	gameplayModsData.timeLeftUntilNextRandomGameplayMod = randomGameplayMods->timeUntilNextRandomGameplayMod;
})
.OnTimeExpired([] {
	if ( auto randomGameplayMods = gameplayMods::randomGameplayMods.isActive<RandomGameplayModsInfo>() ) {
		gameplayModsData.timeLeftUntilNextRandomGameplayMod = randomGameplayMods->timeUntilNextRandomGameplayMod + 1.0f;
		proposedGameplayMods.clear();
	}

	previouslyProposedRandomMods = previouslyProposedRandomModsCopy;
	previouslyProposedRandomMods.insert( &gameplayMods::chaosEdition );
});

GameplayMod &::cncSounds = GameplayMod::Define( "cnc_sounds", "Command & Conquer death sounds" )
.Description( "Tribute to one of the best death sounds" )
.CanOnlyBeActivatedRandomlyWhen( []() {
	return !::deusExSounds.isActive();
} );

GameplayMod &::crossbowExplosiveBolts = GameplayMod::Define( "crossbow_explosive_bolts", "Crossbow explosive bolts" )
.Description( "Crossbow bolts explode when they hit the wall" )
.CanOnlyBeActivatedRandomlyWhen( []() -> bool {
	if ( auto player = GetPlayer() ) {
		return player->HasNamedPlayerItem( "weapon_crossbow", true );
	}
	return false;
} );

GameplayMod &::damageMultiplier = GameplayMod::Define( "damage_multiplier", "Damage multiplier" )
.Description( "Multiplies damage dealt from enemies to you" )
.RandomGameplayModName( "Double damage" )
.RandomGameplayModDescription( "It's double painful now" )
.Arguments( {
	Argument( "multipler" ).MinMax( 0.01, 100 ).RandomMinMax( 2.0, 2.0 ).Default( "2" ).Description( []( const std::string string, float value ) {
		return fmt::sprintf( "Multiplier: %.1f", value );
	} )
} )
.CanOnlyBeActivatedRandomlyWhen( []() {
	return !::oneHitKO.isActive();
} );

GameplayMod &::damageMultiplierFromPlayer = GameplayMod::Define( "damage_multiplier_from_player", "Damage multiplier from player" )
.Description( "Multiplies damage dealt by you" )
.RandomGameplayModName( "Double damage from player" )
.RandomGameplayModDescription( "You deal double damage to enemies" )
.Arguments( {
	Argument( "multipler" ).MinMax( 0.01, 100 ).RandomMinMax( 2.0, 2.0 ).Default( "2" ).Description( []( const std::string string, float value ) {
		return fmt::sprintf( "Multiplier: %.1f", value );
	} )
} )
.CanOnlyBeActivatedRandomlyWhen( []() {
	return !::oneHitKO.isActive();
} );

GameplayMod &::deusExSounds = GameplayMod::Define( "deus_ex_sounds", "Deus EX death sounds" )
.Description( "Self explanatory" )
.CanOnlyBeActivatedRandomlyWhen( []() {
	return !::cncSounds.isActive();
} );

GameplayMod &::difficultyEasy = GameplayMod::Define( "easy", "Easy difficulty" )
.Description( "Sets up easy level of difficulty" )
.CannotBeActivatedRandomly();

GameplayMod &::difficultyHard = GameplayMod::Define( "hard", "Hard difficulty" )
.Description( "Sets up hard level of difficulty" )
.CannotBeActivatedRandomly();

GameplayMod &::divingAllowedWithoutSlowmotion = GameplayMod::Define( "diving_allowed_without_slowmotion", "Diving allowed without slowmotion" )
.Description(
	"You're still allowed to dive even if you have no slowmotion charge\n"
	"In that case you will dive without going into slowmotion"
)
.CannotBeActivatedRandomly();

GameplayMod &::divingOnly = GameplayMod::Define( "diving_only", "Diving only" )
.Description(
	"The only way to move around is by diving\n"
	"This enables Infinite slowmotion by default\n"
	"You can dive even when in crouch-like position, like when being in vents etc"
)
.CannotBeActivatedRandomly();

GameplayMod &::drunkAim = GameplayMod::Define( "drunk_aim", "Drunk aim" )
.Description( "Your aim becomes wobbly" )
.Arguments( {
	Argument( "max_horizontal_wobble" ).IsOptional().MinMax( 0, 25.5 ).RandomMinMax( 2, 25.5 ).Default( "20" ).Description( []( const std::string string, float value ) {
		return fmt::sprintf( "Max horizontal wobble: %.2f deg", value );
	} ),
	Argument( "max_vertical_wobble" ).IsOptional().MinMax( 0, 25.5 ).RandomMinMax( 2, 25.5 ).Default( "5" ).Description( []( const std::string string, float value ) {
		return fmt::sprintf( "Max vertical wobble: %.2f deg", value );
	} ),
	Argument( "wobble_frequency" ).IsOptional().MinMax( 0.01 ).RandomMinMax( 0.05, 6 ).Default( "1" ).Description( []( const std::string string, float value ) {
		return fmt::sprintf( "Wobble frequency: %.2f", value );
	} ),
} );

GameplayMod &::drunkFOV = GameplayMod::Define( "drunk_fov", "Drunk FOV" )
.Description( "Your field of view becomes wobbly" )
.Arguments( {
	Argument( "fov_offset_amplitude" ).IsOptional().MinMax( 0.01, 100 ).RandomMinMax( 0, 30 ).Default( "10" ).Description( []( const std::string string, float value ) {
		return fmt::sprintf( "FOV offset amplitude: %.1f deg", value );
	} ),
	Argument( "fov_offset_frequency" ).IsOptional().MinMax( 0.01 ).RandomMinMax( 0.1, 5 ).Default( "1" ).Description( []( const std::string string, float value ) {
		return fmt::sprintf( "FOV offset frequency: %.2f", value );
	} ),
} )
.IsAlsoActiveWhen( []() -> std::optional<std::string> {
	if ( GetMapName() == "nightmare" ) {
		return "5 1";
	}

	return std::nullopt;
} );

GameplayMod &::drunkLook = GameplayMod::Define( "drunk_look", "Drunk look" )
.Description( "Camera is wobbly and makes aim harder" )
.Arguments( {
	Argument( "drunkiness" ).IsOptional().MinMax( 0.1, 1000 ).RandomMinMax( 25, 100 ).Default( "25" ).Description( []( const std::string string, float value ) {
		return fmt::sprintf( "Drunkiness: %.0f percent", value );
	} ),
} )
.IsAlsoActiveWhen( []() -> std::optional<std::string> {
	if ( GetMapName() == "nightmare" ) {
		return "10";
	}

	return std::nullopt;
} );

GameplayMod &::explosionJumping = GameplayMod::Define( "explosion_jumping", "Explosion jumping" )
.Description( "Jupming off your own rockets and grenades is harmless" )
.CanOnlyBeActivatedRandomlyWhen( []() {
	if ( auto player = GetPlayer() ) {
		if (
			player->HasNamedPlayerItem( "weapon_handgrenade", true ) ||
			player->HasNamedPlayerItem( "weapon_satchel", true ) ||
			player->HasNamedPlayerItem( "weapon_tripmine", true ) ||
			player->HasNamedPlayerItem( "weapon_rpg", true )
		) {
			return true;
		}

		if ( auto mp5 = player->GetPlayerItem( "weapon_9mmAR" ) ) {
			auto grenadeAmmoIndex = mp5->SecondaryAmmoIndex();
			if ( player->m_rgAmmo[grenadeAmmoIndex] > 0 ) {
				return true;
			}
		}
	}

	return false;
} );

GameplayMod &::fadingOut = GameplayMod::Define( "fading_out", "Fading out" )
.Description(
	"It's getting darker, take painkillers to see better\n"
	"Allows to take painkillers even when you have 100 health and enough time have passed since the last take"
)
.Arguments( {
	Argument( "fade_out_percent" ).IsOptional().MinMax( 0, 100 ).RandomMinMax( 50, 90 ).Default( "90" ).Description( []( const std::string string, float value ) {
		return fmt::sprintf( "Fade out intensity: %.0f percent", value );
	} ),
	Argument( "fade_out_update_period" ).IsOptional().MinMax( 0.01 ).RandomMinMax( 0.2, 0.5 ).Default( "0.5" ).Description( []( const std::string string, float value ) {
		return fmt::sprintf( "Fade out update period: %.1f sec", value );
	} )
} );

GameplayMod &::frictionOverride = GameplayMod::Define( "friction", "Friction" )
.Description( "Changes player's friction" )
.RandomGameplayModName( "Low friction" )
.RandomGameplayModDescription( "Slide around" )
.Arguments( {
	Argument( "friction" ).IsOptional().MinMax( 0 ).RandomMinMax( 0, 1 ).Default( "4" ).Description( []( const std::string string, float value ) {
		return fmt::sprintf( "Friction: %.1f", value );
	} )
} );

GameplayMod &::gaussFastCharge = GameplayMod::Define( "gauss_fast_charge", "Gauss fast charge" )
.Description( "Gauss charges faster like in multiplayer" )
.CanOnlyBeActivatedRandomlyWhen( []() -> bool {
	if ( auto player = GetPlayer() ) {
		return player->HasNamedPlayerItem( "weapon_gauss", true );
	}
	return false;
} );
	
GameplayMod &::gaussJumping = GameplayMod::Define( "gauss_jumping", "Gauss jumping" )
.Description( "Allows for easier gauss jumping like in multiplayer" )
.CanOnlyBeActivatedRandomlyWhen( []() -> bool {
	if ( auto player = GetPlayer() ) {
		return player->HasNamedPlayerItem( "weapon_gauss", true );
	}
	return false;
} );

GameplayMod &::gaussNoSelfGauss = GameplayMod::Define( "no_self_gauss", "No self gauss" )
.Description( "Prevents self gauss effect" )
.IsAlsoActiveWhen( []() -> std::optional<std::string> {
	if ( ::randomGameplayMods.isActive() && ::instagib.isActive() ) {
		return "";
	}

	return std::nullopt;
} )
.CannotBeActivatedRandomly();

GameplayMod &::gibsAlways = GameplayMod::Define( "always_gib", "Always gib" )
.Description( "Kills will always try to result in gibbing" );

GameplayMod &::gibsEdible = GameplayMod::Define( "edible_gibs", "Edible gibs" )
.Description( "Eat gibs and heal yourself by pressing USE when aiming at the gib" );

GameplayMod &::gibsGarbage = GameplayMod::Define( "garbage_gibs", "Garbage gibs" )
.Description( "Replaces all gibs with garbage" );

GameplayMod &::gravity = GameplayMod::Define( "gravity", "Gravity" )
.Description( "Changes gravity value" )
.RandomGameplayModName( "Low gravity" )
.RandomGameplayModDescription( "You start to feel alittle lightheaded" )
.Arguments( {
	Argument( "gravity" ).IsOptional().MinMax( 1, 2000 ).RandomMinMax( 100, 400 ).Default( "200" ).Description( []( const std::string string, float value ) {
		return fmt::sprintf( "Forced sv_gravity: %.0f", value );
	} )
} );

GameplayMod &::grenadePellets = GameplayMod::Define( "grenade_pellets", "Grenade pellets" )
.Description( "Additional pellets will emerge after grenade explosion" )
.Arguments( {
	Argument( "pellet_amount" ).IsOptional().MinMax( 4, 64 ).RandomMinMax( 48, 64 ).Default( "48" ).Description( []( const std::string string, float value ) {
		return fmt::sprintf( "Pellet amount: %.0f", value );
	} )
} )
.CanOnlyBeActivatedRandomlyWhen( []() -> bool {
	if ( auto player = GetPlayer() ) {
		if ( auto mp5 = player->GetPlayerItem( "weapon_9mmAR" ) ) {
			auto grenadeAmmoIndex = mp5->SecondaryAmmoIndex();
			if ( player->m_rgAmmo[grenadeAmmoIndex] > 0 ) {
				return true;
			}
		}
		
		if ( player->HasNamedPlayerItem( "weapon_handgrenade", true ) ) {
			return true;
		}

		CBaseEntity *pEntity = NULL;
		while ( ( pEntity = UTIL_FindEntityInSphere( pEntity, player->pev->origin, 8192.0f ) ) != NULL ) {
			if (
				FStrEq( STRING( pEntity->pev->classname ), "monster_human_grunt" ) &&
				pEntity->pev->weapons & ( 1 << 2 ) // HGRUNT_GRENADELAUNCHER flag
			) {
				return true;
			}
		}
	}


	return false;
} );

GameplayMod &::godConstant = GameplayMod::Define( "god", "God mode" )
.Description( "You are invincible and it doesn't count as a cheat" )
.CannotBeActivatedRandomly();

GameplayMod &::gungame = GameplayMod::Define( "gungame", "Gungame" )
.Description( "Kill enemies to switch weapons" )
.Arguments( {
	Argument( "amount_of_kills" ).IsOptional().MinMax( 0 ).RandomMinMax( 1, 5 ).Default( "1" ).Description( []( const std::string string, float value ) {
		return fmt::sprintf( "Amount of kills required to get next weapon: %.0f", value );
	} ),
	Argument( "weapon_switch_time" ).IsOptional().MinMax( 0 ).Default( "0" ).Description( []( const std::string string, float value ) {
		if ( value <= 0.0f ) {
			return std::string( "Weapon won't be switched until you do enough kills" );
		} else {
			return fmt::sprintf( "Amount of kills required to get next weapon: %.0f", value );
		}
	} ),
	Argument( "sequential" ).IsOptional().Default( "" ).Description( []( const std::string string, float value ) {
		return string == "sequential" ? "Weapons will change sequentially" : "Weapons will change randomly";
	} ),
} )
.IsExcludedFromChaosEdition();

GameplayMod &::headshots = GameplayMod::Define( "headshots", "Headshots" )
.Description( "Headshots dealt to enemies become much more deadly" )
.CannotBeActivatedRandomly();

GameplayMod &::healthRegeneration = GameplayMod::Define( "health_regeneration", "Health regeneration" )
.Description( "Allows for health regeneration options" )
.Arguments( {
	Argument( "regeneration_max" ).IsOptional().MinMax( 0 ).Default( "20" ).Description( []( const std::string string, float value ) {
		return fmt::sprintf( "Regenerate up to: %.0f HP", value );
	} ),
	Argument( "regeneration_delay" ).IsOptional().MinMax( 0 ).Default( "3" ).Description( []( const std::string string, float value ) {
		return fmt::sprintf( "%.1f sec after last damage", value );
	} ),
	Argument( "regeneration_frequency" ).IsOptional().MinMax( 0.01 ).Default( "0.2" ).Description( []( const std::string string, float value ) {
		return fmt::sprintf( "Regenerating: %.1f HP/sec", 1.0f / value );
	} )
} )
.ForceDefaultArguments( "20 3 0.2" )
.CannotBeActivatedRandomly();

GameplayMod &::healOnKill = GameplayMod::Define( "heal_on_kill", "Heal on kill" )
.Description( "Your health will be replenished after killing an enemy" )
.Arguments( {
	Argument( "max_health_taken_percent" ).IsOptional().MinMax( 1, 100 ).RandomMinMax( 50, 100 ).Default( "50" ).Description( []( const std::string string, float value ) {
		return fmt::sprintf( "Victim's max health percent taken after kill: %.0f", value );
	} ),
} );

GameplayMod &::instagib = GameplayMod::Define( "instagib", "Instagib" )
.Description(
	"Gauss Gun becomes much more deadly with 9999 damage, also gets red beam and slower rate of fire\n"
	"More gibs come out"
)
.CanOnlyBeActivatedRandomlyWhen( []() {
	if ( auto player = GetPlayer() ) {
		if ( player->HasNamedPlayerItem( "weapon_gauss", true ) ) {
			return true;
		};
	}
	return false;
} );

GameplayMod &::infiniteAmmo = GameplayMod::Define( "infinite_ammo", "Infinite ammo" )
.Description( "All weapons get infinite ammo" )
.IsAlsoActiveWhen( []() -> std::optional<std::string> {
	if ( auto player = GetPlayer() ) {
		if ( ::gungame.isActive() ) {
			if ( player->m_pActiveItem && FStrEq( STRING( player->m_pActiveItem->pev->classname ), "weapon_hornetgun" ) ) {
				return std::nullopt;
			}

			return "";
		}

		static std::vector< CBasePlayer::DESPERATION_TYPE> desperations = {
			CBasePlayer::DESPERATION_TYPE::DESPERATION_FIGHTING,
			CBasePlayer::DESPERATION_TYPE::DESPERATION_REVENGE
		};

		if ( aux::ctr::includes( desperations, player->desperation ) ) {
			return "";
		}
	}

	return std::nullopt;
} );

GameplayMod &::infiniteAmmoClip = GameplayMod::Define( "infinite_ammo_clip", "Infinite ammo clip" )
.Description( "Most weapons get an infinite ammo clip and need no reloading" )
.CanOnlyBeActivatedRandomlyWhen( []() {
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

	if ( auto player = GetPlayer() ) {
		for ( auto &clipBasedWeapon : clipBasedWeapons ) {
			if ( player->HasNamedPlayerItem( clipBasedWeapon.c_str(), true ) ) {
				return true;
			}
		}
	}

	return false;
} );

GameplayMod &::initialClipAmmo = GameplayMod::Define( "initial_clip_ammo", "Initial ammo clip" )
.Description( "All weapons will have specified ammount of ammo in the clip when first picked up" )
.Arguments( {
	Argument( "initial_clip_ammo" ).IsOptional().MinMax( 1 ).Default( "4" ).Description( []( const std::string string, float value ) {
		return fmt::sprintf( "Initial ammo in the clip: %.0f", value );
	} )
} )
.CannotBeActivatedRandomly();

GameplayMod &::initialHealth = GameplayMod::Define( "starting_health", "Starting Health" )
.Description( "Start with specified health amount" )
.Arguments( {
	Argument( "health_amount" ).IsOptional().MinMax( 1 ).Default( "100" ).Description( []( const std::string string, float value ) {
		return fmt::sprintf( "Health amount: %.0f", value );
	} )
} )
.CannotBeActivatedRandomly();

GameplayMod &::inverseControls = GameplayMod::Define( "inverse_controls", "Inverse controls" )
.Description( "Movement and view controls become inversed" );

GameplayMod &::invisibility = GameplayMod::Define( "invisibility", "Invisibility" )
.Description( "Everyone is now less visible" )
.Arguments( {
	Argument( "invisibilty_amount" ).IsOptional().MinMax( 1, 100 ).RandomMinMax( 70, 90 ).Default( "80" ).Description( []( const std::string string, float value ) {
		return fmt::sprintf( "Enemies become %.0f percent less visible", value );
	} )
} );

GameplayMod &::kerotanDetector = GameplayMod::Define( "kerotan_detector", "Kerotan detector" )
.Description( "Kerotan frogs will call out to you when you get near them" )
.CannotBeActivatedRandomly();

GameplayMod &::modsDoubleTime = GameplayMod::Define( "double_time_mods", "Double time mods" )
.Description( "Next mods will last for twice as long" );

GameplayMod &::noFallDamage = GameplayMod::Define( "no_fall_damage", "No fall damage" )
.Description( "Self explanatory" )
.IsAlsoActiveWhen( []() -> std::optional<std::string> {
	if ( ::randomGameplayMods.isActive() && ::vvvvvv.isActive() ) {
		return "";
	}

	return std::nullopt;
} );

GameplayMod &::noGameTitle = GameplayMod::Define( "no_game_title", "No game title" )
.Description( "Prevents game title from appearing" )
.CannotBeActivatedRandomly();

GameplayMod &::noJumping = GameplayMod::Define( "no_jumping", "No jumping" )
.Description( "Don't allow to jump" )
.CannotBeActivatedRandomly();

GameplayMod &::noHealing = GameplayMod::Define( "no_healing", "No healing" )
.Description( "Don't allow to heal in any way, including Xen healing pools" )
.CannotBeActivatedRandomly();

GameplayMod &::noMapMusic = GameplayMod::Define( "no_map_music", "No map music" )
.Description(
	"Music which is defined by map will not be played\n"
	"Only the music defined in gameplay config files will play"
)
.IsAlsoActiveWhen( []() -> std::optional<std::string> {
#ifndef CLIENT_DLL
	if ( CHalfLifeRules *rules = dynamic_cast< CHalfLifeRules * >( g_pGameRules ) ) {
		for ( auto it = rules->configs.rbegin(); it != rules->configs.rend(); it++ ) {
			auto &cfg = *( *it );
			if ( !cfg.musicPlaylist.empty() ) {
				return "";
			}
		}
	}
#else
	if ( !clientConfig.musicPlaylist.empty() ) {
		return "";
	}
#endif

	return std::nullopt;
} )
.CannotBeActivatedRandomly();

GameplayMod &::noPainkillers = GameplayMod::Define( "no_pills", "No painkillers" )
.Description( "Don't allow to take painkillers" )
.CannotBeActivatedRandomly();

GameplayMod &::noSaving = GameplayMod::Define( "no_saving", "No saving" )
.Description( "Don't allow to load saved files" )
.IsAlsoActiveWhen( []() -> std::optional<std::string> {
	if ( gameplayMods::autoSavesOnly.isActive() && !g_autoSaved ) {
		return "";
	}

	return std::nullopt;
} )
.CannotBeActivatedRandomly();

GameplayMod &::noSecondaryAttack = GameplayMod::Define( "no_secondary_attack", "No secondary attack" )
.Description( "Disables the secondary attack on all weapons" )
.CanOnlyBeActivatedRandomlyWhen( []() {
	static std::vector<const char *> weaponsWithSecondaryAttack = { 
		"weapon_shotgun",
		"weapon_9mmAR",
		"weapon_crossbow",
		"weapon_gauss"
	};
	if ( auto player = GetPlayer() ) {
		for ( auto weapon : weaponsWithSecondaryAttack ) {
			if ( player->HasNamedPlayerItem( weapon, true ) ) {
				return true;
			}
		}
	}
	return false;
} );

GameplayMod &::noSmgGrenadePickup = GameplayMod::Define( "no_smg_grenade_pickup", "No SMG grenade pickup" )
.Description( "You're not allowed to pickup and use SMG (MP5) grenades" )
.CannotBeActivatedRandomly();

GameplayMod &::noStartDark = GameplayMod::Define( "no_start_dark", "No start dark" )
.Description( "Prevents game fading out from dark at the start" )
.CannotBeActivatedRandomly();

GameplayMod &::noTargetConstant = GameplayMod::Define( "no_target", "No target" )
.Description( "You are invisible to everyone and it doesn't count as a cheat" )
.CannotBeActivatedRandomly();

GameplayMod &::noWalking = GameplayMod::Define( "no_walking", "No walking" )
.Description( "Don't allow to walk, swim, dive, climb ladders" )
.CannotBeActivatedRandomly();

GameplayMod &::oneHitKO = GameplayMod::Define( "one_hit_ko", "One hit KO" )
.Description(
	"Any hit from an enemy will kill you instantly\n"
	"You still get proper damage from falling and environment"
)
.IsAlsoActiveWhen( []() -> std::optional<std::string> {
	if ( ::randomGameplayMods.isActive() && ::superHot.isActive() ) {
		return "";
	}

	return std::nullopt;
} )
.CanOnlyBeActivatedRandomlyWhen( []() {
	return !::snarkNuclear.isActive() && !::teleportOnKill.isActive() && !::damageMultiplier.isActive();
} )
.IsExcludedFromChaosEdition();

GameplayMod &::oneHitKOFromPlayer = GameplayMod::Define( "one_hit_ko_from_player", "One hit KO from player" )
.Description( "All enemies die in one hit" )
.IsAlsoActiveWhen( []() -> std::optional<std::string> {
	if ( ::randomGameplayMods.isActive() && ::superHot.isActive() ) {
		return "";
	}

	return std::nullopt;
} )
.CannotBeActivatedRandomly();

GameplayMod &::painkillersInfinite = GameplayMod::Define( "infinite_painkillers", "Infinite painkillers" )
.Description( "Self explanatory" )
.CannotBeActivatedRandomly();

GameplayMod &::painkillersSlow = GameplayMod::Define( "slow_painkillers", "Slow painkillers" )
.Description( "Painkillers take time to have an effect, like in original Max Payne" )
.Arguments( {
	Argument( "healing_period" ).IsOptional().MinMax( 0.01 ).RandomMinMax( 0.1, 0.3 ).Default( "0.2" ).Description( []( const std::string string, float value ) {
		return fmt::sprintf( "Healing period %.1f sec", value );
	} )
} );

GameplayMod &::payned = GameplayMod::Define( "payned", "Payned" )
.Description( "Payne to the max!" );

GameplayMod &::paynedSounds = GameplayMod::Define( "payned_sounds", "Payned sounds" )
.Description( "Everyone make sounds of thugs from Max Payne" )
.CannotBeActivatedRandomly();

GameplayMod &::paynedSoundsHumans = GameplayMod::Define( "payned_sounds_humans", "Payned sounds - humans" )
.Description( "Humans make sounds of thugs from Max Payne" )
.IsAlsoActiveWhen( []() -> std::optional<std::string> {
	if ( ::paynedSounds.isActive() ) {
		return "";
	}

	if ( ::randomGameplayMods.isActive() && ::payned.isActive() ) {
		return "";
	}

	return std::nullopt;
} )
.CannotBeActivatedRandomly();

GameplayMod &::paynedSoundsMonsters = GameplayMod::Define( "payned_sounds_monsters", "Payned sounds - monsters" )
.Description( "Monsters make sounds of thugs from Max Payne" )
.IsAlsoActiveWhen( []() -> std::optional<std::string> {
	if ( ::paynedSounds.isActive() ) {
		return "";
	}

	if ( ::randomGameplayMods.isActive() && ::payned.isActive() ) {
		return "";
	}

	return std::nullopt;
} )
.CannotBeActivatedRandomly();

GameplayMod &::preventMonsterMovement = GameplayMod::Define( "prevent_monster_movement", "Prevent monster movement" )
.Description( "Monsters will always stay at spawn spot" )
.CannotBeActivatedRandomly();

GameplayMod &::preventMonsterSpawn = GameplayMod::Define( "prevent_monster_spawn", "Prevent monster spawn" )
.Description(
	"Don't spawn predefined monsters (NPCs) when visiting a new map\n"
	"This doesn't affect dynamic monster_spawners"
)
.CannotBeActivatedRandomly();

GameplayMod &::preventMonsterDrops = GameplayMod::Define( "prevent_monster_drops", "Prevent monster drops" )
.Description( "Monsters won't drop anything when dying" )
.CannotBeActivatedRandomly();

GameplayMod &::preserveNightmare = GameplayMod::Define( "preserve_nightmare", "Preserve nightmare" )
.Description( "Nightmare additional chapter will not be skipped" )
.CannotBeActivatedRandomly();

GameplayMod &::quakeRockets = GameplayMod::Define( "quake_rockets", "Quake rockets" )
.Description( "RPG is much more fast and rockets are shot instantly" )
.Arguments( {
	Argument( "fire_delay" ).IsOptional().MinMax( 0.1 ).Default( "0.8" ).Description( []( const std::string string, float value ) {
		return fmt::sprintf( "Fire delay: %.1f sec", value );
	} ),
	Argument( "speed" ).IsOptional().MinMax( 10, 2000 ).Default( "1500" ).Description( []( const std::string string, float value ) {
		return fmt::sprintf( "Rocket speed: %.0f units", value );
	} ),
} )
.CannotBeActivatedRandomly();

GameplayMod &::randomGameplayMods = GameplayMod::Define( "random_gameplay_mods", "Random gameplay mods" )
.Description( "Random gameplay mods" )
.Arguments( {
	Argument( "time_for_random_gameplay_mod" ).IsOptional().MinMax( 1 ).Default( "60" ).Description( []( const std::string string, float value ) {
		return fmt::sprintf( "Random mod will last for %.0f sec", value );
	} ),
	Argument( "time_until_next_random_gameplay_mod" ).IsOptional().MinMax( 2 ).Default( "180" ).Description( []( const std::string string, float value ) {
		return fmt::sprintf( "Random mods will be applied every %.0f sec", value );
	} ),
	Argument( "time_for_random_gameplay_mod_voting" ).IsOptional().MinMax( 0 ).Default( "30" ).Description( []( const std::string string, float value ) {
		return fmt::sprintf( "Upcoming random mods will be shown for %.0f sec", value );
	} )
} )
.IsAlsoActiveWhen( []() -> std::optional<std::string> {
	if ( ::chaosEdition.isActive() ) {
		return "4 2 2";
	}

	return std::nullopt;
} )
.CannotBeActivatedRandomly();

GameplayMod &::randomSpawnerSeed = GameplayMod::Define( "random_spawner_seed", "Random spawner seed" )
.Description( "Makes random spawns same for all players based on a seed" )
.Arguments( {
	Argument( "seed" ).Description( []( const std::string string, float value ) {
		return "Seed: " + string + " \n";
	} )
} )
.CannotBeActivatedRandomly();

GameplayMod &::scoreAttack = GameplayMod::Define( "score_attack", "Score attack" )
.Description( "Kill enemies to get as much score as possible. Build combos to get even more score" )
.CannotBeActivatedRandomly();

GameplayMod &::shotgunAutomatic = GameplayMod::Define( "shotgun_automatic", "Automatic shotgun" )
.Description( "Shotgun only fires single shots and doesn't have to be reloaded after each shot" )
.CanOnlyBeActivatedRandomlyWhen( []() -> bool {
	if ( auto player = GetPlayer() ) {
		return player->HasNamedPlayerItem( "weapon_shotgun", true );
	}
	return false;
} );

GameplayMod &::shootUnderwater = GameplayMod::Define( "shoot_underwater", "Shoot underwater" )
.Description( "All weapons now can shoot underwater" )
.CannotBeActivatedRandomly();

GameplayMod &::slowmotionConstant = GameplayMod::Define( "constant_slowmotion", "Constant slowmotion" )
.Description( "You start in slowmotion, it's infinite and you can't turn it off" )
.IsAlsoActiveWhen( []() -> std::optional<std::string> {
	if ( GetMapName() == "nightmare" ) {
		return "";
	}

	if ( auto player = GetPlayer() ) {
		static std::vector< CBasePlayer::DESPERATION_TYPE> desperations = {
			CBasePlayer::DESPERATION_TYPE::DESPERATION_PRE_IMMINENT,
			CBasePlayer::DESPERATION_TYPE::DESPERATION_IMMINENT,
			CBasePlayer::DESPERATION_TYPE::DESPERATION_ALL_FOR_REVENGE,
			CBasePlayer::DESPERATION_TYPE::DESPERATION_REVENGE
		};

		if ( aux::ctr::includes( desperations, player->desperation ) ) {
			return "";
		}
	}

	return std::nullopt;
} )
.CannotBeActivatedRandomly();

GameplayMod &::slowmotionFastWalk = GameplayMod::Define( "slowmotion_fast_walk", "Fast walk in slowmotion" )
.Description( "You still walk and run almost as fast as when slowmotion is not active" )
.CanOnlyBeActivatedRandomlyWhen( []() {
	return !::slowmotionForbidden.isActive();
} );

GameplayMod &::slowmotionForbidden = GameplayMod::Define( "no_slowmotion", "No slowmotion" )
.Description( "You're not allowed to use slowmotion" )
.CanOnlyBeActivatedRandomlyWhen( []() {
	return
		!::slowmotionInfinite.isActive() && !::superHotToggleable.isActive() && !!::bulletDelayOnSlowmotion.isActive() &&
		!::slowmotionFastWalk.isActive() && !::slowmotionOnlyDiving.isActive() && !::slowmotionOnDamage.isActive();
} );

GameplayMod &::slowmotionInfinite = GameplayMod::Define( "infinite_slowmotion", "Infinite slowmotion" )
.Description( "You have infinite slowmotion charge and it's not considered as cheat" )
.IsAlsoActiveWhen( []() -> std::optional<std::string> {
	if ( ::slowmotionConstant.isActive() ) {
		return "";
	}

	if ( ::divingOnly.isActive() ) {
		return "";
	}

	if ( gameplayModsData.slowmotionInfiniteCheat ) {
		return "";
	}

	if ( auto player = GetPlayer() ) {
		if ( player->desperation == CBasePlayer::DESPERATION_TYPE::DESPERATION_FIGHTING && !::slowmotionForbidden.isActive() ) {
			return "";
		}
	}

	return std::nullopt;
} );

GameplayMod &::slowmotionInitialCharge = GameplayMod::Define( "initial_slowmotion_charge", "Initial slowmotion charge" )
.Description( "Start with specified slowmotion charge" )
.Arguments( {
	Argument( "slowmotion_charge" ).IsOptional().MinMax( 0, 100 ).Default( "0" ).Description( []( const std::string string, float value ) {
		return fmt::sprintf( "%.0f percent of slowmotion charge", value );
	} )
} )
.CannotBeActivatedRandomly();

GameplayMod &::slowmotionOnDamage = GameplayMod::Define( "slowmotion_on_damage", "Slowmotion on damage" )
.Description( "You get slowmotion charge when receiving damage" )
.CanOnlyBeActivatedRandomlyWhen( []() {
	return !::slowmotionForbidden.isActive();
} );

GameplayMod &::slowmotionOnlyDiving = GameplayMod::Define( "slowmotion_only_diving", "Slowmotion only when diving" )
.Description( "You're allowed to go into slowmotion only by diving" )
.CanOnlyBeActivatedRandomlyWhen( []() {
	return !::slowmotionForbidden.isActive();
} );

GameplayMod &::snarkFriendlyToAllies = GameplayMod::Define( "snark_friendly_to_allies", "Snarks friendly to allies" )
.Description( "Snarks won't attack player's allies" )
.CannotBeActivatedRandomly();

GameplayMod &::snarkFriendlyToPlayer = GameplayMod::Define( "snark_friendly_to_player", "Snarks friendly to player" )
.Description( "Snarks won't attack player" )
.CanOnlyBeActivatedRandomlyWhen( []() {
	if ( auto player = GetPlayer() ) {
		if ( player->HasNamedPlayerItem( "weapon_snark", true ) ) {
			return true;
		}
	}
	return ::snarkInfestation.isActive() || ::snarkFromExplosion.isActive();
} );

GameplayMod &::snarkFromExplosion = GameplayMod::Define( "snark_from_explosion", "Snark from explosion" )
.Description( "Snarks will spawn in the place of explosion" );

GameplayMod &::snarkInception = GameplayMod::Define( "snark_inception", "Snark inception" )
.Description( "Killing a snark splits it into two snarks" )
.Arguments( {
	Argument( "inception_depth" ).IsOptional().MinMax( 1, 100 ).RandomMinMax( 1, 1 ).Default( "10" ).Description( []( const std::string string, float value ) {
		return fmt::sprintf( "Inception depth: %.0f snarks", value );
	} )
} )
.CanOnlyBeActivatedRandomlyWhen( []() {
	if ( auto player = GetPlayer() ) {
		if ( player->HasNamedPlayerItem( "weapon_snark", true ) ) {
			return true;
		}
	}
	return
		( ::snarkInfestation.isActive() || ::snarkFromExplosion.isActive() ) &&
		( !::teleportOnKill.isActive() );
} );

GameplayMod &::snarkInfestation = GameplayMod::Define( "snark_infestation", "Snark infestation" )
.Description(
	"Snark will spawn in the body of killed NPC\n"
	"Even more snarks spawn if NPC's corpse has been gibbed"
)
.CanOnlyBeActivatedRandomlyWhen( []() {
	return !::teleportOnKill.isActive();
} );

GameplayMod &::snarkNuclear = GameplayMod::Define( "snark_nuclear", "Snark nuclear" )
.Description( "Killing a snark produces a grenade-like explosion" )
.CanOnlyBeActivatedRandomlyWhen( []() {
	if ( auto player = GetPlayer() ) {
		if ( player->HasNamedPlayerItem( "weapon_snark", true ) ) {
			return true;
		}
	}
	return
		( ::snarkInfestation.isActive() || ::snarkFromExplosion.isActive() ) &&
		( !::oneHitKO.isActive() && !::teleportOnKill.isActive() );
} );

GameplayMod &::snarkPenguins = GameplayMod::Define( "snark_penguins", "Snark penguins" )
.Description( "Replaces snarks with penguins from Opposing Force\n" )
.CanOnlyBeActivatedRandomlyWhen( []() {
	if ( auto player = GetPlayer() ) {
		if ( player->HasNamedPlayerItem( "weapon_snark" ) ) {
			return true;
		}
	}
	return ::snarkInfestation.isActive() || ::snarkFromExplosion.isActive();
} );

GameplayMod &::snarkStayAlive = GameplayMod::Define( "snark_stay_alive", "Snark stay alive" )
.Description( "Snarks will never die on their own, they must be shot" )
.CanOnlyBeActivatedRandomlyWhen( []() {
	if ( auto player = GetPlayer() ) {
		if ( player->HasNamedPlayerItem( "weapon_snark", true ) ) {
			return true;
		}
	}
	return ::snarkInfestation.isActive() || ::snarkFromExplosion.isActive();
} );

GameplayMod &::superHot = GameplayMod::Define( "superhot", "SUPERHOT" )
.Description(
	"Time moves forward only when you move around, everyone dies in one hit\n"
	"Inspired by the game SUPERHOT"
)
.IsAlsoActiveWhen( []() -> std::optional<std::string> {
	if ( auto player = GetPlayer() ) {
		if ( gameplayMods::superHotToggleable.isActive() && player->slowMotionWasEnabled ) {
			return "";
		}
	}

	return std::nullopt;
} )
.CanOnlyBeActivatedRandomlyWhen( []() {
	if ( GetMapName() == "c2a3e" ) {
		return false;
	}

	return !::timescale.isActive( true ) && !::timescaleOnDamage.isActive() && !::superHotToggleable.isActive() && !::oneHitKO.isActive() && !::oneHitKOFromPlayer.isActive();
} );

GameplayMod &::superHotToggleable = GameplayMod::Define( "superhot_toggleable", "SUPERHOT Toggleable" )
.Description(
	"After you activate slowmotion, time moves forward only when you move around and everyone dies in one hit\n"
	"Inspired by the game SUPERHOT"
)
.CanOnlyBeActivatedRandomlyWhen( []() {
	return !::timescale.isActive( true ) && !::timescaleOnDamage.isActive() && !::superHot.isActive() && !::slowmotionForbidden.isActive();
} );

GameplayMod &::swearOnKill = GameplayMod::Define( "swear_on_kill", "Swear on kill" )
.Description( "Max will swear after killing an enemy" );

GameplayMod &::teleportMaintainVelocity = GameplayMod::Define( "teleport_maintain_velocity", "Teleport maintain velocity" )
.Description( "Your velocity will be preserved after going through teleporters" )
.CannotBeActivatedRandomly();

GameplayMod &::teleportOnKill = GameplayMod::Define( "teleport_on_kill", "Teleport on kill" )
.Description( "You will be teleported to the enemy you kill" )
.Arguments( {
	Argument( "teleport_weapon" ).IsOptional().Default( "any" ).Description( []( const std::string string, float value ) {
		return "Weapon that causes teleport: " + ( string.empty() ? "any" : string ) + "\n";
	} )
} )
.CanOnlyBeActivatedRandomlyWhen( []() {
	return
		!::snarkNuclear.isActive() && !::snarkInfestation.isActive() &&
		!::snarkInception.isActive() && !::oneHitKO.isActive();
} );

GameplayMod &::timeRestriction = GameplayMod::Define( "time_restriction", "Time restriction" )
.Description( "You are killed if time runs out" )
.Arguments( {
	Argument( "time" ).IsOptional().MinMax( 1 ).Default( "60" ).Description( []( const std::string string, float value ) {
		return fmt::sprintf( "%.0f seconds", value );
	} ),
} )
.IsAlsoActiveWhen( []() -> std::optional<std::string> {
	
	if ( ::blackMesaMinute.isActive() ) {
		if ( auto timeRestrictionFromConfig = ::timeRestriction.GetArgumentsFromCustomGameplayModeConfig() ) {
			return timeRestrictionFromConfig->at( 0 ).string;
		} else {
			return "60";
		}
	}

	return std::nullopt;
} )
.CannotBeActivatedRandomly();

GameplayMod &::timerShown = GameplayMod::Define( "show_timer", "Show timer" )
.Description( "Timer will be shown. Time is affected by slowmotion" )
.IsAlsoActiveWhen( []() -> std::optional<std::string> {
	if ( ::timeRestriction.isActive() ) {
		return "";
	}

	if ( ::timerShownReal.isActive() ) {
		return "";
	}

	return std::nullopt;
} )
.CannotBeActivatedRandomly();

GameplayMod &::timerShownReal = GameplayMod::Define( "show_timer_real_time", "Show timer with real time" )
.Description( "Time will be shown and it's not affected by slowmotion, which is useful for speedruns" )
.CannotBeActivatedRandomly();

GameplayMod &::timescale = GameplayMod::Define( "timescale", "Timescale" )
.Description( "Changes default timescale" )
.RandomGameplayModName( "Haste" )
.RandomGameplayModDescription( "Increased timescale" )
.Arguments( {
	Argument( "timescale" ).MinMax( 0.1, 10 ).RandomMinMax( 1.3, 2.0 ).Default( "1" ).Description( []( const std::string string, float value ) {
		return fmt::sprintf( "Timescale: %.1f", value );
	} ),
} )
.CanOnlyBeActivatedRandomlyWhen( []() {
	return !::superHot.isActive() && !::superHotToggleable.isActive() && !::timescaleOnDamage.isActive();
} )
.ForceDefaultArguments( "1" );

GameplayMod &::timescaleOnDamage = GameplayMod::Define( "timescale_on_damage", "Timescale on damage" )
.Description( "You kill - time goes faster, you're hit - time goes slower" )
.CanOnlyBeActivatedRandomlyWhen( []() {
	return !::superHot.isActive() && !::timescale.isActive( true );
} );

GameplayMod &::tripminesDetachable = GameplayMod::Define( "detachable_tripmines", "Detachable tripmines" )
.Description( "Pressing USE button on attached tripmines will detach them" )
.Arguments( {
	Argument( "detach_tripmines_instantly" ).IsOptional().Default( "" ).Description( []( const std::string string, float value ) {
		return string == "instantly" ? "Tripmines will be INSTANTLY added to your inventory if possible" : "";
	} )
} )
.CannotBeActivatedRandomly();

GameplayMod &::upsideDown = GameplayMod::Define( "upside_down", "Upside down" )
.Description( "You see the world upside down" )
.IsAlsoActiveWhen( []() -> std::optional<std::string> {
	if ( gameplayModsData.reverseGravity && ::vvvvvv.isActive() ) {
		return "";
	}

	return std::nullopt;
} );

GameplayMod &::vvvvvv = GameplayMod::Define( "vvvvvv", "VVVVVV" )
.Description(
	"Pressing jump reverses gravity for player\n"
	"Inspired by the game VVVVVV"
)
.CanOnlyBeActivatedRandomlyWhen( []() {
	std::vector<std::string> allowedMaps = {
		"c1a0", "c1a0d", "c1a0a", "c1a0b", "c1a0e",
		"c1a0c", "c1a1", "c1a1a", "c1a1b", "c1a1c", "c1a1d", "c1a1f",
		"c1a2", "c1a2a", "c1a2b", "c1a2c", "c1a2d",
		"c1a3", "c1a3a", "c1a3d",
		"c1a4", "c1a4b", "c1a4d", "c1a4e", "c1a4f", "c1a4g", "c1a4i", "c1a4j", "c1a4k",
		"c2a1", "c2a1a", "c2a1b",
		"c2a2", "c2a2a", "c2a2b1", "c2a2b2", "c2a2c", "c2a2d", "c2a2e", "c2a2f",
		"c2a3", "c2a3a", "c2a3b", "c2a3c",
		"c2a4a", "c2a4b", "c2a4c",
		"c2a4d", "c2a4e", "c2a4f",
		"c3a2e", "c3a2", "c3a2a", "c3a2b", "c3a2c", "c3a2d", "c3a2f"
	};

	if ( aux::ctr::includes( allowedMaps, GetMapName() ) ) {
		return true;
	}
	
	return false;
} )
.CanBeCancelledAfterChangeLevel();

GameplayMod &::weaponImpact = GameplayMod::Define( "weapon_impact", "Weapon impact" )
.Description(
	"Taking damage means to be pushed back"
)
.Arguments( {
	Argument( "impact" ).IsOptional().MinMax( 1 ).RandomMinMax( 3, 8 ).Default( "3" ).Description( []( const std::string string, float value ) {
		return fmt::sprintf( "Impact multiplier: %.1f", value );
	} ),
} );

GameplayMod &::weaponPushBack = GameplayMod::Define( "weapon_push_back", "Weapon push back" )
.Description(
	"Shooting weapons pushes you back"
)
.Arguments( {
	Argument( "weapon_push_back_multiplier" ).IsOptional().MinMax( 0.1 ).RandomMinMax( 0.8, 1.2 ).Default( "1" ).Description( []( const std::string string, float value ) {
		return fmt::sprintf( "Push back multiplier: %.1f", value );
	} ),
} );

GameplayMod &::weaponRestricted = GameplayMod::Define( "weapon_restricted", "Weapon restricted" )
.Description(
	"If you have no weapons - you are allowed to pick only one\n"
	"You can have several weapons at once if they are specified in [loadout] section\n"
	"Weapon stripping doesn't affect you"
)
.CannotBeActivatedRandomly();

#ifndef CLIENT_DLL
extern int gEvilImpulse101;
#endif
GameplayMod &::eventGiveRandomWeapon = GameplayMod::Define( "event_give_random_weapon", "Give random weapon" )
.OnEventInit( []() -> std::pair<std::string, std::string> {
#ifndef CLIENT_DLL
	static std::vector<std::pair<const char *, const char *>> allowedRandomWeapons = {
		{ "weapon_chicken", "Chicken" },
		{ "weapon_9mmhandgun", "Beretta" },
		{ "weapon_shotgun", "Shotgun" },
		{ "weapon_9mmAR", "SMG" },
		{ "weapon_m249", "M249" },
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

	if ( auto player = GetPlayer() ) {

		float hud_autoswitch = CVAR_GET_FLOAT( "hud_autoswitch" );
		auto &weapon = aux::rand::choice( allowedRandomWeapons );
		CVAR_SET_FLOAT( "hud_autoswitch", 0.0f );
		
		gEvilImpulse101 = TRUE;
		player->GiveNamedItem( weapon.first, true );
		if ( auto playerWeapon = player->GetPlayerItem( weapon.first ) ) {
			player->m_rgAmmo[playerWeapon->PrimaryAmmoIndex()] = playerWeapon->iMaxAmmo1();
		}
		gEvilImpulse101 = FALSE;

		CVAR_SET_FLOAT( "hud_autoswitch", hud_autoswitch );
		
		return { "Received weapon", weapon.second };
	}

#endif
	return { "", "" };
} )
.IsExcludedFromChaosEdition();

GameplayMod &::eventGivePainkillers = GameplayMod::Define( "event_give_painkillers", "Give painkillers" )
.OnEventInit( []() -> std::pair<std::string, std::string> {
	if ( auto player = GetPlayer() ) {
		player->painkillerCount = MAX_PAINKILLERS;
	}

	return { "Give painkillers", "Painkillers replenished" };
} )
.CanOnlyBeActivatedRandomlyWhen( []() {
	if ( auto player = GetPlayer() ) {
		return player->painkillerCount <= 3;
	}

	return false;
} )
.IsExcludedFromChaosEdition();

GameplayMod &::eventModPack = GameplayMod::Define( "event_modpack", "Modpack" )
.OnEventInit( []() -> std::pair<std::string, std::string> {
#ifndef CLIENT_DLL
	auto randomGameplayMods = gameplayMods::randomGameplayMods.isActive<RandomGameplayModsInfo>();

	if ( !randomGameplayMods ) {
		return { "", "" };
	}

	int modAmount = aux::rand::uniformInt( 2, 5 );

	for ( int i = 0; i < modAmount; i++ ) {
		auto filteredMods = gameplayMods::GetFilteredRandomGameplayMods();
		aux::ctr::filter( &filteredMods, []( GameplayMod *mod ) {
			if ( mod->isEvent ) {
				return false;
			}

			return true;
		} );

		if ( filteredMods.empty() ) {
			previouslyProposedRandomMods.clear();
			break;
		}

		auto randomMod = aux::rand::choice( filteredMods );
		filteredMods.erase( randomMod );
		
		timedGameplayMods.push_back( {
			randomMod,
			randomMod->GetRandomArguments(),
			randomGameplayMods->timeForRandomGameplayMod,
			randomGameplayMods->timeForRandomGameplayMod
		} );

		previouslyProposedRandomMods.insert( randomMod );
	}

	return { "Modpack", "Activating several mods at once!" };
#else 
	return { "", "" };
#endif

} )
.IsExcludedFromChaosEdition();

GameplayMod &::eventSpawnRandomMonsters = GameplayMod::Define( "event_spawn_random_monsters", "Spawn random enemies" )
.OnEventInit( []() -> std::pair<std::string, std::string> {
	gameplayModsData.monsterSpawnAttempts = aux::rand::uniformInt( 2, 4 );

	return { "Spawn random monsters", "Some of the next maps will have more enemies" };
} )
.IsExcludedFromChaosEdition();
