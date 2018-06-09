#include "ScriptPCH.h"
#include "Define.h"
#include "Language.h"
#include "Chat.h"
#include "Guild.h"
#include "GuildMgr.h"
#include "InstanceSaveMgr.h"
#include "Pet.h"
#include "DatabaseEnv.h"
#include "ObjectMgr.h"
#include "SpellMgr.h"
#include "ScriptedGossip.h"
#include "GossipDef.h"
#include "Item.h"
#include "Player.h"
#include "WorldSession.h"
#include "World.h"
#include "../Auxiliares/Promo/Promo.h"



bool CreatePet(Player *player, uint32 entry)
{
    ChatHandler chat(player->GetSession());
    if (player->getClass() != CLASS_HUNTER) {
        chat.SendSysMessage("No eres un cazador!");
        return false;
    }

    if (player->GetPet())
    {
        chat.SendSysMessage("Ya controlas una Mascota invocada");
        return false;
    }

    Creature *creatureTarget = player->SummonCreature(entry, player->GetPositionX(), player->GetPositionY() + 2, player->GetPositionZ(), player->GetOrientation(), TEMPSUMMON_CORPSE_TIMED_DESPAWN, 500);
    if (!creatureTarget) return false;

    Pet* pet = player->CreateTamedPetFrom(creatureTarget, 0);
    if (!pet) return false;

    // kill original creature
    creatureTarget->setDeathState(JUST_DIED);
    creatureTarget->RemoveCorpse();
    creatureTarget->SetHealth(0);                       // just for nice GM-mode view

    pet->SetUInt64Value(UNIT_FIELD_CREATEDBY, player->GetGUID());
    pet->SetUInt32Value(UNIT_FIELD_FACTIONTEMPLATE, player->GetTeam());

    // prepare visual effect for levelup
    pet->SetUInt32Value(UNIT_FIELD_LEVEL, player->getLevel() - 1);
    pet->GetMap()->AddToMap(pet->ToCreature());
    // visual effect for levelup
    pet->SetUInt32Value(UNIT_FIELD_LEVEL, player->getLevel());

    pet->GetCharmInfo()->SetPetNumber(sObjectMgr->GeneratePetNumber(), true);
    if (!pet->InitStatsForLevel(player->getLevel()))

        pet->UpdateAllStats();

    // caster have pet now
    player->SetMinion(pet, true);

    pet->SavePetToDB(PET_SAVE_AS_CURRENT);
    pet->InitTalentForLevel();
    player->PetSpellInitialize();

    //end
    chat.SendSysMessage("Mascota añadida!");
    return true;
}

enum Accion
{
    ApplyAura = 0,
    CastSpell = 1,
    AddItemsTmorg = 2
};
void DoAction(Player* player, std::string const& data, Accion accion)
{
    uint32 spellid = 0;
    Tokenizer tokens(data, ' ');

    for (Tokenizer::const_iterator iter = tokens.begin(); iter != tokens.end(); ++iter)
    {
        uint32 id = atoi(*iter);
        if (accion != AddItemsTmorg)
        {
            if (sSpellMgr->GetSpellInfo(id))
            {
                switch (accion)
                {
                case ApplyAura:
                    player->AddAura(id, player);
                    break;
                case CastSpell:
                    player->CastSpell(player, id, true);
                    break;
                default:
                    break;
                }
            }
        }
        else if (accion == AddItemsTmorg)
        {
            const ItemTemplate* proto = sObjectMgr->GetItemTemplate(id);
            if (proto)
            {
                WorldDatabase.PExecute("Replace into items_obtenidos (cuenta,entry,displayID) value (%d,%d,%d)", player->GetSession()->GetAccountId(), proto->ItemId, proto->DisplayInfoID);
            }
        }
    }

}

static std::string GetTimeString(uint64 time)
{
    uint64 days = time / DAY, hours = (time % DAY) / HOUR, minute = (time % HOUR) / MINUTE;
    std::ostringstream ss;
    if (days)
        ss << days << "d ";
    if (hours)
        ss << hours << "h ";
    ss << minute << 'm';
    return ss.str();
}

bool UnBindInstance(Player* player, uint16 MapId)
{
    if (player->GetInstanceId())
    {
        //esta en una instancia
        ChatHandler chat(player->GetSession());
        chat.SendSysMessage("No puedes usar eso dentro de una instancia");
        return false;
    }

    int8 diff = -1;
    uint16 counter = 0;

    ChatHandler handler(player->GetSession());

    for (uint8 i = 0; i < MAX_DIFFICULTY; ++i)
    {
        Player::BoundInstancesMap &binds = player->GetBoundInstances(Difficulty(i));
        for (Player::BoundInstancesMap::iterator itr = binds.begin(); itr != binds.end();)
        {
            InstanceSave* save = itr->second.save;
            if (itr->first != player->GetMapId() && (!MapId || MapId == itr->first))
            {
                player->UnbindInstance(itr, Difficulty(i));
                counter++;
            }
            else
                ++itr;
        }
    }
    handler.PSendSysMessage(LANG_COMMAND_INST_UNBIND_UNBOUND, counter);

    return true;
}

