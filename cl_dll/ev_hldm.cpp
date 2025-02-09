/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "entity_types.h"
#include "usercmd.h"
#include "pm_defs.h"
#include "pm_materials.h"

#include "eventscripts.h"
#include "ev_hldm.h"

#include "r_efx.h"
#include "event_api.h"
#include "event_args.h"
#include "in_defs.h"

#include <string.h>
#define M_PI 3.14159265358979323846

#include "r_studioint.h"
#include "com_model.h"

extern engine_studio_api_t IEngineStudio;

static int tracerCount[ 32 ];

extern float spreadMultiplier;
extern int upsideDown;
extern int paynedSoundsMonsters;

extern "C"
{
#include "pm_shared.h"
}

void V_PunchAxis( int axis, float punch );
void V_PunchAxisAdditive( int axis, float punch );
void VectorAngles( const float *forward, float *angles );

extern cvar_t *cl_lw;

extern "C"
{

// HLDM
void EV_FireGlock1( struct event_args_s *args  );
void EV_FireGlock2( struct event_args_s *args  );
void EV_FireGlockTwin( struct event_args_s *args );
void EV_FireShotGunSingle( struct event_args_s *args  );
void EV_FireShotGunDouble( struct event_args_s *args  );
void EV_FireIngram( struct event_args_s *args );
void EV_FireIngramTwin( struct event_args_s *args );
void EV_FireIngramTwinTracer( struct event_args_s *args );
void EV_FireMP5( struct event_args_s *args  );
void EV_FireMP52( struct event_args_s *args  );
void EV_FireM249( struct event_args_s *args  );
void EV_FirePython( struct event_args_s *args  );
void EV_FireGauss( struct event_args_s *args  );
void EV_SpinGauss( struct event_args_s *args  );
void EV_Crowbar( struct event_args_s *args  );
void EV_Chicken( struct event_args_s *args );
void EV_FireCrossbow( struct event_args_s *args  );
void EV_FireCrossbow2( struct event_args_s *args  );
void EV_FireRpg( struct event_args_s *args  );
void EV_EgonFire( struct event_args_s *args  );
void EV_EgonStop( struct event_args_s *args  );
void EV_HornetGunFire( struct event_args_s *args  );
void EV_TripmineFire( struct event_args_s *args  );
void EV_SnarkFire( struct event_args_s *args  );


void EV_TrainPitchAdjust( struct event_args_s *args );
}

#define VECTOR_CONE_1DEGREES Vector( 0.00873, 0.00873, 0.00873 )
#define VECTOR_CONE_2DEGREES Vector( 0.01745, 0.01745, 0.01745 )
#define VECTOR_CONE_3DEGREES Vector( 0.02618, 0.02618, 0.02618 )
#define VECTOR_CONE_4DEGREES Vector( 0.03490, 0.03490, 0.03490 )
#define VECTOR_CONE_5DEGREES Vector( 0.04362, 0.04362, 0.04362 )
#define VECTOR_CONE_6DEGREES Vector( 0.05234, 0.05234, 0.05234 )
#define VECTOR_CONE_7DEGREES Vector( 0.06105, 0.06105, 0.06105 )	
#define VECTOR_CONE_8DEGREES Vector( 0.06976, 0.06976, 0.06976 )
#define VECTOR_CONE_9DEGREES Vector( 0.07846, 0.07846, 0.07846 )
#define VECTOR_CONE_10DEGREES Vector( 0.08716, 0.08716, 0.08716 )
#define VECTOR_CONE_15DEGREES Vector( 0.13053, 0.13053, 0.13053 )
#define VECTOR_CONE_20DEGREES Vector( 0.17365, 0.17365, 0.17365 )

// DUMBEST HACK to prevent double executing of the event
// The root cause is client prediction - cl_lw 0 would fix the issue,
// but I'd prefer not to change the default value
// 0.011 - is approximation
// also see: https://github.com/ValveSoftware/halflife/issues/1621
float latestShoot = 0.0f;
float latestTracer = 0.0f;
float latestTracer2 = 0.0f;
float latestTracer3 = 0.0f;

bool AllowedToFireRapidEvent( float &param ) {
	bool allowed = true;

	float currentAbsoluteTime = gEngfuncs.GetClientTime();
	if ( fabs( currentAbsoluteTime - param ) < 0.011f ) {
		allowed = false;
	}
	param = currentAbsoluteTime;

	return allowed;
}

// Offset the source of trace to the right\left alittle, and then rotate the aim to the left\right according to the distance
// ( function should be moved to UTIL? )
void VectorSkew( vec3_t vecSrc, vec3_t angles, vec3_t forward, float *vecAiming, float rightOffset ) {
	
	pmtrace_t tr;
	vec3_t vecEnd;
	float traceDistance = 8192;

	VectorMA( vecSrc, traceDistance, forward, vecEnd );

	gEngfuncs.pEventAPI->EV_PlayerTrace( vecSrc, vecEnd, PM_STUDIO_BOX, -1, &tr );
	float hitDistance = ( tr.endpos - vecSrc ).Length();
	double yawRotation = ( atan( rightOffset / hitDistance ) * 180 ) / M_PI;
	if ( upsideDown ) {
		yawRotation *= -1.0f;
	}

	AngleVectors( angles + Vector( 0.0, yawRotation, 0.0 ), forward, Vector(), Vector() );
	VectorCopy( forward, vecAiming );
}

// play a strike sound based on the texture that was hit by the attack traceline.  VecSrc/VecEnd are the
// original traceline endpoints used by the attacker, iBulletType is the type of bullet that hit the texture.
// returns volume of strike instrument (crowbar) to play
float EV_HLDM_PlayTextureSound( int idx, pmtrace_t *ptr, float *vecSrc, float *vecEnd, int iBulletType )
{
	// hit the world, try to play sound based on texture material type
	char chTextureType = CHAR_TEX_CONCRETE;
	cl_entity_t* cl_entity = NULL;
	float fvol;
	float fvolbar;
	char *rgsz[4];
	int cnt;
	float fattn = ATTN_NORM;
	int entity;
	char *pTextureName;
	char texname[ 64 ];
	char szbuffer[ 64 ];

	entity = gEngfuncs.pEventAPI->EV_IndexFromTrace( ptr );

	// FIXME check if playtexture sounds movevar is set
	//

	chTextureType = 0;

	// Player
	if ( entity == 0 )
	{
		// get texture from entity or world (world is ent(0))
		pTextureName = (char *)gEngfuncs.pEventAPI->EV_TraceTexture( ptr->ent, vecSrc, vecEnd );
		
		if ( pTextureName )
		{
			strcpy( texname, pTextureName );
			pTextureName = texname;

			// strip leading '-0' or '+0~' or '{' or '!'
			if (*pTextureName == '-' || *pTextureName == '+')
			{
				pTextureName += 2;
			}

			if (*pTextureName == '{' || *pTextureName == '!' || *pTextureName == '~' || *pTextureName == ' ')
			{
				pTextureName++;
			}
			
			// '}}'
			strcpy( szbuffer, pTextureName );
			szbuffer[ CBTEXTURENAMEMAX - 1 ] = 0;
				
			// get texture type
			chTextureType = PM_FindTextureType( szbuffer );	
		}
	}
	else
	{
		// JoshA: Look up the entity and find the EFLAG_FLESH_SOUND flag.
		// This broke at some point then TF:C added prediction.
		//
		// It used to use Classify of pEntity->Classify() != CLASS_NONE && pEntity->Classify() != CLASS_MACHINE
		// to determine what sound to play, but that's server side and isn't available on the client
		// and got lost in the translation to that.
		// Now the server will replicate that state via an eflag.
		cl_entity = gEngfuncs.GetEntityByIndex(entity);

		if (cl_entity && !!(cl_entity->curstate.eflags & EFLAG_FLESH_SOUND))
			chTextureType = CHAR_TEX_FLESH;
	}	
	
	switch (chTextureType)
	{
	default:
	case CHAR_TEX_CONCRETE: fvol = 0.9;	fvolbar = 0.6;
		rgsz[0] = "player/pl_step1.wav";
		rgsz[1] = "player/pl_step2.wav";
		cnt = 2;
		break;
	case CHAR_TEX_METAL: fvol = 0.9; fvolbar = 0.3;
		rgsz[0] = "player/pl_metal1.wav";
		rgsz[1] = "player/pl_metal2.wav";
		cnt = 2;
		break;
	case CHAR_TEX_DIRT:	fvol = 0.9; fvolbar = 0.1;
		rgsz[0] = "player/pl_dirt1.wav";
		rgsz[1] = "player/pl_dirt2.wav";
		rgsz[2] = "player/pl_dirt3.wav";
		cnt = 3;
		break;
	case CHAR_TEX_VENT:	fvol = 0.5; fvolbar = 0.3;
		rgsz[0] = "player/pl_duct1.wav";
		rgsz[1] = "player/pl_duct1.wav";
		cnt = 2;
		break;
	case CHAR_TEX_GRATE: fvol = 0.9; fvolbar = 0.5;
		rgsz[0] = "player/pl_grate1.wav";
		rgsz[1] = "player/pl_grate4.wav";
		cnt = 2;
		break;
	case CHAR_TEX_TILE:	fvol = 0.8; fvolbar = 0.2;
		rgsz[0] = "player/pl_tile1.wav";
		rgsz[1] = "player/pl_tile3.wav";
		rgsz[2] = "player/pl_tile2.wav";
		rgsz[3] = "player/pl_tile4.wav";
		cnt = 4;
		break;
	case CHAR_TEX_SLOSH: fvol = 0.9; fvolbar = 0.0;
		rgsz[0] = "player/pl_slosh1.wav";
		rgsz[1] = "player/pl_slosh3.wav";
		rgsz[2] = "player/pl_slosh2.wav";
		rgsz[3] = "player/pl_slosh4.wav";
		cnt = 4;
		break;
	case CHAR_TEX_WOOD: fvol = 0.9; fvolbar = 0.2;
		rgsz[0] = "debris/wood1.wav";
		rgsz[1] = "debris/wood2.wav";
		rgsz[2] = "debris/wood3.wav";
		cnt = 3;
		break;
	case CHAR_TEX_GLASS:
	case CHAR_TEX_COMPUTER:
		fvol = 0.8; fvolbar = 0.2;
		rgsz[0] = "debris/glass1.wav";
		rgsz[1] = "debris/glass2.wav";
		rgsz[2] = "debris/glass3.wav";
		cnt = 3;
		break;
	case CHAR_TEX_FLESH:
		if (iBulletType == BULLET_PLAYER_CROWBAR)
			return 0.0; // crowbar already makes this sound
		fvol = 1.0;	fvolbar = 0.2;
		rgsz[0] = "weapons/bullet_hit1.wav";
		rgsz[1] = "weapons/bullet_hit2.wav";
		fattn = 1.0;
		cnt = 2;
		break;
	}

	// play material hit sound
	gEngfuncs.pEventAPI->EV_PlaySound( 0, ptr->endpos, CHAN_STATIC, rgsz[gEngfuncs.pfnRandomLong(0,cnt-1)], fvol, fattn, 0, 96 + gEngfuncs.pfnRandomLong(0,0xf) );
	return fvolbar;
}

