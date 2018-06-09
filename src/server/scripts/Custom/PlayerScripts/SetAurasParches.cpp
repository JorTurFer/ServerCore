#include "ScriptPCH.h"
#include "World.h"
#include "Group.h"
#include "DBCStores.h"
#include "SpellAuras.h"
#include "SpellMgr.h"
#include "WorldSession.h"

class SetAurasParches : public PlayerScript
{
public:
    SetAurasParches() : PlayerScript("SetAurasParches") {}

    void OnLogin(Player* player, bool /*bFirstLogin*/)
    {
		//Cambio las auras
		if (player->GetSession()->HasPatch())
		{
			player->RemoveAura(300612);
			player->AddAura(300611, player);
		}
		else
		{
			player->RemoveAura(300611);
			player->AddAura(300612, player);
		}
        //Hago el recall       
        player->TeleportTo(player->GetWorldLocation());
    }
};

void AddSC_SetAurasParches()
{
    new SetAurasParches;
}
