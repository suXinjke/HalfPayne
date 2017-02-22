//========= Copyright � 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#if !defined ( EV_HLDMH )
#define EV_HLDMH

// bullet types
typedef	enum
{
	BULLET_NONE = 0,
	BULLET_PLAYER_9MM, // glock
	BULLET_PLAYER_MP5, // mp5
	BULLET_PLAYER_357, // python
	BULLET_PLAYER_BUCKSHOT, // shotgun
	BULLET_PLAYER_CROWBAR, // crowbar swipe

	BULLET_MONSTER_9MM,
	BULLET_MONSTER_MP5,
	BULLET_MONSTER_12MM,
} Bullet;

enum glock_e {
	GLOCK_IDLE1 = 0,
	GLOCK_IDLE_NOSHOT,
	GLOCK_IDLE3,
	GLOCK_SHOOT,
	GLOCK_SHOOT_EMPTY,
	GLOCK_RELOAD_NOT_EMPTY,
	GLOCK_RELOAD_NOT_EMPTY_FAST,
	GLOCK_RELOAD,
	GLOCK_RELOAD_FAST,

	GLOCK_DRAW,
	GLOCK_DRAW_FAST,

	GLOCK_HOLSTER,

	GLOCK_DRAW_FROM_TWIN,
	GLOCK_DRAW_FROM_TWIN_FAST,

	GLOCK_DRAW_FROM_TWIN_NOSHOT_BOTH,
	GLOCK_DRAW_FROM_TWIN_NOSHOT_BOTH_FAST,

	GLOCK_DRAW_FROM_TWIN_NOSHOT_LEFT,
	GLOCK_DRAW_FROM_TWIN_NOSHOT_LEFT_FAST,

	GLOCK_DRAW_FROM_TWIN_NOSHOT_RIGHT,
	GLOCK_DRAW_FROM_TWIN_NOSHOT_RIGHT_FAST
};

enum glock_twin_e {
	GLOCK_TWIN_IDLE = 0,
	GLOCK_TWIN_IDLE_NOSHOT_BOTH,
	GLOCK_TWIN_IDLE_NOSHOT_LEFT,
	GLOCK_TWIN_IDLE_NOSHOT_RIGHT,

	GLOCK_TWIN_SHOOT_RIGHT,
	GLOCK_TWIN_SHOOT_RIGHT_WHEN_LEFT_EMPTY,
	GLOCK_TWIN_SHOOT_RIGHT_THEN_EMPTY,
	GLOCK_TWIN_SHOOT_RIGHT_THEN_EMPTY_WHEN_LEFT_EMPTY,

	GLOCK_TWIN_SHOOT_LEFT,
	GLOCK_TWIN_SHOOT_LEFT_WHEN_RIGHT_EMPTY,
	GLOCK_TWIN_SHOOT_LEFT_THEN_EMPTY,
	GLOCK_TWIN_SHOOT_LEFT_THEN_EMPTY_WHEN_RIGHT_EMPTY,

	GLOCK_TWIN_RELOAD,
	GLOCK_TWIN_RELOAD_FAST,

	GLOCK_TWIN_RELOAD_NOSHOT_BOTH,
	GLOCK_TWIN_RELOAD_NOSHOT_BOTH_FAST,
	GLOCK_TWIN_RELOAD_NOSHOT_LEFT,
	GLOCK_TWIN_RELOAD_NOSHOT_LEFT_FAST,
	GLOCK_TWIN_RELOAD_NOSHOT_RIGHT,
	GLOCK_TWIN_RELOAD_NOSHOT_RIGHT_FAST,

	GLOCK_TWIN_RELOAD_ONLY_LEFT,
	GLOCK_TWIN_RELOAD_ONLY_LEFT_FAST,
	GLOCK_TWIN_RELOAD_ONLY_LEFT_NOSHOT,
	GLOCK_TWIN_RELOAD_ONLY_LEFT_NOSHOT_FAST,
	GLOCK_TWIN_RELOAD_ONLY_LEFT_NOSHOT_BOTH,
	GLOCK_TWIN_RELOAD_ONLY_LEFT_NOSHOT_BOTH_FAST,

	GLOCK_TWIN_RELOAD_ONLY_RIGHT,
	GLOCK_TWIN_RELOAD_ONLY_RIGHT_FAST,
	GLOCK_TWIN_RELOAD_ONLY_RIGHT_NOSHOT,
	GLOCK_TWIN_RELOAD_ONLY_RIGHT_NOSHOT_FAST,
	GLOCK_TWIN_RELOAD_ONLY_RIGHT_NOSHOT_BOTH,
	GLOCK_TWIN_RELOAD_ONLY_RIGHT_NOSHOT_BOTH_FAST,