char *EV_HLDM_DamageDecal( physent_t *pe )
{
	static char decalname[ 32 ];
	int idx;

	if ( pe->classnumber == 1 )
	{
		idx = gEngfuncs.pfnRandomLong( 0, 2 );
		sprintf( decalname, "{break%i", idx + 1 );
	}
	else if ( pe->rendermode != kRenderNormal )
	{
		sprintf( decalname, "{bproof1" );
	}
	else
	{
		idx = gEngfuncs.pfnRandomLong( 0, 4 );
		sprintf( decalname, "{shot%i", idx + 1 );
	}
	return decalname;
}

void EV_HLDM_GunshotDecalTrace( pmtrace_t *pTrace, char *decalName )
{
	int iRand;
	physent_t *pe;

	gEngfuncs.pEfxAPI->R_BulletImpactParticles( pTrace->endpos );

	iRand = gEngfuncs.pfnRandomLong(0,0x7FFF);
	if ( iRand < (0x7fff/2) )// not every bullet makes a sound.
	{
		switch( iRand % 5)
		{
		case 0:	gEngfuncs.pEventAPI->EV_PlaySound( -1, pTrace->endpos, 0, "weapons/ric1.wav", 1.0, ATTN_NORM, 0, PITCH_NORM ); break;
		case 1:	gEngfuncs.pEventAPI->EV_PlaySound( -1, pTrace->endpos, 0, "weapons/ric2.wav", 1.0, ATTN_NORM, 0, PITCH_NORM ); break;
		case 2:	gEngfuncs.pEventAPI->EV_PlaySound( -1, pTrace->endpos, 0, "weapons/ric3.wav", 1.0, ATTN_NORM, 0, PITCH_NORM ); break;
		case 3:	gEngfuncs.pEventAPI->EV_PlaySound( -1, pTrace->endpos, 0, "weapons/ric4.wav", 1.0, ATTN_NORM, 0, PITCH_NORM ); break;
		case 4:	gEngfuncs.pEventAPI->EV_PlaySound( -1, pTrace->endpos, 0, "weapons/ric5.wav", 1.0, ATTN_NORM, 0, PITCH_NORM ); break;
		}
	}

	pe = gEngfuncs.pEventAPI->EV_GetPhysent( pTrace->ent );

	// Only decal brush models such as the world etc.
	if (  decalName && decalName[0] && pe && ( pe->solid == SOLID_BSP || pe->movetype == MOVETYPE_PUSHSTEP ) )
	{
		if ( CVAR_GET_FLOAT( "r_decals" ) )
		{
			gEngfuncs.pEfxAPI->R_DecalShoot( 
				gEngfuncs.pEfxAPI->Draw_DecalIndex( gEngfuncs.pEfxAPI->Draw_DecalIndexFromName( decalName ) ), 
				gEngfuncs.pEventAPI->EV_IndexFromTrace( pTrace ), 0, pTrace->endpos, 0 );
		}
	}
}

void EV_HLDM_DecalGunshot( pmtrace_t *pTrace, int iBulletType )
{
	physent_t *pe;

	pe = gEngfuncs.pEventAPI->EV_GetPhysent( pTrace->ent );

	if ( pe && pe->solid == SOLID_BSP )
	{
		switch( iBulletType )
		{
		case BULLET_PLAYER_9MM:
		case BULLET_MONSTER_9MM:
		case BULLET_PLAYER_MP5:
		case BULLET_MONSTER_MP5:
		case BULLET_PLAYER_BUCKSHOT:
		case BULLET_PLAYER_357:
		default:
			// smoke and decal
			EV_HLDM_GunshotDecalTrace( pTrace, EV_HLDM_DamageDecal( pe ) );
			break;
		}
	}
}

int EV_HLDM_CheckTracer( int idx, float *vecSrc, float *end, float *forward, float *right, int iBulletType, int iTracerFreq, int *tracerCount )
{
	int tracer = 0;
	int i;
	qboolean player = idx >= 1 && idx <= gEngfuncs.GetMaxClients() ? true : false;

	if ( iTracerFreq != 0 && ( (*tracerCount)++ % iTracerFreq) == 0 )
	{
		vec3_t vecTracerSrc;

		if ( player )
		{
			vec3_t offset( 0, 0, -4 );

			// adjust tracer position for player
			for ( i = 0; i < 3; i++ )
			{
				vecTracerSrc[ i ] = vecSrc[ i ] + offset[ i ] + right[ i ] * 2 + forward[ i ] * 16;
			}
		}
		else
		{
			VectorCopy( vecSrc, vecTracerSrc );
		}
		
		if ( iTracerFreq != 1 )		// guns that always trace also always decal
			tracer = 1;

		switch( iBulletType )
		{
		case BULLET_PLAYER_MP5:
		case BULLET_MONSTER_MP5:
		case BULLET_MONSTER_9MM:
		case BULLET_MONSTER_12MM:
		default:
			EV_CreateTracer( vecTracerSrc, end );
			break;
		}
	}

	return tracer;
}


/*
================
FireBullets

Go to the trouble of combining multiple pellets into a single damage call.
================
*/
void EV_HLDM_FireBullets( int idx, float *forward, float *right, float *up, int cShots, float *vecSrc, float *vecDirShooting, float flDistance, int iBulletType, int iTracerFreq, int *tracerCount, float flSpreadX, float flSpreadY )
{
	flSpreadX *= spreadMultiplier;
	flSpreadY *= spreadMultiplier;

	int i;
	pmtrace_t tr;
	int iShot;
	int tracer;
	
	for ( iShot = 1; iShot <= cShots; iShot++ )	
	{
		vec3_t vecDir, vecEnd;
			
		float x, y, z;
		//We randomize for the Shotgun.
		if ( iBulletType == BULLET_PLAYER_BUCKSHOT )
		{
			do {
				x = gEngfuncs.pfnRandomFloat(-0.5,0.5) + gEngfuncs.pfnRandomFloat(-0.5,0.5);
				y = gEngfuncs.pfnRandomFloat(-0.5,0.5) + gEngfuncs.pfnRandomFloat(-0.5,0.5);
				z = x*x+y*y;
			} while (z > 1);

			for ( i = 0 ; i < 3; i++ )
			{
				vecDir[i] = vecDirShooting[i] + x * flSpreadX * right[ i ] + y * flSpreadY * up [ i ];
				vecEnd[i] = vecSrc[ i ] + flDistance * vecDir[ i ];
			}
		}//But other guns already have their spread randomized in the synched spread.
		else
		{

			for ( i = 0 ; i < 3; i++ )
			{
				vecDir[i] = vecDirShooting[i] + flSpreadX * right[ i ] + flSpreadY * up [ i ];
				vecEnd[i] = vecSrc[ i ] + flDistance * vecDir[ i ];
			}
		}

		gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction( false, true );
	
		// Store off the old count
		gEngfuncs.pEventAPI->EV_PushPMStates();
	
		// Now add in all of the players.
		gEngfuncs.pEventAPI->EV_SetSolidPlayers ( idx - 1 );	

		gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
		gEngfuncs.pEventAPI->EV_PlayerTrace( vecSrc, vecEnd, PM_STUDIO_BOX, -1, &tr );

		tr.endpos = tr.endpos + ( upsideDown ? Vector( 0, 0, -4 ) : Vector( 0, 0, 4 ) );
		tracer = EV_HLDM_CheckTracer( idx, vecSrc, tr.endpos, forward, right, iBulletType, iTracerFreq, tracerCount );

		// do damage, paint decals
		if ( tr.fraction != 1.0 )
		{
			switch(iBulletType)
			{
			default:
			case BULLET_PLAYER_9MM:		
				
				EV_HLDM_PlayTextureSound( idx, &tr, vecSrc, vecEnd, iBulletType );
				EV_HLDM_DecalGunshot( &tr, iBulletType );
			
					break;
			case BULLET_PLAYER_MP5:		
				
				if ( !tracer )
				{
					EV_HLDM_PlayTextureSound( idx, &tr, vecSrc, vecEnd, iBulletType );
					EV_HLDM_DecalGunshot( &tr, iBulletType );
				}
				break;
			case BULLET_PLAYER_BUCKSHOT:
				
				EV_HLDM_DecalGunshot( &tr, iBulletType );
			
				break;
			case BULLET_PLAYER_357:
				
				EV_HLDM_PlayTextureSound( idx, &tr, vecSrc, vecEnd, iBulletType );
				EV_HLDM_DecalGunshot( &tr, iBulletType );
				
				break;

			}
		}

		gEngfuncs.pEventAPI->EV_PopPMStates();
	}
}

