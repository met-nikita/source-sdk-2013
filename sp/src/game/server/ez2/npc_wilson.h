//================= Copyright LOLOLOLOL, All rights reserved. ==================//
//
// Purpose: Wilson header
// 
// Author: Blixibon
//
//=============================================================================//

#include "npc_turret_floor.h"
#include "ai_speech.h"
#include "ai_playerally.h"
#include "ai_baseactor.h"
#include "ai_sensorydummy.h"

#include "sprite.h"
#include "filters.h"

//-----------------------------------------------------------------------------
#define TLK_WILLE_TIPPED "TLK_TIPPED"
#define TLK_WILLE_SEE_ENEMY TLK_STARTCOMBAT
#define TLK_WILLE_DAMAGE TLK_WOUND
#define TLK_WILLE_FIDGET "TLK_FIDGET"
#define TLK_WILLE_PLDEAD TLK_PLDEAD
#define TLK_WILLE_PLHURT TLK_PLHURT
#define TLK_WILLE_PLRELOAD TLK_PLRELOAD

// These are triggered by the maps right now
#define TLK_WILLE_BEAST_DANGER "TLK_BEAST_DANGER"
//-----------------------------------------------------------------------------

// This was CAI_ExpresserHost<CNPCBaseInteractive<CAI_BaseActor>>, but then I remembered we don't need CAI_ExpresserHost because CAI_BaseActor already has it.
// This was then just CNPCBaseInteractive<CAI_BaseActor>, but then I remembered CNPCBaseInteractive is just the Alyx-hackable stuff, which Wilson doesn't need.
// Then this was just CAI_BaseActor, but then I realized CAI_PlayerAlly has a lot of code Wilson could use.
// Then this was just CAI_PlayerAlly, but then I started using CAI_SensingDummy for some Bad Cop code and it uses things I used for Will-E.
// So now it's CAI_SensingDummy<CAI_PlayerAlly>.
typedef CAI_SensingDummy<CAI_PlayerAlly> CAI_WilsonBase;

//-----------------------------------------------------------------------------
// Purpose: Wilson, Willie, Will-E
// 
// NPC created by Blixibon.
// 
// Will-E is a floor turret with no desire for combat and the ability to use the Response System.
// 
//-----------------------------------------------------------------------------
class CNPC_Wilson : public CAI_WilsonBase, public CDefaultPlayerPickupVPhysics, public ITippableTurret
{
	DECLARE_CLASS(CNPC_Wilson, CAI_WilsonBase);
	DECLARE_DATADESC();
public:
	CNPC_Wilson();
	~CNPC_Wilson();

	static CNPC_Wilson *GetWilson( void );
	CNPC_Wilson *m_pNext;

	void	Precache();
	void	Spawn();
	void	Activate( void );
	bool	CreateVPhysics( void );
	void	UpdateOnRemove( void );
	float	MaxYawSpeed( void ){ return 0; }

	void	Touch(	CBaseEntity *pOther );
	void	SimpleUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
	{
		CBasePlayer *pPlayer = ToBasePlayer( pActivator );
		if ( pPlayer && !m_bStatic )
		{
			pPlayer->PickupObject( this, false );
		}
	}

	void	PlayerPenetratingVPhysics( void ) {}
	bool	CanBecomeServerRagdoll( void ) { return false; }

	// Don't waste CPU doing sensing code.
	// We now need hearing for state changes and stuff, but sight isn't necessary at the moment.
	int		GetSensingFlags( void ) { return SENSING_FLAGS_DONT_LOOK /*| SENSING_FLAGS_DONT_LISTEN*/; }

	int		OnTakeDamage( const CTakeDamageInfo &info );
	void	Event_Killed( const CTakeDamageInfo &info );

	// We use our own regen code
	bool	ShouldRegenerateHealth( void ) { return false; }

	// Wilson should only hear combat-related sounds, ironically.
	int		GetSoundInterests( void ) { return SOUND_COMBAT | SOUND_DANGER | SOUND_BULLET_IMPACT; }

	void			NPCThink();
	void			PrescheduleThink( void );
	void			GatherConditions( void );
	void			GatherEnemyConditions( CBaseEntity *pEnemy );

	// Wilson hardly cares about his NPC state because he's just a vessel for choreography and player attachment, not a useful combat ally.
	// This means we redirect SelectIdleSpeech() and SelectAlertSpeech() to the same function. Sure, we could've marked SelectNonCombatSpeech() virtual
	// and overrode that instead, but this makes things simpler to understand and avoids changing all allies just for Will-E to say this nonsense.
	bool			DoCustomSpeechAI( AISpeechSelection_t *pSelection, int iState );
	bool			SelectIdleSpeech( AISpeechSelection_t *pSelection ) { return DoCustomSpeechAI(pSelection, NPC_STATE_IDLE); }
	bool			SelectAlertSpeech( AISpeechSelection_t *pSelection ) { return DoCustomSpeechAI(pSelection, NPC_STATE_ALERT); }

	void			InputSelfDestruct( inputdata_t &inputdata );

	virtual bool	HasPreferredCarryAnglesForPlayer( CBasePlayer *pPlayer );
	virtual QAngle	PreferredCarryAngles( void );

	virtual void	OnPhysGunPickup( CBasePlayer *pPhysGunUser, PhysGunPickup_t reason );
	virtual void	OnPhysGunDrop( CBasePlayer *pPhysGunUser, PhysGunDrop_t reason );
	virtual bool	OnAttemptPhysGunPickup( CBasePlayer *pPhysGunUser, PhysGunPickup_t reason );

