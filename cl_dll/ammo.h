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

#ifndef __AMMO_H__
#define __AMMO_H__

#define MAX_WEAPON_NAME 128


#define WEAPON_FLAGS_SELECTONEMPTY	1

#define WEAPON_IS_ONTARGET 0x40

struct WEAPON
{
	char	szName[MAX_WEAPON_NAME];
	int		iAmmoType;
	int		iAmmo2Type;
	int		iMax1;
	int		iMax2;
	int		iSlot;
	int		iSlotPos;
	int		iFlags;
	int		iId;
	int		iClip;
	int		iClip2;

	int		iCount;		// # of itesm in plist

	SPRITE_HANDLE hActive;
	wrect_t rcActive;
	SPRITE_HANDLE hInactive;
	wrect_t rcInactive;
	SPRITE_HANDLE	hAmmo;
	wrect_t rcAmmo;
	SPRITE_HANDLE hAmmo2;
	wrect_t rcAmmo2;
	SPRITE_HANDLE hCrosshair;
	wrect_t rcCrosshair;
	SPRITE_HANDLE hAutoaim;
	wrect_t rcAutoaim;
	SPRITE_HANDLE hZoomedCrosshair;
	wrect_t rcZoomedCrosshair;
	SPRITE_HANDLE hZoomedAutoaim;
	wrect_t rcZoomedAutoaim;
};

typedef int AMMO;


#endif