//======================
//	    GLOCK START
//======================
void EV_FireGlock1( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;
	int empty;

	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	int shell;
	vec3_t vecSrc, vecAiming;
	vec3_t up, right, forward;
	
	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	ApplyAimOffset( angles );

	empty = args->bparam1;
	int shouldProducePhysicalBullets = args->bparam2;
	AngleVectors( angles, forward, right, up );

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/shell.mdl");// brass shell

	if ( EV_IsLocal( idx ) )
	{
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( empty ? GLOCK_SHOOT_EMPTY : GLOCK_SHOOT, 2 );

		V_PunchAxis( 0, -2.0 * ( upsideDown ? -1 : 1 ) );
	}

	EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -5, 10 );

	EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHELL ); 

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/pl_gun3.wav", gEngfuncs.pfnRandomFloat(0.92, 1.0), ATTN_NORM, 0, 98 + gEngfuncs.pfnRandomLong( 0, 3 ) );

	if ( shouldProducePhysicalBullets ) {
		return;
	}

	EV_GetGunPosition( args, vecSrc, origin );

	vecSrc = vecSrc + forward * 5;

	float rightOffset = 8;
	float upOffset = upsideDown ? -10 : 0;
	VectorSkew( vecSrc, angles, forward, vecAiming, rightOffset );
	EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc + right * rightOffset + up * upOffset, vecAiming, 8192, BULLET_PLAYER_9MM, 1, &tracerCount[idx-1], args->fparam1, args->fparam2 );
}

void EV_FireGlock2( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;
	
	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	int shell;
	vec3_t vecSrc, vecAiming;
	vec3_t vecSpread;
	vec3_t up, right, forward;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	ApplyAimOffset( angles );

	AngleVectors( angles, forward, right, up );

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/shell.mdl");// brass shell

	if ( EV_IsLocal( idx ) )
	{
		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( GLOCK_SHOOT, 2 );

		V_PunchAxis( 0, -2.0 * ( upsideDown ? -1 : 1 ) );
	}

	EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -12, 4 );

	EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHELL ); 

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/pl_gun3.wav", gEngfuncs.pfnRandomFloat(0.92, 1.0), ATTN_NORM, 0, 98 + gEngfuncs.pfnRandomLong( 0, 3 ) );

	EV_GetGunPosition( args, vecSrc, origin );
	
	VectorCopy( forward, vecAiming );

	EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc, vecAiming, 8192, BULLET_PLAYER_9MM, 0, &tracerCount[idx-1], args->fparam1, args->fparam2 );
	
}
//======================
//	   GLOCK END
//======================

//======================
//	 GLOCK TWIN START
//======================
void EV_FireGlockTwin( event_args_t *args ) {
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;

	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	int shell;
	vec3_t vecSrc, vecAiming;
	vec3_t up, right, forward;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	ApplyAimOffset( angles );

	int empty = args->iparam1;
	int empty2 = args->iparam2;
	int shouldProducePhysicalBullets = args->bparam1;
	int shootingRight = args->bparam2;

	AngleVectors( angles, forward, right, up );

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex( "models/shell.mdl" );// brass shell

	if ( EV_IsLocal( idx ) )
	{
		EV_MuzzleFlash();
		int anim;
		if ( shootingRight ) {
			if ( empty && empty2 ) {
				anim = GLOCK_TWIN_SHOOT_RIGHT_THEN_EMPTY_WHEN_LEFT_EMPTY;
			} else {
				if ( empty2 ) {
					anim = empty ? GLOCK_TWIN_SHOOT_RIGHT_THEN_EMPTY_WHEN_LEFT_EMPTY : GLOCK_TWIN_SHOOT_RIGHT_WHEN_LEFT_EMPTY;
				} else {
					anim = empty ? GLOCK_TWIN_SHOOT_RIGHT_THEN_EMPTY : GLOCK_TWIN_SHOOT_RIGHT;
				}
			}
		} else {
			if ( empty && empty2 ) {
				anim = GLOCK_TWIN_SHOOT_LEFT_THEN_EMPTY_WHEN_RIGHT_EMPTY;
			} else {
				if ( empty ) {
					anim = empty2 ? GLOCK_TWIN_SHOOT_LEFT_THEN_EMPTY_WHEN_RIGHT_EMPTY : GLOCK_TWIN_SHOOT_LEFT_WHEN_RIGHT_EMPTY;
				} else {
					anim = empty2 ? GLOCK_TWIN_SHOOT_LEFT_THEN_EMPTY : GLOCK_TWIN_SHOOT_LEFT;
				}
			}		
		}
		if ( isSlowmotionEnabled() ) {
			anim++;
		}

		gEngfuncs.pEventAPI->EV_WeaponAnimation( anim, 2 );

		V_PunchAxis( 0, -2.0 * ( upsideDown ? -1 : 1 ) );
	}

	if ( shootingRight ) {
		EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -5, 10 );
	} else {
		EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -2, -10 );
	}

	EV_EjectBrass( ShellOrigin, ShellVelocity, angles[YAW], shell, TE_BOUNCE_SHELL );

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/pl_gun3.wav", gEngfuncs.pfnRandomFloat( 0.92, 1.0 ), ATTN_NORM, 0, 98 + gEngfuncs.pfnRandomLong( 0, 3 ) );

	if ( shouldProducePhysicalBullets ) {
		return;
	}

	EV_GetGunPosition( args, vecSrc, origin );

	vecSrc = vecSrc + forward * 5;

	float rightOffset = shootingRight ? 8 : -8;
	float upOffset = upsideDown ? -10 : 0;
	VectorSkew( vecSrc, angles, forward, vecAiming, rightOffset );
	EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc + right * rightOffset + up * upOffset, vecAiming, 8192, BULLET_PLAYER_9MM, 1, &tracerCount[idx-1], args->fparam1, args->fparam2 );
}
//======================
//	  GLOCK TWIN END
//======================

void EV_FireIngram( event_args_t *args )
{
	if ( !AllowedToFireRapidEvent( latestShoot ) ) {
		return;
	}

	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;
	int empty;

	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	int shell;
	vec3_t vecSrc, vecAiming;
	vec3_t up, right, forward;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	ApplyAimOffset( angles );

	// interpret iparam1 back as float
	float stress = max( 0.4f, ( ( *( float * ) &args->iparam1 ) ) );

	empty = args->bparam1;
	int shouldProducePhysicalBullets = args->bparam2;
	AngleVectors( angles, forward, right, up );

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/shell.mdl");// brass shell

	if ( EV_IsLocal( idx ) )
	{
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( empty ? INGRAM_SHOOT_EMPTY : ( INGRAM_SHOOT_1 + gEngfuncs.pfnRandomLong( 0, 2 ) ), 2 );

		V_PunchAxis( 0, gEngfuncs.pfnRandomFloat( -stress * 0.25f, stress * 0.25f ) * ( upsideDown ? -1 : 1 ) );
	}

	EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -5, 10 );

	EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHELL ); 

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/ingram_shot.wav", gEngfuncs.pfnRandomFloat(0.92, 1.0), ATTN_NORM, 0, 98 + gEngfuncs.pfnRandomLong( 0, 3 ) );

	if ( shouldProducePhysicalBullets ) {
		return;
	}

	EV_GetGunPosition( args, vecSrc, origin );

	vecSrc = vecSrc + forward * 5;

	float rightOffset = 8;
	float upOffset = upsideDown ? -10 : 0;
	VectorSkew( vecSrc, angles, forward, vecAiming, rightOffset );
	EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc + right * rightOffset + up * upOffset, vecAiming, 8192, BULLET_PLAYER_9MM, 1, &tracerCount[idx-1], args->fparam1, args->fparam2 );
}

void EV_FireIngramTwin( event_args_t *args ) {
	if ( !AllowedToFireRapidEvent( latestShoot ) ) {
		return;
	}

	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;

	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	int shell;
	vec3_t vecSrc, vecAiming;
	vec3_t up, right, forward;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );
	
	ApplyAimOffset( angles );
	
	int empty = args->iparam1 & 1;
	int empty2 = args->iparam1 & 2;

	// interpret iparam2 back as float
	float stress = max( 0.4f, ( ( *( float * ) &args->iparam2 ) ) );

	int shouldProducePhysicalBullets = args->bparam1;
	bool shootingBoth = args->bparam2 == 2;
	bool shootingLeft = args->bparam2 == 1;
	bool shootingRight = args->bparam2 == 0;

	AngleVectors( angles, forward, right, up );

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex( "models/shell.mdl" );// brass shell

	if ( EV_IsLocal( idx ) )
	{
		EV_MuzzleFlash();
		int anim;

		if ( shootingBoth ) {
			if ( empty && empty2 ) {
				anim = INGRAM_TWIN_SHOOT_BOTH_THEN_EMPTY;
			} else if ( empty2 ) {
				anim = INGRAM_TWIN_SHOOT_BOTH_THEN_LEFT_EMPTY;
			} else {
				anim = empty ? INGRAM_TWIN_SHOOT_BOTH_THEN_RIGHT_EMPTY : ( INGRAM_TWIN_SHOOT_BOTH_1 + gEngfuncs.pfnRandomLong( 0, 2 ) );
			}

		} else if ( shootingRight ) {
			if ( empty ) {
				anim = INGRAM_TWIN_SHOOT_RIGHT_THEN_EMPTY_WHEN_LEFT_EMPTY;
			} else {
				anim = INGRAM_TWIN_SHOOT_RIGHT_WHEN_LEFT_EMPTY_1 + gEngfuncs.pfnRandomLong( 0, 2 );
			}
		} else {
			if ( empty2 ) {
				anim = INGRAM_TWIN_SHOOT_LEFT_THEN_EMPTY_WHEN_RIGHT_EMPTY;
			} else {
				anim = INGRAM_TWIN_SHOOT_LEFT_WHEN_RIGHT_EMPTY_1 + gEngfuncs.pfnRandomLong( 0, 2 );
			}
		}

		gEngfuncs.pEventAPI->EV_WeaponAnimation( anim, 2 );
		V_PunchAxis( 0, gEngfuncs.pfnRandomFloat( -stress * 0.25f, stress * 0.25f ) * ( upsideDown ? -1 : 1 ) );
	}

	if ( !empty ) {
		EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -5, 10 );
		EV_EjectBrass( ShellOrigin, ShellVelocity, angles[YAW], shell, TE_BOUNCE_SHELL );
	}

	if ( !empty2 ) {
		EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -3, -10 );
		EV_EjectBrass( ShellOrigin, ShellVelocity, angles[YAW], shell, TE_BOUNCE_SHELL );
	}

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/ingram_shot.wav", gEngfuncs.pfnRandomFloat( 0.92, 1.0 ), ATTN_NORM, 0, 98 + gEngfuncs.pfnRandomLong( 0, 3 ) );

	// Tracers are in function below
	// Separate tracer event was made to ease up producing tracers with different destination vectors
}