class item_script_custom_generic : public ItemScript
{
public:
    item_script_custom_generic() : ItemScript("item_script_custom_generic") { };

    bool OnUse(Player* player, Item* item, SpellCastTargets const& /*targets*/) override
    {
        //Los items que abren gossip los destruye el final del gossip
        bool destroy = true;
        const ItemTemplate*  itemproto = item->GetTemplate();
        if (itemproto->ScriptType == 1) //Subir Level
        {
            uint32 nCantidad = atoi(itemproto->ScriptIDs.c_str());
            if (!nCantidad) return true;

            if (player->getLevel() >= 80)
            {
                ChatHandler chat(player->GetSession());
                chat.SendSysMessage("Ya eres nivel 80");
                return true;
            }
            uint32 nextLevel = player->getLevel() + (nCantidad);
            if (nextLevel > player->getLevel())
            {
                if (nextLevel > 80) nextLevel = 80;
                player->GiveLevel(nextLevel);
            }

        }
        else if (itemproto->ScriptType == 2) //ChangeFaction
        {
            PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_ADD_AT_LOGIN_FLAG);
            stmt->setUInt16(0, uint16(AT_LOGIN_CHANGE_FACTION));
            ChatHandler chat(player->GetSession());
            chat.SendSysMessage("Paquete activado, Ya tienes la opcion de cambiar de faccion en la lista de personajes de tu cuenta");
            player->SetAtLoginFlag(AT_LOGIN_CHANGE_FACTION);
            stmt->setUInt32(1, player->GetGUID().GetCounter());
            CharacterDatabase.Execute(stmt);
        }
        else if (itemproto->ScriptType == 3) //ChangeRace
        {
            PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_ADD_AT_LOGIN_FLAG);
            stmt->setUInt16(0, uint16(AT_LOGIN_CHANGE_RACE));
            ChatHandler chat(player->GetSession());
            chat.SendSysMessage("Paquete activado, Ya tienes la opcion de cambiar de raza en la lista de personajes de tu cuenta");
            player->SetAtLoginFlag(AT_LOGIN_CHANGE_RACE);
            stmt->setUInt32(1, player->GetGUID().GetCounter());