	GLOCK_TWIN_DRAW,
	GLOCK_TWIN_DRAW_FAST,
	GLOCK_TWIN_DRAW_NOSHOT_BOTH,
	GLOCK_TWIN_DRAW_NOSHOT_BOTH_FAST,
	GLOCK_TWIN_DRAW_NOSHOT_LEFT,
	GLOCK_TWIN_DRAW_NOSHOT_LEFT_FAST,
	GLOCK_TWIN_DRAW_NOSHOT_RIGHT,
	GLOCK_TWIN_DRAW_NOSHOT_RIGHT_FAST,

	GLOCK_TWIN_DRAW_ONLY_LEFT,
	GLOCK_TWIN_DRAW_ONLY_LEFT_FAST,
	GLOCK_TWIN_DRAW_NOSHOT_BOTH_ONLY_LEFT,
	GLOCK_TWIN_DRAW_NOSHOT_BOTH_ONLY_LEFT_FAST,
	GLOCK_TWIN_DRAW_NOSHOT_LEFT_ONLY_LEFT,
	GLOCK_TWIN_DRAW_NOSHOT_LEFT_ONLY_LEFT_FAST,
	GLOCK_TWIN_DRAW_NOSHOT_RIGHT_ONLY_LEFT,
	GLOCK_TWIN_DRAW_NOSHOT_RIGHT_ONLY_LEFT_FAST
};

enum shotgun_e {
	SHOTGUN_IDLE = 0,
	SHOTGUN_FIRE,
	SHOTGUN_FIRE_FAST,
	SHOTGUN_FIRE2,
	SHOTGUN_FIRE2_FAST,
	SHOTGUN_RELOAD,
	SHOTGUN_PUMP,
	SHOTGUN_PUMP_FAST,
	SHOTGUN_START_RELOAD,
	SHOTGUN_START_RELOAD_FAST,
	SHOTGUN_DRAW,
	SHOTGUN_HOLSTER,
	SHOTGUN_IDLE4,
	SHOTGUN_IDLE_DEEP
};

enum mp5_e
{
	MP5_LONGIDLE = 0,
	MP5_IDLE1,
	MP5_LAUNCH,
	MP5_RELOAD,
	MP5_RELOAD_FAST,
	MP5_DEPLOY,
	MP5_DEPLOY2,
	MP5_FIRE1,
	MP5_FIRE2,
	MP5_FIRE3,
};

enum python_e {
	PYTHON_IDLE1 = 0,
	PYTHON_IDLE_NOSHOT,
	PYTHON_FIDGET,
	PYTHON_FIRE1,
	PYTHON_FIRE1_NOSHOT,

	PYTHON_RELOAD,
	PYTHON_RELOAD_NOSHOT_FAST,

	PYTHON_HOLSTER,

	PYTHON_DRAW,
	PYTHON_DRAW_FAST,

	PYTHON_DRAW_NOSHOT,
	PYTHON_DRAW_NOSHOT_FAST
};


#define	GAUSS_PRIMARY_CHARGE_VOLUME	256// how loud gauss is while charging
#define GAUSS_PRIMARY_FIRE_VOLUME	450// how loud gauss is when discharged

enum gauss_e {
	GAUSS_IDLE = 0,
	GAUSS_IDLE2,
	GAUSS_FIDGET,
	GAUSS_SPINUP,
	GAUSS_SPIN,
	GAUSS_FIRE,
	GAUSS_FIRE2,
	GAUSS_HOLSTER,
	GAUSS_DRAW
};

void EV_HLDM_GunshotDecalTrace( pmtrace_t *pTrace, char *decalName );
void EV_HLDM_DecalGunshot( pmtrace_t *pTrace, int iBulletType );
int EV_HLDM_CheckTracer( int idx, float *vecSrc, float *end, float *forward, float *right, int iBulletType, int iTracerFreq, int *tracerCount );
void EV_HLDM_FireBullets( int idx, float *forward, float *right, float *up, int cShots, float *vecSrc, float *vecDirShooting, float flDistance, int iBulletType, int iTracerFreq, int *tracerCount, float flSpreadX, float flSpreadY );

#endif // EV_HLDMH