void EV_FireIngramTwinTracer( event_args_t *args ) {
	int shootingRight = args->bparam2;

	if ( shootingRight && !AllowedToFireRapidEvent( latestTracer ) ) {
		return;
	}

	if ( !shootingRight && !AllowedToFireRapidEvent( latestTracer2 ) ) {
		return;
	}

	int idx = args->entindex;
	vec3_t origin;
	vec3_t angles;

	vec3_t vecSrc, vecAiming;
	vec3_t up, right, forward;

	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );

	ApplyAimOffset( angles );

	AngleVectors( angles, forward, right, up );
	EV_GetGunPosition( args, vecSrc, origin );

	vecSrc = vecSrc + forward * 5;

	float upOffset = upsideDown ? -10 : 0;
	

	float rightOffset = shootingRight ? 8 : -8;
	VectorSkew( vecSrc, angles, forward, vecAiming, rightOffset );
	EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc + right * rightOffset + up * upOffset, vecAiming, 8192, BULLET_PLAYER_9MM, 1, &tracerCount[idx-1], args->fparam1, args->fparam2 );
}

//======================
//	  SHOTGUN START
//======================
void EV_FireShotGunDouble( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;

	int j;
	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	int shell;
	vec3_t vecSrc, vecAiming;
	vec3_t vecSpread;
	vec3_t up, right, forward;
	float flSpread = 0.01;

	int shouldProducePhysicalBullets = args->bparam1;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	ApplyAimOffset( angles );

	AngleVectors( angles, forward, right, up );

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/shotgunshell.mdl");// brass shell

	

	int anim = SHOTGUN_FIRE2;
	if ( isSlowmotionEnabled() ) {
		anim = SHOTGUN_FIRE2_FAST;
	}

	if ( EV_IsLocal( idx ) )
	{
		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( anim, 2 );
		V_PunchAxisAdditive( 0, -5.0 * ( upsideDown ? -1 : 1 ) );
	}

	for ( j = 0; j < 2; j++ )
	{
		EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 32, -12, 18 );

		EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHOTSHELL ); 
	}

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/dbarrel1.wav", gEngfuncs.pfnRandomFloat(0.98, 1.0), ATTN_NORM, 0, 85 + gEngfuncs.pfnRandomLong( 0, 0x1f ) );

	if ( shouldProducePhysicalBullets ) {
		return;
	}

	EV_GetGunPosition( args, vecSrc, origin );
	VectorCopy( forward, vecAiming );

	if ( gEngfuncs.GetMaxClients() > 1 )
	{
		EV_HLDM_FireBullets( idx, forward, right, up, 8, vecSrc, vecAiming, 2048, BULLET_PLAYER_BUCKSHOT, 1, &tracerCount[idx-1], 0.17365, 0.04362 );
	}
	else
	{
		EV_HLDM_FireBullets( idx, forward, right, up, 12, vecSrc, vecAiming, 2048, BULLET_PLAYER_BUCKSHOT, 1, &tracerCount[idx-1], 0.08716, 0.08716 );
	}
}

void EV_FireShotGunSingle( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;
	
	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	int shell;
	vec3_t vecSrc, vecAiming;
	vec3_t vecSpread;
	vec3_t up, right, forward;
	float flSpread = 0.01;

	int shouldProducePhysicalBullets = args->bparam1;
	int isAutomaticShotgun = args->bparam2;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	ApplyAimOffset( angles );

	AngleVectors( angles, forward, right, up );

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/shotgunshell.mdl");// brass shell

	int anim = isAutomaticShotgun ? SHOTGUN_FIRE_NO_RELOAD : SHOTGUN_FIRE;
	if ( isSlowmotionEnabled() ) {
		anim++;
	}

	if ( EV_IsLocal( idx ) )
	{
		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( anim, 2 );

		V_PunchAxisAdditive( 0, -2.5 * ( upsideDown ? -1 : 1 ) );
	}

	EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 32, -12, 18 );

	EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHOTSHELL ); 

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/sbarrel1.wav", gEngfuncs.pfnRandomFloat(0.95, 1.0), ATTN_NORM, 0, 93 + gEngfuncs.pfnRandomLong( 0, 0x1f ) );

	if ( shouldProducePhysicalBullets ) {
		return;
	}

	EV_GetGunPosition( args, vecSrc, origin );
	VectorCopy( forward, vecAiming );

	if ( gEngfuncs.GetMaxClients() > 1 )
	{
		EV_HLDM_FireBullets( idx, forward, right, up, 4, vecSrc, vecAiming, 2048, BULLET_PLAYER_BUCKSHOT, 1, &tracerCount[idx-1], 0.08716, 0.04362 );
	}
	else
	{
		EV_HLDM_FireBullets( idx, forward, right, up, 6, vecSrc, vecAiming, 2048, BULLET_PLAYER_BUCKSHOT, 1, &tracerCount[idx-1], 0.08716, 0.08716 );
	}
}
//======================
//	   SHOTGUN END
//======================


//======================
//	    MP5 START
//======================
void EV_FireMP5( event_args_t *args )
{
	if ( !AllowedToFireRapidEvent( latestShoot ) ) {
		return;
	}

	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;

	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	int shell;
	vec3_t vecSrc, vecAiming;
	vec3_t up, right, forward;
	float flSpread = 0.01;

	// interpret iparam1 back as float
	float stress = max( 0.4f, ( ( *( float * ) &args->iparam1 ) ) );

	int shouldProducePhysicalBullets = args->bparam1;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	ApplyAimOffset( angles );

	AngleVectors( angles, forward, right, up );

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/shell.mdl");// brass shell
	
	if ( EV_IsLocal( idx ) )
	{
		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( MP5_FIRE1 + gEngfuncs.pfnRandomLong(0,2), 2 );

		// Punch is higher with the stress
		V_PunchAxis( 0, gEngfuncs.pfnRandomFloat( -stress * 0.5f, stress * 0.5f ) * ( upsideDown ? -1 : 1 ) );
	}

	EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -12, 4 );

	EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHELL ); 

	switch( gEngfuncs.pfnRandomLong( 0, 1 ) )
	{
	case 0:
		gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/hks1.wav", 1, ATTN_NORM, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );
		break;
	case 1:
		gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/hks2.wav", 1, ATTN_NORM, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );
		break;
	}

	if ( shouldProducePhysicalBullets ) {
		return;
	}

	EV_GetGunPosition( args, vecSrc, origin );
	VectorCopy( forward, vecAiming );

	float upOffset = upsideDown ? -10 : 0;

	if ( gEngfuncs.GetMaxClients() > 1 )
	{
		EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc + up * upOffset, vecAiming, 8192, BULLET_PLAYER_MP5, 1, &tracerCount[idx-1], args->fparam1, args->fparam2 );
	}
	else
	{
		EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc + up * upOffset, vecAiming, 8192, BULLET_PLAYER_MP5, 1, &tracerCount[idx-1], args->fparam1, args->fparam2 );
	}
}

// We only predict the animation and sound
// The grenade is still launched from the server.
void EV_FireMP52( event_args_t *args )
{
	int idx;
	vec3_t origin;
	
	idx = args->entindex;
	VectorCopy( args->origin, origin );

	if ( EV_IsLocal( idx ) )
	{
		gEngfuncs.pEventAPI->EV_WeaponAnimation( MP5_LAUNCH, 2 );
		V_PunchAxisAdditive( 0, -5 * ( upsideDown ? -1 : 1 ) );
	}
	
	switch( gEngfuncs.pfnRandomLong( 0, 1 ) )
	{
	case 0:
		gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/glauncher.wav", 1, ATTN_NORM, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );
		break;
	case 1:
		gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/glauncher2.wav", 1, ATTN_NORM, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );
		break;
	}
}
//======================
//		 MP5 END
//======================

//======================
//	    M249 START
//======================