	bool			IsBeingCarriedByPlayer( void ) { return m_bCarriedByPlayer; }
	bool			WasJustDroppedByPlayer( void ) { return m_flPlayerDropTime > gpGlobals->curtime; }
	bool			IsHeldByPhyscannon( )	{ return VPhysicsGetObject() && (VPhysicsGetObject()->GetGameFlags() & FVPHYSICS_PLAYER_HELD); }

	bool			OnSide( void );

	bool	HandleInteraction( int interactionType, void *data, CBaseCombatCharacter *sourceEnt );

	bool	PreThink( turretState_e state );
	void	SetEyeState( eyeState_t state );

	virtual void    ModifyOrAppendCriteria(AI_CriteriaSet& criteriaSet);
	void			ModifyOrAppendDamageCriteria(AI_CriteriaSet & set, const CTakeDamageInfo & info);
	virtual bool    SpeakIfAllowed(AIConcept_t concept, char *pszOutResponseChosen = NULL, size_t bufsize = 0, IRecipientFilter *filter = NULL, bool bCanInterrupt = false);
	virtual bool    SpeakIfAllowed(AIConcept_t concept, AI_CriteriaSet& modifiers, char *pszOutResponseChosen = NULL, size_t bufsize = 0, IRecipientFilter *filter = NULL, bool bCanInterrupt = false);
	void			PostSpeakDispatchResponse( AIConcept_t concept, AI_Response *response );
	virtual void    PostConstructor(const char *szClassname);
	virtual CAI_Expresser *CreateExpresser(void);
	virtual CAI_Expresser *GetExpresser() { return m_pExpresser;  }

	void			InputAnswerQuestion( inputdata_t &inputdata );

	int		BloodColor( void ) { return DONT_BLEED; }

	// Will-E doesn't attack anyone and nobody attacks him.
	// You can make NPCs attack him with ai_relationship, but Will-E is literally incapable of combat.
	Class_T		Classify( void ) { return CLASS_NONE; }

protected:

	COutputEvent m_OnTipped;
	COutputEvent m_OnPlayerUse;
	COutputEvent m_OnPhysGunPickup;
	COutputEvent m_OnPhysGunDrop;
	COutputEvent m_OnDestroyed;

	int						m_iEyeAttachment;
	eyeState_t				m_iEyeState;
	CHandle<CSprite>		m_hEyeGlow;
	CHandle<CBeam>			m_hLaser;
	CHandle<CTurretTipController>	m_pMotionController;

	bool	m_bBlinkState;
	bool	m_bTipped;
	bool	m_bCarriedByPlayer;
	bool	m_bUseCarryAngles;
	float	m_flPlayerDropTime;

	float		m_fNextFidgetSpeechTime;

	// For when Wilson is meant to be non-interactive (e.g. background maps, dev commentary)
	// This makes Wilson unmovable and deactivates/doesn't precache a few miscellaneous things.
	bool	m_bStatic;

	CAI_Expresser *m_pExpresser;

	DEFINE_CUSTOM_AI;
};

//-----------------------------------------------------------------------------
// Purpose: Arbeit scanner that Wilson uses.
//-----------------------------------------------------------------------------
class CArbeitScanner : public CBaseAnimating
{
	DECLARE_CLASS(CArbeitScanner, CBaseAnimating);
	DECLARE_DATADESC();
public:
	CArbeitScanner();
	~CArbeitScanner();

	void	Precache();
	void	Spawn();

	// I doubt we'll need anything more robust in the future
	inline bool	IsScannable( CAI_BaseNPC *pNPC ) { return pNPC->ClassMatches(m_target); }

	// Checks if they're in front and then tests visibility
	inline bool	IsInScannablePosition( CAI_BaseNPC *pNPC, Vector &vecToNPC, Vector &vecForward ) { return DotProduct( vecToNPC, vecForward ) < -0.10f && FVisible( pNPC, MASK_SOLID_BRUSHONLY ); }

	// Just hijack the damage filter
	inline bool	CanPassScan( CAI_BaseNPC *pNPC ) { return m_hDamageFilter ? static_cast<CBaseFilter*>(m_hDamageFilter.Get())->PassesFilter(this, pNPC) : true; }

	CAI_BaseNPC *FindMatchingNPC( float flRadiusSqr );

	// For now, just use our skin
	inline void	SetScanState( int iState ) { m_nSkin = iState; }
	inline int	GetScanState() { return m_nSkin; }

	void	IdleThink();
	void	AwaitScanThink();
	void	WaitForReturnThink();

	void	ScanThink();
	void	CleanupScan();

	void	InputEnable( inputdata_t &inputdata );
	void	InputDisable( inputdata_t &inputdata );

	enum
	{
		SCAN_IDLE,		// Default
		SCAN_WAITING,	// Spotted NPC, waiting for them to get close
		SCAN_SCANNING,	// Scan in progress
		SCAN_DONE,		// Scanning done, displays a checkmark or something
		SCAN_REJECT,	// Scanning rejected, displays an X or something
	};

	COutputEvent m_OnScanDone;
	COutputEvent m_OnScanReject;

	COutputEvent m_OnScanStart;
	COutputEvent m_OnScanInterrupt;

private:

	bool m_bDisabled;

	// -1 = Don't cool down
	float m_flCooldown;

	float m_flOuterRadius;
	float m_flInnerRadius;

	string_t m_iszScanSound;
	string_t m_iszScanDeploySound;
	string_t m_iszScanDoneSound;
	string_t m_iszScanRejectSound;

	// How long to wait
	float m_flScanTime;

	// Actual curtime
	float m_flScanEndTime;

	AIHANDLE	m_hScanning;
	CSprite		*m_pSprite;

	int m_iScanAttachment;

};
