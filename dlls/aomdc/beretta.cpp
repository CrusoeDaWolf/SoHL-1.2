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

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"

enum beretta_e {
	BERETTA_IDLE1 = 0,
	BERETTA_IDLE2,
	BERETTA_FIRE1,
	BERETTA_FIRE2,
	BERETTA_FIRE_LOAD,
	BERETTA_RELOAD,
	BERETTA_RELOAD_EMPTY,
	BERETTA_DRAW
};

LINK_ENTITY_TO_CLASS( weapon_beretta, CBeretta );

void CBeretta::Spawn( )
{
	Precache( );
	m_iId = WEAPON_BERETTA;
	SET_MODEL(ENT(pev), "models/w_beretta.mdl");

	m_iDefaultAmmo = BERETTA_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CBeretta::Precache( void )
{
	PRECACHE_MODEL("models/v_beretta.mdl");
	PRECACHE_MODEL("models/w_beretta.mdl");
	PRECACHE_MODEL("models/p_9mmhandgun.mdl");

	m_iShell = PRECACHE_MODEL ("models/shell.mdl");// brass shell

	PRECACHE_SOUND("items/9mmclip1.wav");
	PRECACHE_SOUND("items/9mmclip2.wav");

	PRECACHE_SOUND ("weapons/ber_cockback.wav");
	PRECACHE_SOUND ("weapons/ber_cockforward.wav");
	PRECACHE_SOUND ("weapons/ber_fire.wav");
	PRECACHE_SOUND ("weapons/ber_magin.wav");
	PRECACHE_SOUND ("weapons/ber_magout.wav");
	PRECACHE_SOUND ("weapons/ber_magplace.wav");
	PRECACHE_SOUND ("weapons/ber_slideforward.wav");

	m_usFireBeretta = PRECACHE_EVENT( 1, "events/beretta.sc" );
}

int CBeretta::AddToPlayer( CBasePlayer *pPlayer )
{
	if ( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
			WRITE_BYTE( m_iId );
		MESSAGE_END();
		return TRUE;
	}
	return FALSE;
}

int CBeretta::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "9mm";
	p->iMaxAmmo1 = BERETTA_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = BERETTA_MAX_CLIP;
	p->iSlot = 1;
	p->iPosition = 1;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_BERETTA;
	p->iWeight = BERETTA_WEIGHT;

	return 1;
}

BOOL CBeretta::Deploy( )
{
	// pev->body = 1;
	return DefaultDeploy( "models/v_beretta.mdl", "models/p_9mmhandgun.mdl", BERETTA_DRAW, "Beretta", /*UseDecrement() ? 1 : 0*/ 0 );
}

void CBeretta::PrimaryAttack( void )
{
	float flSpread = 0.01;
	float flCycleTime = 0.3;
	BOOL fUseAutoAim = TRUE;

	m_iShotCounter++;

	if( m_iShotCounter > 1 )
		return;

	if (m_iClip <= 0)
	{
		if (m_fFireOnEmpty)
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.2;
		}

		return;
	}

	m_iClip--;

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	int flags;

#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	// non-silenced
	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming;
	
	if ( fUseAutoAim )
	{
		vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );
	}
	else
	{
		vecAiming = gpGlobals->v_forward;
	}

	Vector vecDir;
	vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, Vector( flSpread, flSpread, flSpread ), 8192, BULLET_PLAYER_9MM, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usFireBeretta, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, ( m_iClip == 0 ) ? 1 : 0, 0 );

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + flCycleTime;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}


void CBeretta::Reload( void )
{
	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == BERETTA_MAX_CLIP )
		 return;

	int iResult;

	if (m_iClip == 0)
		iResult = DefaultReload( BERETTA_MAX_CLIP, BERETTA_RELOAD_EMPTY, 1.5 );
	else
		iResult = DefaultReload( BERETTA_MAX_CLIP, BERETTA_RELOAD, 1.5 );

	if (iResult)
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
	}
}



void CBeretta::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	// only idle if the slid isn't back
	if (m_iClip != 0)
	{
		int iAnim;
		float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0.0, 1.0 );

		if (flRand <= 0.6 + 0 * 0.875)
		{
			iAnim = BERETTA_IDLE1;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 60.0 / 16.0;
		}
		else
		{
			iAnim = BERETTA_IDLE2;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 40.0 / 16.0;
		}
		SendWeaponAnim( iAnim, 1 );
	}
}








class CBerettaAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_weaponclips/w_berettaclip.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_weaponclips/w_berettaclip.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		if (pOther->GiveAmmo( AMMO_BERETTACLIP_GIVE, "9mm", BERETTA_MAX_CARRY ) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_beretta, CBerettaAmmo );