void EV_FireM249( event_args_t *args ) {
	if ( !AllowedToFireRapidEvent( latestShoot ) ) {
		return;
	}

	float stress = max( 0.4f, ( ( *( float * ) &args->iparam2 ) ) );
	int iBody = args->iparam1;
	int shouldProducePhysicalBullets = args->bparam1;

	static bool bAlternatingEject = false;
	bAlternatingEject = !bAlternatingEject;

	Vector up, right, forward;

	AngleVectors( args->angles, forward, right, up );

	int iShell = bAlternatingEject ?
		gEngfuncs.pEventAPI->EV_FindModelIndex( "models/saw_link.mdl" ) :
		gEngfuncs.pEventAPI->EV_FindModelIndex( "models/saw_shell.mdl" );

	if ( EV_IsLocal( args->entindex ) ) {
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( gEngfuncs.pfnRandomLong( 0, 2 ) + M249_SHOOT1, iBody );
		V_PunchAxis( 0, gEngfuncs.pfnRandomFloat( -0.8, 0.8 ) * stress );
		V_PunchAxis( 1, gEngfuncs.pfnRandomFloat( -0.4, 0.4 ) * stress );
	}

	Vector ShellVelocity;
	Vector ShellOrigin;

	EV_GetDefaultShellInfo( args, args->origin, args->velocity, ShellVelocity, ShellOrigin, forward, right, up, 8, -2, 4 );

	EV_EjectBrass( ShellOrigin, ShellVelocity, args->angles[1], iShell, TE_BOUNCE_SHELL );

	gEngfuncs.pEventAPI->EV_PlaySound( args->entindex, args->origin, CHAN_WEAPON, "weapons/saw_fire1.wav", VOL_NORM, ATTN_NORM, 0, 94 + gEngfuncs.pfnRandomLong( 0, 15 ) );

	if ( shouldProducePhysicalBullets ) {
		return;
	}

	Vector vecSrc;
	EV_GetGunPosition( args, vecSrc, args->origin );

	EV_HLDM_FireBullets( args->entindex, forward, right, up, 1, vecSrc, forward, 8192.0, BULLET_PLAYER_9MM, 0, 0, args->fparam1, args->fparam2 );
}

//======================
//	   M249 END
//======================

//======================
//	   PHYTON START 
//	     ( .357 )
//======================
void EV_FirePython( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;

	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	int shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/shell.mdl");
	vec3_t vecSrc, vecAiming;
	vec3_t up, right, forward;
	float flSpread = 0.01;

	int empty = args->bparam1;
	int shouldProducePhysicalBullets = args->bparam2;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	ApplyAimOffset( angles );

	AngleVectors( angles, forward, right, up );

	if ( EV_IsLocal( idx ) )
	{
		// Python uses different body in multiplayer versus single player
		int multiplayer = gEngfuncs.GetMaxClients() == 1 ? 0 : 1;

		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( empty ? PYTHON_FIRE1_NOSHOT : PYTHON_FIRE1, multiplayer ? 1 : 0 );

		V_PunchAxisAdditive( 0, -5.0 * ( upsideDown ? -1 : 1 ) );
	}

	switch( gEngfuncs.pfnRandomLong( 0, 1 ) )
	{
	case 0:
		gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/357_shot1.wav", gEngfuncs.pfnRandomFloat(0.8, 0.9), ATTN_NORM, 0, PITCH_NORM );
		break;
	case 1:
		gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/357_shot2.wav", gEngfuncs.pfnRandomFloat(0.8, 0.9), ATTN_NORM, 0, PITCH_NORM );
		break;
	}

	EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -5, 10 );

	EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHELL ); 

	if ( shouldProducePhysicalBullets ) {
		return;
	}

	EV_GetGunPosition( args, vecSrc, origin );
	
	float rightOffset = 5;
	float upOffset = upsideDown ? -10 : 0;
	VectorSkew( vecSrc, angles, forward, vecAiming, rightOffset );
	EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc + right * rightOffset + up * upOffset, vecAiming, 8192, BULLET_PLAYER_357, 1, &tracerCount[idx-1], args->fparam1, args->fparam2 );
}
//======================
//	    PHYTON END 
//	     ( .357 )
//======================

//======================
//	   GAUSS START 
//======================
#define SND_CHANGE_PITCH	(1<<7)		// duplicated in protocol.h change sound pitch

void EV_SpinGauss( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;
	int iSoundState = 0;

	int pitch;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	pitch = args->iparam1;

	iSoundState = args->bparam1 ? SND_CHANGE_PITCH : 0;

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "ambience/pulsemachine.wav", 1.0, ATTN_NORM, iSoundState, pitch );
}

/*
==============================
EV_StopPreviousGauss

==============================
*/
void EV_StopPreviousGauss( int idx )
{
	// Make sure we don't have a gauss spin event in the queue for this guy
	gEngfuncs.pEventAPI->EV_KillEvents( idx, "events/gaussspin.sc" );
	gEngfuncs.pEventAPI->EV_StopSound( idx, CHAN_WEAPON, "ambience/pulsemachine.wav" );
}

extern float g_flApplyVel;

void EV_FireGauss( event_args_t *args )
{
	if ( !AllowedToFireRapidEvent( latestTracer3 ) ) {
		return;
	}

	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;
	float flDamage = args->fparam1;
	int primaryfire = args->bparam1;
	int instaGib = args->iparam1;

	int m_fPrimaryFire = args->bparam1;
	int m_iWeaponVolume = GAUSS_PRIMARY_FIRE_VOLUME;
	vec3_t vecSrc;
	vec3_t vecDest;
	edict_t		*pentIgnore;
	pmtrace_t tr, beam_tr;
	float flMaxFrac = 1.0;
	int	nTotal = 0;
	int fHasPunched = 0;
	int fFirstBeam = 1;
	int	nMaxHits = 10;
	physent_t *pEntity;
	int m_iBeam, m_iGlow, m_iBalls;
	vec3_t up, right, forward;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	ApplyAimOffset( angles );

	if ( args->bparam2 )
	{
		EV_StopPreviousGauss( idx );
		return;
	}

//	Con_Printf( "Firing gauss with %f\n", flDamage );
	EV_GetGunPosition( args, vecSrc, origin );

	m_iBeam = gEngfuncs.pEventAPI->EV_FindModelIndex( "sprites/smoke.spr" );
	if ( instaGib ) {
		m_iBalls = m_iGlow = gEngfuncs.pEventAPI->EV_FindModelIndex( "sprites/hotglow2.spr" );
	} else {
		m_iBalls = m_iGlow = gEngfuncs.pEventAPI->EV_FindModelIndex( "sprites/hotglow.spr" );
	}
	
	
	AngleVectors( angles, forward, right, up );

	VectorMA( vecSrc, 8192, forward, vecDest );

	if ( EV_IsLocal( idx ) )
	{
		if ( !instaGib ) {
			V_PunchAxis( 0, -2.0 * ( upsideDown ? -1 : 1 ) );
		}
		gEngfuncs.pEventAPI->EV_WeaponAnimation( GAUSS_FIRE2, 2 );

		if ( m_fPrimaryFire == false )
			 g_flApplyVel = flDamage;	
			 
	}

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/gauss2.wav", 0.5 + flDamage * (1.0 / 400.0), ATTN_NORM, 0, 85 + gEngfuncs.pfnRandomLong( 0, 0x1f ) );

	while (flDamage > 10 && nMaxHits > 0)
	{
		nMaxHits--;

		gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction( false, true );
		
		// Store off the old count
		gEngfuncs.pEventAPI->EV_PushPMStates();
	
		// Now add in all of the players.
		gEngfuncs.pEventAPI->EV_SetSolidPlayers ( idx - 1 );	

		gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
		gEngfuncs.pEventAPI->EV_PlayerTrace( vecSrc, vecDest, PM_STUDIO_BOX, -1, &tr );

		gEngfuncs.pEventAPI->EV_PopPMStates();

		if ( tr.allsolid )
			break;

		int primaryGColor = instaGib ? 0 : 128;
		int secondaryGBColor = instaGib ? 0 : 255;

		if (fFirstBeam)
		{
			if ( EV_IsLocal( idx ) )
			{
				// Add muzzle flash to current weapon model
				EV_MuzzleFlash();
			}
			fFirstBeam = 0;

			gEngfuncs.pEfxAPI->R_BeamEntPoint( 
				idx | 0x1000,
				tr.endpos,
				m_iBeam,
				0.1,
				m_fPrimaryFire ? 1.0 : 2.5,
				0.0,
				m_fPrimaryFire ? 128.0 : flDamage,
				0,
				0,
				0,
				m_fPrimaryFire ? 255 : 255,
				m_fPrimaryFire ? primaryGColor : secondaryGBColor,
				m_fPrimaryFire ? 0 : secondaryGBColor
			);
		}
		else
		{
			gEngfuncs.pEfxAPI->R_BeamPoints( vecSrc,
				tr.endpos,
				m_iBeam,
				0.1,
				m_fPrimaryFire ? 1.0 : 2.5,
				0.0,
				m_fPrimaryFire ? 128.0 : flDamage,
				0,
				0,
				0,
				m_fPrimaryFire ? 255 : 255,
				m_fPrimaryFire ? primaryGColor : secondaryGBColor,
				m_fPrimaryFire ? 0 : secondaryGBColor
			);
		}

		pEntity = gEngfuncs.pEventAPI->EV_GetPhysent( tr.ent );
		if ( pEntity == NULL )
			break;

		if ( pEntity->solid == SOLID_BSP )
		{
			float n;

			pentIgnore = NULL;

			n = -DotProduct( tr.plane.normal, forward );

			if (n < 0.5) // 60 degrees	
			{
				// ALERT( at_console, "reflect %f\n", n );
				// reflect
				vec3_t r;
			
				VectorMA( forward, 2.0 * n, tr.plane.normal, r );

				flMaxFrac = flMaxFrac - tr.fraction;
				
				VectorCopy( r, forward );

				VectorMA( tr.endpos, 8.0, forward, vecSrc );
				VectorMA( vecSrc, 8192.0, forward, vecDest );

				gEngfuncs.pEfxAPI->R_TempSprite( tr.endpos, vec3_origin, 0.2, m_iGlow, kRenderGlow, kRenderFxNoDissipation, flDamage * n / 255.0, flDamage * n * 0.5 * 0.1, FTENT_FADEOUT );

				vec3_t fwd;
				VectorAdd( tr.endpos, tr.plane.normal, fwd );

				gEngfuncs.pEfxAPI->R_Sprite_Trail( TE_SPRITETRAIL, tr.endpos, fwd, m_iBalls, 3, 0.1, gEngfuncs.pfnRandomFloat( 10, 20 ) / 100.0, 100,
									255, 100 );

				// lose energy
				if ( n == 0 )
				{
					n = 0.1;
				}
				
				flDamage = flDamage * (1 - n);

			}
			else
			{
				// tunnel
				EV_HLDM_DecalGunshot( &tr, BULLET_MONSTER_12MM );

				gEngfuncs.pEfxAPI->R_TempSprite( tr.endpos, vec3_origin, 1.0, m_iGlow, kRenderGlow, kRenderFxNoDissipation, flDamage / 255.0, 6.0, FTENT_FADEOUT );

				// limit it to one hole punch
				if (fHasPunched)
				{
					break;
				}
				fHasPunched = 1;
				
				// try punching through wall if secondary attack (primary is incapable of breaking through)
				if ( !m_fPrimaryFire )
				{
					vec3_t start;

					VectorMA( tr.endpos, 8.0, forward, start );

					// Store off the old count
					gEngfuncs.pEventAPI->EV_PushPMStates();
						
					// Now add in all of the players.
					gEngfuncs.pEventAPI->EV_SetSolidPlayers ( idx - 1 );

					gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
					gEngfuncs.pEventAPI->EV_PlayerTrace( start, vecDest, PM_STUDIO_BOX, -1, &beam_tr );

					if ( !beam_tr.allsolid )
					{
						vec3_t delta;
						float n;

						// trace backwards to find exit point

						gEngfuncs.pEventAPI->EV_PlayerTrace( beam_tr.endpos, tr.endpos, PM_STUDIO_BOX, -1, &beam_tr );

						VectorSubtract( beam_tr.endpos, tr.endpos, delta );
						
						n = Length( delta );

						if (n < flDamage)
						{
							if (n == 0)
								n = 1;
							flDamage -= n;

							// absorption balls
							{
								vec3_t fwd;
								VectorSubtract( tr.endpos, forward, fwd );
								gEngfuncs.pEfxAPI->R_Sprite_Trail( TE_SPRITETRAIL, tr.endpos, fwd, m_iBalls, 3, 0.1, gEngfuncs.pfnRandomFloat( 10, 20 ) / 100.0, 100,
									255, 100 );
							}

	//////////////////////////////////// WHAT TO DO HERE
							// CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, NORMAL_EXPLOSION_VOLUME, 3.0 );

							EV_HLDM_DecalGunshot( &beam_tr, BULLET_MONSTER_12MM );
							
							gEngfuncs.pEfxAPI->R_TempSprite( beam_tr.endpos, vec3_origin, 0.1, m_iGlow, kRenderGlow, kRenderFxNoDissipation, flDamage / 255.0, 6.0, FTENT_FADEOUT );
			
							// balls
							{
								vec3_t fwd;
								VectorSubtract( beam_tr.endpos, forward, fwd );
								gEngfuncs.pEfxAPI->R_Sprite_Trail( TE_SPRITETRAIL, beam_tr.endpos, fwd, m_iBalls, (int)(flDamage * 0.3), 0.1, gEngfuncs.pfnRandomFloat( 10, 20 ) / 100.0, 200,
									255, 40 );
							}
							
							VectorAdd( beam_tr.endpos, forward, vecSrc );
						}
					}
					else
					{
						flDamage = 0;
					}

					gEngfuncs.pEventAPI->EV_PopPMStates();
				}
				else
				{
					if ( m_fPrimaryFire )
					{
						// slug doesn't punch through ever with primary 
						// fire, so leave a little glowy bit and make some balls
						gEngfuncs.pEfxAPI->R_TempSprite( tr.endpos, vec3_origin, 0.2, m_iGlow, kRenderGlow, kRenderFxNoDissipation, 200.0 / 255.0, 0.3, FTENT_FADEOUT );
			
						{
							vec3_t fwd;
							VectorAdd( tr.endpos, tr.plane.normal, fwd );
							gEngfuncs.pEfxAPI->R_Sprite_Trail( TE_SPRITETRAIL, tr.endpos, fwd, m_iBalls, 8, 0.6, gEngfuncs.pfnRandomFloat( 10, 20 ) / 100.0, 100,
								255, 200 );
						}
					}

					flDamage = 0;
				}
			}
		}
		else
		{
			VectorAdd( tr.endpos, forward, vecSrc );
		}
	}
}
//======================
//	   GAUSS END 
//======================