            CharacterDatabase.Execute(stmt);
        }
        else if (itemproto->ScriptType == 4) //Customization
        {
            PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_ADD_AT_LOGIN_FLAG);
            stmt->setUInt16(0, uint16(AT_LOGIN_CUSTOMIZE));
            ChatHandler chat(player->GetSession());
            chat.SendSysMessage("Paquete activado, Ya tienes la opcion de cambiar de apariencia en la lista de personajes de tu cuenta");
            player->SetAtLoginFlag(AT_LOGIN_CUSTOMIZE);
            stmt->setUInt32(1, player->GetGUID().GetCounter());
            CharacterDatabase.Execute(stmt);
        }
        else if (itemproto->ScriptType == 5) //Pet
        {
            uint32 entryPet = atoi(itemproto->ScriptIDs.c_str());
            if (!entryPet) return true;
            if (!CreatePet(player, entryPet))
                return true;
        }
        else if (itemproto->ScriptType == 6) //CastSpell
        {
            DoAction(player, itemproto->ScriptIDs.c_str(), CastSpell);
        }
        else if (itemproto->ScriptType == 7) //ApplyAura
        {
            DoAction(player, itemproto->ScriptIDs.c_str(), ApplyAura);
        }
        else if (itemproto->ScriptType == 8) //Transfiguracion
        {
            DoAction(player, itemproto->ScriptIDs.c_str(), AddItemsTmorg);
            player->GetSession()->SendAreaTriggerMessage("Agregado a tu lista de Transfiguraciones.");
        }
        else if (itemproto->ScriptType == 9) //SetLevel
        {
            uint32 nCantidad = atoi(itemproto->ScriptIDs.c_str());
            if (!nCantidad) return true;

            if (player->getLevel() == nCantidad)
            {
                ChatHandler chat(player->GetSession());
                std::string msg = "Ya eres nivel " + std::to_string(nCantidad);
                chat.SendSysMessage(msg.c_str());
                return true;
            }
            else if (player->getLevel() > nCantidad)
            {
                ChatHandler chat(player->GetSession());
                std::string msg = "Tu nivel ya es mayor que " + std::to_string(nCantidad);
                chat.SendSysMessage(msg.c_str());
                return true;
            }
            uint32 nextLevel = nCantidad;
            if (nextLevel > 80) nextLevel = 80;
            player->GiveLevel(nextLevel);

        }
        else if (itemproto->ScriptType == 10) //Guild Rename
        {
            Guild* guild = player->GetGuild();
            if (!guild)
            {
                ChatHandler chat(player->GetSession());
                std::string msg = "Debes pertenecer a una hermandad";
                chat.SendSysMessage(msg.c_str());
                return true;
            }

            if (player->GetGUID() != guild->GetLeaderGUID())
            {
                ChatHandler chat(player->GetSession());
                std::string msg = "Para poder cambiar el nombre de la hermandad, debes ser el lider";
                chat.SendSysMessage(msg.c_str());
                return true;
            }
            ClearGossipMenuFor(player);
            CloseGossipMenuFor(player);
            AddGossipItemFor(player, 3, "GuildRename", 0, 0, "Introduce el nuevo nombre para la hermandad", 0, true);
            SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, item->GetGUID());
            destroy = false;
            player->SetCurrentItemGossip(GUILD_RENAME);
        }
        else if (itemproto->ScriptType == 11) //UnBan
        {
            ClearGossipMenuFor(player);
            CloseGossipMenuFor(player);
            AddGossipItemFor(player, 3, "unban", 0, 0, "Introduce el nombre del jugador a desbanear", 0, true);
            SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, item->GetGUID());
            destroy = false;
            player->SetCurrentItemGossip(PLAYER_UNBAN);
        }
        else if (itemproto->ScriptType == 12) //Unbind
        {
            destroy = UnBindInstance(player, atoi(itemproto->ScriptIDs.c_str()));
        }
        else if (itemproto->ScriptType == 13) //Add LoyaltyPoints
        {
            player->GetSession()->AddVotePoints(atoi(itemproto->ScriptIDs.c_str()), false);
        }
        else if (itemproto->ScriptType == 14) //PromocionIndividual
        {
            player->SetCurrentItemGossip(PLAYER_BOOST);
            Promocion::GossipHello(player, item->GetGUID(), item->GetEntry());
            destroy = false;
        }
        else if (itemproto->ScriptType == 15) //Extender Promo Guild
        {
            player->SetCurrentItemGossip(GUILD_PROMO_EXTEND);
            Promocion::GossipHello(player, item->GetGUID(), item->GetEntry());
            destroy = false;
        }
        if (destroy)
        {
            player->DestroyItemCount(itemproto->ItemId, 1, true, false);
            player->CastSpell(player, 29541);
        }
        return true;
    }

    void GossipSelectCode(Player* player, Item* item, uint32 /*menuId*/, uint32 gossipListId, const char* code) override
    {
        uint32 action = player->PlayerTalkClass->GetGossipOptionAction(gossipListId);
        const ItemTemplate*  itemproto = item->GetTemplate();
        std::string strOldName = "";
        std::string strNewName = "";
        Guild* guild;
        ChatHandler chat(player->GetSession());

        if (player->IsCurrentItemGossip(GUILD_RENAME))
        {
            guild = player->GetGuild();
            strOldName = guild->GetName();
            strNewName = code;

            if (sGuildMgr->GetGuildByName(strNewName))
            {
                chat.PSendSysMessage(LANG_GUILD_RENAME_ALREADY_EXISTS, strNewName);
                chat.SetSentErrorMessage(true);
                return;
            }

            if (!guild->SetName(strNewName))
            {
                chat.SendSysMessage(LANG_BAD_VALUE);
                chat.SetSentErrorMessage(true);
                return;
            }
            chat.SendGlobalSysMessage(("La hermandad '" + strOldName + "' ha cambiado su nombre por '" + strNewName + "'").c_str());
        }
        else if (player->IsCurrentItemGossip(PLAYER_UNBAN))
        {
            std::string name = code;

            if (!normalizePlayerName(name))
            {
                chat.SendSysMessage(LANG_PLAYER_NOT_FOUND);
                chat.SetSentErrorMessage(true);
                return;
            }

            if (!sWorld->RemoveBanCharacter(name))
            {
                chat.SendSysMessage(LANG_PLAYER_NOT_FOUND);
                chat.SetSentErrorMessage(true);
                return;
            }
            chat.SendSysMessage("Personaje desbaneado");
        }

        player->DestroyItemCount(itemproto->ItemId, 1, true, false);
        player->CastSpell(player, 29541);
        return;

    }

    void GossipSelect(Player* player, Item* item, uint32 menuId, uint32 gossipListId) override
    {
        if (player->IsCurrentItemGossip(PLAYER_BOOST) || player->IsCurrentItemGossip(GUILD_PROMO_EXTEND))
            Promocion::GossipSelect(player, menuId, gossipListId, item->GetGUID());

    }

};

void AddSC_item_customization()
{
    new item_script_custom_generic();
}
