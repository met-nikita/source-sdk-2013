//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef GRENADE_HOPWIRE_H
#define GRENADE_HOPWIRE_H
#ifdef _WIN32
#pragma once
#endif

#include "basegrenade_shared.h"
#include "Sprite.h"

extern ConVar hopwire_trap;

class CGravityVortexController : public CBaseEntity
{
	DECLARE_CLASS( CGravityVortexController, CBaseEntity );
	DECLARE_DATADESC();

public:

	CGravityVortexController( void ) : m_flEndTime( 0.0f ), m_flRadius( 256 ), m_flStrength( 256 ), m_flMass( 0.0f ) {}
	float	GetConsumedMass( void ) const;

#ifdef EZ2
	static CGravityVortexController *Create( const Vector &origin, float radius, float strength, float duration, CBaseEntity *pGrenade = NULL );

	void	StartSpawning();
	void	SpawnThink();
	bool	TryCreateRecipeNPC( const char *szClass, const char *szKV );
	void	ParseKeyValues( CBaseEntity *pEntity, const char *szKV );

	void	SetThrower( CBaseCombatCharacter *pBCC ) { m_hThrower.Set( pBCC ); }
	CBaseCombatCharacter *GetThrower() { return m_hThrower.Get(); }
#else
	static CGravityVortexController *Create( const Vector &origin, float radius, float strength, float duration );
#endif

private:

	void	ConsumeEntity( CBaseEntity *pEnt );
	void	PullPlayersInRange( void );
	bool	KillNPCInRange( CBaseEntity *pVictim, IPhysicsObject **pPhysObj );
	void	CreateDenseBall( void );
#ifdef EZ2
	int		Restore( IRestore &restore );
	void	CreateXenLife();

	void	CreateOldXenLife( void ); // 1upD - Create headcrab or bullsquid 
	bool	TryCreateNPC( const char *className ); // 1upD - Try to spawn an NPC
	bool	TryCreateBaby( const char *className ); // 1upD - Try to spawn an NPC
	bool	TryCreateBird( const char *className ); // 1upD - Try to spawn an NPC
	bool	TryCreateComplexNPC( const char *className, bool isBaby, bool isBird ); // 1upD - Try to spawn an NPC
#endif
	void	PullThink( void );
	void	StartPull( const Vector &origin, float radius, float strength, float duration );

	float	m_flMass;		// Mass consumed by the vortex
	float	m_flEndTime;	// Time when the vortex will stop functioning
	float	m_flRadius;		// Area of effect for the vortex
	float	m_flStrength;	// Pulling strength of the vortex

#ifdef EZ2
							// If this points to an entity, the Xen grenade will always call g_interactionXenGrenadeRelease on it instead of spawning Xen life.
							// This is so Will-E pops back out of Xen grenades.
	CHandle<CBaseCombatCharacter>	m_hReleaseEntity;

	CHandle<CBaseCombatCharacter>	m_hThrower;

	// Stuff gathered for recipes
	CUtlMap<string_t, float, short> m_ModelMass;
	CUtlMap<string_t, float, short> m_ClassMass;
	CUtlMap<Vector, int, short> m_HullMap;

public:
	CUtlMap<string_t, string_t> m_SpawnList;
	unsigned int m_iCurSpawned;

	int m_iSuckedProps;
	int m_iSuckedNPCs;
#endif
};

class CGrenadeHopwire : public CBaseGrenade
{
	DECLARE_CLASS( CGrenadeHopwire, CBaseGrenade );
	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();

public:
	void	Spawn( void );
	void	Precache( void );
	bool	CreateVPhysics( void );
	void	SetTimer( float timer );
	void	SetVelocity( const Vector &velocity, const AngularImpulse &angVelocity );
	void	Detonate( void );

#ifdef EZ2
	void	DelayThink();
	void	SpriteOff();
	void	BlipSound() { EmitSound( "WeaponXenGrenade.Blip" ); }
	void	OnRestore( void );
	void	CreateEffects( void );
#endif
	
	void	EndThink( void );		// Last think before going away
	void	CombatThink( void );	// Makes the main explosion go off

	void SetWorldModelClosed(const char * modelName) { Q_strncpy(szWorldModelClosed, modelName, MAX_WEAPON_STRING); }
	void SetWorldModelOpen(const char * modelName) { Q_strncpy(szWorldModelOpen, modelName, MAX_WEAPON_STRING); }

protected:

	void	KillStriders( void );

	char	szWorldModelClosed[MAX_WEAPON_STRING]; // "models/roller.mdl"
	char	szWorldModelOpen[MAX_WEAPON_STRING]; // "models/roller_spikes.mdl"

	CHandle<CGravityVortexController>	m_hVortexController;

#ifdef EZ2
	CHandle<CSprite>		m_pMainGlow;

	float	m_flNextBlipTime;
#endif
};

extern CBaseGrenade *HopWire_Create( const Vector &position, const QAngle &angles, const Vector &velocity, const AngularImpulse &angVelocity, CBaseEntity *pOwner, float timer, const char * modelClosed, const char * modelOpen );

#endif // GRENADE_HOPWIRE_H
