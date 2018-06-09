//Falta probar
#include "ScriptPCH.h"
#include "ScriptMgr.h"
#include "WorldQueue.h"
#include "Player.h"
#include "DatabaseEnv.h"
#include "ObjectMgr.h"
#include "WorldSession.h"
#include "DBCStores.h"
#include "AuxAccountSpell.h"

class AccountSpell : public PlayerScript
{
public:
    AccountSpell() : PlayerScript("AccountSpells") { }

    void DoWork(Player* player)
    {
        // Si no equitacion nada
        if (!player->HasSkill(762) || player->GetSkillValue(762) < 70)
            return;
        // Si es Huargen???????
        if (player->getRace() == 16)
            player->CastSpell(player, 68996, true, NULL, NULL, player->GetGUID());

        AccionesWorldQueueScript tmp;
        tmp.nActionType = TYPE_ACCOUNT_SPELL;

        std::vector<uint32> Spells;

        QueryResult result2 = CharacterDatabase.PQuery("SELECT spell FROM character_spell WHERE guid in (select guid from characters where account = %u) and guid <> %u", player->GetSession()->GetAccountId(), player->GetGUID());
        if (!result2)
            return;

        do
        {
            Spells.push_back(result2->Fetch()[0].GetUInt32());
        } while (result2->NextRow());

        for (auto& i : Spells)
        {
            AccountSpellDataRow Spell_Row = sFiltroSpells->GetRow(i);
            if (Spell_Row.m_SpellType == AccountSpellTypeNONE || !Spell_Row.m_learn) continue;
            if (player->HasSpell(i))continue;

            // Compruebo la raza
            if (Spell_Row.m_maskRace > 0)
            {
                if ((Spell_Row.m_maskRace & player->getRaceMask()) == 0)
                    continue;
            }

            // Compruebo la clase
            if (Spell_Row.m_maskClass > 0)
            {
                if ((Spell_Row.m_maskClass & player->getClassMask()) == 0)
                    continue;
            }

            tmp.nIdSpell = i;
            tmp.nIdCharacter = player->GetGUID();
            sWorldQueueScript->Encolar(tmp);
        }
    }

    void OnLogin(Player* player, bool /*firstLogin*/) override
    {
        DoWork(player);
    }

    void OnSpellLearn(Player* player, uint32 idSpell) override
    {
        // Si el spell es equitacion
        if (idSpell == 33388)
            DoWork(player);
    }
};

class WS_AccountSpellDataTable_Load : public WorldScript
{
public:
    WS_AccountSpellDataTable_Load() : WorldScript("WS_AccountSpellDataTable_Load") {}

    void OnStartup() override
    {
        sFiltroSpells->Load_Account_Spell_Data();
    }
};

void AddSC_AccountSpells()
{
    new AccountSpell;
    new WS_AccountSpellDataTable_Load();
}