//======================
//	   CROWBAR START
//======================

enum crowbar_e {
	CROWBAR_IDLE = 0,
	CROWBAR_DRAW,
	CROWBAR_HOLSTER,
	CROWBAR_ATTACK1HIT,
	CROWBAR_ATTACK1MISS,
	CROWBAR_ATTACK2MISS,
	CROWBAR_ATTACK2HIT,
	CROWBAR_ATTACK3MISS,
	CROWBAR_ATTACK3HIT
};

int g_iSwing;

//Only predict the miss sounds, hit sounds are still played 
//server side, so players don't get the wrong idea.
void EV_Crowbar( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	
	//Play Swing sound
	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/cbar_miss1.wav", 1, ATTN_NORM, 0, PITCH_NORM); 

	if ( EV_IsLocal( idx ) )
	{
		gEngfuncs.pEventAPI->EV_WeaponAnimation( CROWBAR_ATTACK1MISS, 1 );

		switch( (g_iSwing++) % 3 )
		{
			case 0:
				gEngfuncs.pEventAPI->EV_WeaponAnimation ( CROWBAR_ATTACK1MISS, 1 ); break;
			case 1:
				gEngfuncs.pEventAPI->EV_WeaponAnimation ( CROWBAR_ATTACK2MISS, 1 ); break;
			case 2:
				gEngfuncs.pEventAPI->EV_WeaponAnimation ( CROWBAR_ATTACK3MISS, 1 ); break;
		}
	}
}
//======================
//	   CROWBAR END 
//======================

//======================
//	   CHICKEN START
//======================

enum chicken_e {
	CHICKEN_IDLE_LIGHT1 = 0,
	CHICKEN_IDLE_LIGHT2,
	CHICKEN_IDLE_STRONG1,
	CHICKEN_IDLE_STRONG2,
	CHICKEN_DRAW_LIGHT,
	CHICKEN_DRAW_STRONG,
	CHICKEN_ATTACK_LIGHT1,
	CHICKEN_ATTACK_LIGHT2,
	CHICKEN_ATTACK_STRONG,
	CHICKEN_SWITCH_LIGHT_TO_STRONG,
	CHICKEN_SWITCH_STRONG_TO_LIGHT
};

void EV_Chicken( event_args_t *args ) {
	int idx = args->entindex;;
	vec3_t origin;

	VectorCopy( args->origin, origin );

	int swing = args->iparam1;
	int isLight = args->iparam2;

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/cbar_miss1.wav", 1, ATTN_NORM, 0, PITCH_NORM );

	if ( EV_IsLocal( idx ) && isLight ) {
		gEngfuncs.pEventAPI->EV_WeaponAnimation( swing > 0 ? CHICKEN_ATTACK_LIGHT2 : CHICKEN_ATTACK_LIGHT1, 1 );
	}
}
//======================
//	   CHICKEN END 
//======================

//======================
//	  CROSSBOW START
//======================
enum crossbow_e {
	CROSSBOW_IDLE1 = 0,	// full
	CROSSBOW_IDLE2,		// empty
	CROSSBOW_FIDGET1,	// full
	CROSSBOW_FIDGET2,	// empty
	CROSSBOW_FIRE1,		// full
	CROSSBOW_FIRE2,		// reload
	CROSSBOW_FIRE3,		// empty
	CROSSBOW_RELOAD,	// from empty
	CROSSBOW_RELOAD_FAST,
	CROSSBOW_DRAW1,		// full
	CROSSBOW_DRAW2,		// empty
	CROSSBOW_HOLSTER1,	// full
	CROSSBOW_HOLSTER2,	// empty
};

//=====================
// EV_BoltCallback
// This function is used to correct the origin and angles 
// of the bolt, so it looks like it's stuck on the wall.
//=====================
void EV_BoltCallback ( struct tempent_s *ent, float frametime, float currenttime )
{
	ent->entity.origin = ent->entity.baseline.vuser1;
	ent->entity.angles = ent->entity.baseline.vuser2;
}

