#include "cbase.h"


#include "in_buttons.h"

class CCoopManager : public CBaseEntity
{
	DECLARE_CLASS(CCoopManager, CBaseEntity);
public:
	void	Activate(void);

	void InputMoveSpawn(inputdata_t &inputdata);

	bool IsSpawnSet() { return m_eSpawnSpot?true:false; };

	CBaseEntity* GetSpawnSpot() { return m_eSpawnSpot; };

private:

	CBaseEntity* m_eSpawnSpot = NULL;

	DECLARE_DATADESC();
};