void EV_FireCrossbow2( event_args_t *args )
{
	vec3_t vecSrc, vecEnd;
	vec3_t up, right, forward;
	pmtrace_t tr;

	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );

	ApplyAimOffset( angles );

	VectorCopy( args->velocity, velocity );
	
	AngleVectors( angles, forward, right, up );

	EV_GetGunPosition( args, vecSrc, origin );

	VectorMA( vecSrc, 8192, forward, vecEnd );

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/xbow_fire1.wav", 1, ATTN_NORM, 0, 93 + gEngfuncs.pfnRandomLong(0,0xF) );
	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_ITEM, "weapons/xbow_reload1.wav", gEngfuncs.pfnRandomFloat(0.95, 1.0), ATTN_NORM, 0, 93 + gEngfuncs.pfnRandomLong(0,0xF) );

	if ( EV_IsLocal( idx ) )
	{
		if ( args->iparam1 )
			gEngfuncs.pEventAPI->EV_WeaponAnimation( CROSSBOW_FIRE1, 1 );
		else if ( args->iparam2 )
			gEngfuncs.pEventAPI->EV_WeaponAnimation( CROSSBOW_FIRE3, 1 );
	}

	// Store off the old count
	gEngfuncs.pEventAPI->EV_PushPMStates();

	// Now add in all of the players.
	gEngfuncs.pEventAPI->EV_SetSolidPlayers ( idx - 1 );	
	gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
	gEngfuncs.pEventAPI->EV_PlayerTrace( vecSrc, vecEnd, PM_STUDIO_BOX, -1, &tr );
	
	//We hit something
	if ( tr.fraction < 1.0 )
	{
		physent_t *pe = gEngfuncs.pEventAPI->EV_GetPhysent( tr.ent ); 

		//Not the world, let's assume we hit something organic ( dog, cat, uncle joe, etc ).
		if ( pe->solid != SOLID_BSP )
		{
			switch( gEngfuncs.pfnRandomLong(0,1) )
			{
			case 0:
				gEngfuncs.pEventAPI->EV_PlaySound( idx, tr.endpos, CHAN_BODY, "weapons/xbow_hitbod1.wav", 1, ATTN_NORM, 0, PITCH_NORM ); break;
			case 1:
				gEngfuncs.pEventAPI->EV_PlaySound( idx, tr.endpos, CHAN_BODY, "weapons/xbow_hitbod2.wav", 1, ATTN_NORM, 0, PITCH_NORM ); break;
			}
		}
		//Stick to world but don't stick to glass, it might break and leave the bolt floating. It can still stick to other non-transparent breakables though.
		else if ( pe->rendermode == kRenderNormal ) 
		{
			gEngfuncs.pEventAPI->EV_PlaySound( 0, tr.endpos, CHAN_BODY, "weapons/xbow_hit1.wav", gEngfuncs.pfnRandomFloat(0.95, 1.0), ATTN_NORM, 0, PITCH_NORM );
		
			//Not underwater, do some sparks...
			if ( gEngfuncs.PM_PointContents( tr.endpos, NULL ) != CONTENTS_WATER)
				 gEngfuncs.pEfxAPI->R_SparkShower( tr.endpos );

			vec3_t vBoltAngles;
			int iModelIndex = gEngfuncs.pEventAPI->EV_FindModelIndex( "models/crossbow_bolt.mdl" );

			VectorAngles( forward, vBoltAngles );

			TEMPENTITY *bolt = gEngfuncs.pEfxAPI->R_TempModel( tr.endpos - forward * 10, Vector( 0, 0, 0), vBoltAngles , 5, iModelIndex, TE_BOUNCE_NULL );
			
			if ( bolt )
			{
				bolt->flags |= ( FTENT_CLIENTCUSTOM ); //So it calls the callback function.
				bolt->entity.baseline.vuser1 = tr.endpos - forward * 10; // Pull out a little bit
				bolt->entity.baseline.vuser2 = vBoltAngles; //Look forward!
				bolt->callback = EV_BoltCallback; //So we can set the angles and origin back. (Stick the bolt to the wall)
			}
		}
	}

	gEngfuncs.pEventAPI->EV_PopPMStates();
}

//TODO: Fully predict the fliying bolt.
void EV_FireCrossbow( event_args_t *args )
{
	int idx;
	vec3_t origin;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	
	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/xbow_fire1.wav", 1, ATTN_NORM, 0, 93 + gEngfuncs.pfnRandomLong(0,0xF) );
	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_ITEM, "weapons/xbow_reload1.wav", gEngfuncs.pfnRandomFloat(0.95, 1.0), ATTN_NORM, 0, 93 + gEngfuncs.pfnRandomLong(0,0xF) );

	//Only play the weapon anims if I shot it. 
	if ( EV_IsLocal( idx ) )
	{
		if ( args->iparam1 )
			gEngfuncs.pEventAPI->EV_WeaponAnimation( CROSSBOW_FIRE1, 1 );
		else if ( args->iparam2 )
			gEngfuncs.pEventAPI->EV_WeaponAnimation( CROSSBOW_FIRE3, 1 );

		V_PunchAxis( 0, -2.0 * ( upsideDown ? -1 : 1 ) );
	}
}
//======================
//	   CROSSBOW END 
//======================

//======================
//	    RPG START 
//======================
enum rpg_e {
	RPG_IDLE = 0,
	RPG_FIDGET,
	RPG_RELOAD,		// to reload
	RPG_FIRE2,		// to empty
	RPG_HOLSTER1,	// loaded
	RPG_DRAW1,		// loaded
	RPG_HOLSTER2,	// unloaded
	RPG_DRAW_UL,	// unloaded
	RPG_IDLE_UL,	// unloaded idle
	RPG_FIDGET_UL,	// unloaded fidget
};

void EV_FireRpg( event_args_t *args )
{
	int idx;
	vec3_t origin;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	
	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/rocketfire1.wav", 0.9, ATTN_NORM, 0, PITCH_NORM );
	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_ITEM, "weapons/glauncher.wav", 0.7, ATTN_NORM, 0, PITCH_NORM );

	//Only play the weapon anims if I shot it. 
	if ( EV_IsLocal( idx ) )
	{
		gEngfuncs.pEventAPI->EV_WeaponAnimation( RPG_FIRE2, 1 );
	
		V_PunchAxisAdditive( 0, -2.5 * ( upsideDown ? -1 : 1 ) );
	}
}
//======================
//	     RPG END 
//======================

//======================
//	    EGON END 
//======================
enum egon_e {
	EGON_IDLE1 = 0,
	EGON_FIDGET1,
	EGON_ALTFIREON,
	EGON_ALTFIRECYCLE,
	EGON_ALTFIREOFF,
	EGON_FIRE1,
	EGON_FIRE2,
	EGON_FIRE3,
	EGON_FIRE4,
	EGON_DRAW,
	EGON_HOLSTER
};

int g_fireAnims1[] = { EGON_FIRE1, EGON_FIRE2, EGON_FIRE3, EGON_FIRE4 };
int g_fireAnims2[] = { EGON_ALTFIRECYCLE };

enum EGON_FIRESTATE { FIRE_OFF, FIRE_CHARGE };
enum EGON_FIREMODE { FIRE_NARROW, FIRE_WIDE};

#define	EGON_PRIMARY_VOLUME		450
#define EGON_BEAM_SPRITE		"sprites/xbeam1.spr"
#define EGON_FLARE_SPRITE		"sprites/XSpark1.spr"
#define EGON_SOUND_OFF			"weapons/egon_off1.wav"
#define EGON_SOUND_RUN			"weapons/egon_run3.wav"
#define EGON_SOUND_STARTUP		"weapons/egon_windup2.wav"

#define ARRAYSIZE(p)		(sizeof(p)/sizeof(p[0]))

BEAM *pBeam;
BEAM *pBeam2;

void EV_EgonFire( event_args_t *args )
{
	int idx, iFireState, iFireMode;
	vec3_t origin;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	iFireState = args->iparam1;
	iFireMode = args->iparam2;
	int iStartup = args->bparam1;


	if ( iStartup )
	{
		if ( iFireMode == FIRE_WIDE )
			gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, EGON_SOUND_STARTUP, 0.98, ATTN_NORM, 0, 125 );
		else
			gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, EGON_SOUND_STARTUP, 0.9, ATTN_NORM, 0, 100 );
	}
	else
	{
		gEngfuncs.pEventAPI->EV_StopSound( idx, CHAN_STATIC, EGON_SOUND_RUN );

		if ( iFireMode == FIRE_WIDE )
			gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_STATIC, EGON_SOUND_RUN, 0.98, ATTN_NORM, 0, 125 );
		else
			gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_STATIC, EGON_SOUND_RUN, 0.9, ATTN_NORM, 0, 100 );
	}

	//Only play the weapon anims if I shot it.
	if ( EV_IsLocal( idx ) )
		gEngfuncs.pEventAPI->EV_WeaponAnimation ( g_fireAnims1[ gEngfuncs.pfnRandomLong( 0, 3 ) ], 1 );

	if ( iStartup == 1 && EV_IsLocal( idx ) && !pBeam && !pBeam2 && cl_lw->value ) //Adrian: Added the cl_lw check for those lital people that hate weapon prediction.
	{
		vec3_t vecSrc, vecEnd, origin, angles, forward, right, up;
		pmtrace_t tr;

		cl_entity_t *pl = gEngfuncs.GetEntityByIndex( idx );

		if ( pl )
		{
			VectorCopy( gHUD.m_vecAngles, angles );

			ApplyAimOffset( angles );
			AngleVectors( angles, forward, right, up );

			EV_GetGunPosition( args, vecSrc, pl->origin );

			VectorMA( vecSrc, 2048, forward, vecEnd );

			gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction( false, true );	
				
			// Store off the old count
			gEngfuncs.pEventAPI->EV_PushPMStates();
			
			// Now add in all of the players.
			gEngfuncs.pEventAPI->EV_SetSolidPlayers ( idx - 1 );	

			gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
			gEngfuncs.pEventAPI->EV_PlayerTrace( vecSrc, vecEnd, PM_STUDIO_BOX, -1, &tr );

			gEngfuncs.pEventAPI->EV_PopPMStates();

			int iBeamModelIndex = gEngfuncs.pEventAPI->EV_FindModelIndex( EGON_BEAM_SPRITE );

			float r = 50.0f;
			float g = 50.0f;
			float b = 125.0f;

			if ( IEngineStudio.IsHardware() )
			{
				r /= 100.0f;
				g /= 100.0f;
			}
				
		
			pBeam = gEngfuncs.pEfxAPI->R_BeamEntPoint ( idx | 0x1000, tr.endpos, iBeamModelIndex, 99999, 3.5, 0.2, 0.7, 55, 0, 0, r, g, b );

			if ( pBeam )
				 pBeam->flags |= ( FBEAM_SINENOISE );
 
			pBeam2 = gEngfuncs.pEfxAPI->R_BeamEntPoint ( idx | 0x1000, tr.endpos, iBeamModelIndex, 99999, 5.0, 0.08, 0.7, 25, 0, 0, r, g, b );
		}
	}
}

void EV_EgonStop( event_args_t *args )
{
	int idx;
	vec3_t origin;

	idx = args->entindex;
	VectorCopy ( args->origin, origin );

	gEngfuncs.pEventAPI->EV_StopSound( idx, CHAN_STATIC, EGON_SOUND_RUN );
	
	if ( args->iparam1 )
		 gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, EGON_SOUND_OFF, 0.98, ATTN_NORM, 0, 100 );

	if ( EV_IsLocal( idx ) ) 
	{
		if ( pBeam )
		{
			pBeam->die = 0.0;
			pBeam = NULL;
		}
			
		
		if ( pBeam2 )
		{
			pBeam2->die = 0.0;
			pBeam2 = NULL;
		}
	}
}
//======================
//	    EGON END 
//======================

//======================
//	   HORNET START
//======================
enum hgun_e {
	HGUN_IDLE1 = 0,
	HGUN_FIDGETSWAY,
	HGUN_FIDGETSHAKE,
	HGUN_DOWN,
	HGUN_UP,
	HGUN_SHOOT
};

void EV_HornetGunFire( event_args_t *args )
{
	int idx, iFireMode;
	vec3_t origin, angles, vecSrc, forward, right, up;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	iFireMode = args->iparam1;

	ApplyAimOffset( angles );

	//Only play the weapon anims if I shot it.
	if ( EV_IsLocal( idx ) )
	{
		V_PunchAxis( 0, gEngfuncs.pfnRandomLong ( 0, 2 ) * ( upsideDown ? -1 : 1 ) );
		gEngfuncs.pEventAPI->EV_WeaponAnimation ( HGUN_SHOOT, 1 );
	}

	if ( paynedSoundsMonsters ) {
		switch ( gEngfuncs.pfnRandomLong ( 0, 14 ) ) {
			case 0:	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "payned/pain1.wav", 1, ATTN_NORM, 0, 100 );	break;
			case 1:	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "payned/pain2.wav", 1, ATTN_NORM, 0, 100 );	break;
			case 2:	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "payned/pain3.wav", 1, ATTN_NORM, 0, 100 );	break;
			case 3:	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "payned/pain4.wav", 1, ATTN_NORM, 0, 100 );	break;
			case 4:	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "payned/pain5.wav", 1, ATTN_NORM, 0, 100 );	break;
			case 5:	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "payned/pain6.wav", 1, ATTN_NORM, 0, 100 );	break;
			case 6:	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "payned/pain7.wav", 1, ATTN_NORM, 0, 100 );	break;
			case 7:	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "payned/pain8.wav", 1, ATTN_NORM, 0, 100 );	break;
			case 8:	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "payned/pain9.wav", 1, ATTN_NORM, 0, 100 );	break;
			case 9:	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "payned/die1.wav", 1, ATTN_NORM, 0, 100 );	break;
			case 10:	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "payned/die2.wav", 1, ATTN_NORM, 0, 100 );	break;
			case 11:	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "payned/die3.wav", 1, ATTN_NORM, 0, 100 );	break;
			case 12:	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "payned/die4.wav", 1, ATTN_NORM, 0, 100 );	break;
			case 13:	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "payned/die5.wav", 1, ATTN_NORM, 0, 100 );	break;
			case 14:	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "payned/die6.wav", 1, ATTN_NORM, 0, 100 );	break;
			case 15:	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "payned/die7.wav", 1, ATTN_NORM, 0, 100 );	break;
		}
	} else {
		switch ( gEngfuncs.pfnRandomLong ( 0, 2 ) )
		{
			case 0:	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "agrunt/ag_fire1.wav", 1, ATTN_NORM, 0, 100 );	break;
			case 1:	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "agrunt/ag_fire2.wav", 1, ATTN_NORM, 0, 100 );	break;
			case 2:	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "agrunt/ag_fire3.wav", 1, ATTN_NORM, 0, 100 );	break;
		}
	}
}
//======================
//	   HORNET END
//======================

//======================
//	   TRIPMINE START
//======================
enum tripmine_e {
	TRIPMINE_IDLE1 = 0,
	TRIPMINE_IDLE2,
	TRIPMINE_ARM1,
	TRIPMINE_ARM2,
	TRIPMINE_FIDGET,
	TRIPMINE_HOLSTER,
	TRIPMINE_DRAW,
	TRIPMINE_WORLD,
	TRIPMINE_GROUND,
};

//We only check if it's possible to put a trip mine
//and if it is, then we play the animation. Server still places it.
void EV_TripmineFire( event_args_t *args )
{
	int idx;
	vec3_t vecSrc, angles, view_ofs, forward;
	pmtrace_t tr;

	idx = args->entindex;
	VectorCopy( args->origin, vecSrc );
	VectorCopy( args->angles, angles );

	ApplyAimOffset( angles );

	AngleVectors ( angles, forward, NULL, NULL );
		
	if ( !EV_IsLocal ( idx ) )
		return;

	// Grab predicted result for local player
	gEngfuncs.pEventAPI->EV_LocalPlayerViewheight( view_ofs );

	vecSrc = vecSrc + view_ofs;

	// Store off the old count
	gEngfuncs.pEventAPI->EV_PushPMStates();

	// Now add in all of the players.
	gEngfuncs.pEventAPI->EV_SetSolidPlayers ( idx - 1 );	
	gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
	gEngfuncs.pEventAPI->EV_PlayerTrace( vecSrc, vecSrc + forward * 128, PM_NORMAL, -1, &tr );

	//Hit something solid
	if ( tr.fraction < 1.0 )
		 gEngfuncs.pEventAPI->EV_WeaponAnimation ( TRIPMINE_DRAW, 0 );
	
	gEngfuncs.pEventAPI->EV_PopPMStates();
}
//======================
//	   TRIPMINE END
//======================

//======================
//	   SQUEAK START
//======================
enum squeak_e {
	SQUEAK_IDLE1 = 0,
	SQUEAK_FIDGETFIT,
	SQUEAK_FIDGETNIP,
	SQUEAK_DOWN,
	SQUEAK_UP,
	SQUEAK_THROW
};

#define VEC_HULL_MIN		Vector(-16, -16, -36)
#define VEC_DUCK_HULL_MIN	Vector(-16, -16, -18 )

void EV_SnarkFire( event_args_t *args )
{
	int idx;
	vec3_t vecSrc, angles, view_ofs, forward;
	pmtrace_t tr;

	idx = args->entindex;
	VectorCopy( args->origin, vecSrc );
	VectorCopy( args->angles, angles );

	ApplyAimOffset( angles );

	AngleVectors ( angles, forward, NULL, NULL );
		
	if ( !EV_IsLocal ( idx ) )
		return;
	
	if ( args->ducking )
		vecSrc = vecSrc - ( VEC_HULL_MIN - VEC_DUCK_HULL_MIN );
	
	// Store off the old count
	gEngfuncs.pEventAPI->EV_PushPMStates();

	// Now add in all of the players.
	gEngfuncs.pEventAPI->EV_SetSolidPlayers ( idx - 1 );	
	gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
	gEngfuncs.pEventAPI->EV_PlayerTrace( vecSrc + forward * 20, vecSrc + forward * 64, PM_NORMAL, -1, &tr );

	//Find space to drop the thing.
	if ( tr.allsolid == 0 && tr.startsolid == 0 && tr.fraction > 0.25 )
		 gEngfuncs.pEventAPI->EV_WeaponAnimation ( SQUEAK_THROW, 0 );
	
	gEngfuncs.pEventAPI->EV_PopPMStates();
}
//======================
//	   SQUEAK END
//======================

void EV_TrainPitchAdjust( event_args_t *args )
{
	int idx;
	vec3_t origin;

	unsigned short us_params;
	int noise;
	float m_flVolume;
	int pitch;
	int stop;
	
	char sz[ 256 ];

	idx = args->entindex;
	
	VectorCopy( args->origin, origin );

	us_params = (unsigned short)args->iparam1;
	stop	  = args->bparam1;

	m_flVolume	= (float)(us_params & 0x003f)/40.0;
	noise		= (int)(((us_params) >> 12 ) & 0x0007);
	pitch		= (int)( 10.0 * (float)( ( us_params >> 6 ) & 0x003f ) );

	switch ( noise )
	{
	case 1: strcpy( sz, "plats/ttrain1.wav"); break;
	case 2: strcpy( sz, "plats/ttrain2.wav"); break;
	case 3: strcpy( sz, "plats/ttrain3.wav"); break; 
	case 4: strcpy( sz, "plats/ttrain4.wav"); break;
	case 5: strcpy( sz, "plats/ttrain6.wav"); break;
	case 6: strcpy( sz, "plats/ttrain7.wav"); break;
	default:
		// no sound
		strcpy( sz, "" );
		return;
	}

	if ( stop )
	{
		gEngfuncs.pEventAPI->EV_StopSound( idx, CHAN_STATIC, sz );
	}
	else
	{
		gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_STATIC, sz, m_flVolume, ATTN_NORM, SND_CHANGE_PITCH, pitch );
	}
}

int EV_TFC_IsAllyTeam( int iTeam1, int iTeam2 )
{
	return 0;
}