#include "ScriptPCH.h"
#include "World.h"
#include "Group.h"
#include "Player.h"
#include "DBCStores.h"
#include "World.h"

class Boss_Announcer : public PlayerScript
{
public:
    Boss_Announcer() : PlayerScript("Boss_Announcer") {}

    void OnCreatureKill(Player* player, Creature* boss)
    {
        if (!player || !boss)
            return;

        if (boss->GetEntry() == 18847 || boss->GetEntry() == 18829 || boss->GetEntry() == 17256 || boss->GetOriginalEntry() == 36633)
            return;

        Group* grupo = player->GetGroup();
        if (grupo)
        {
            //Si el boss no es de mazmorra no mostramos
            if ((!boss->IsDungeonBoss() && !boss->isWorldBoss()))
                return;
            //Solo lo dispara el lider del grupo

            //Pcodigo para determinar si ens grupo de hermanad
            bool isGuildParty = true;
            uint32 nGuild = 0;

            Player* member;
            for (auto miembro : grupo->GetMemberSlots())
            {
                member = ObjectAccessor::FindConnectedPlayer(miembro.guid);
                if (member)
                {                    
                    if (!nGuild) nGuild = member->GetGuildId();
                    if (!member->GetGuild() || nGuild != member->GetGuildId())
                    {
                        isGuildParty = false;
                        break;
                    }
                }
            }
            if (!nGuild)
                isGuildParty = false;


            //Si es un grupo y no una raid
            if (!grupo->isRaidGroup())
            {
                try
                {
                    std::string plr = player->GetName();
                    std::string boss_n = boss->GetName();
                    std::string tag_colour = "7bbef7";
                    std::string plr_colour = "ff0000";
                    std::string boss_colour = "18be00";

                    std::string msg = "|CFF";
                    msg += tag_colour;
                    msg += "[IMAGEN AQUI!]|r:|cff";
                    msg += plr_colour;
                    if (isGuildParty)
                    {
                        msg += " La Hermandad ";
                        msg += player->GetGuildName();
                    }
                    else
                    {
                        msg += " El grupo de ";
                        msg += plr;
                    }
                    msg += " ha derrotado a |CFF";
                    msg += boss_colour;
                    msg += "[";
                    msg += boss_n;
                    msg += "]|r en ";

                    AreaTableEntry const* pArea = sAreaTableStore.LookupEntry(player->GetAreaId());
                    msg += pArea ? pArea->area_name[6] : "Zona desconocida";
                    sWorld->SendServerMessage(SERVER_MSG_STRING, msg.c_str());
                }
                catch (std::exception)
                {
                    std::string str = "";
                    str = "ERROR->BossAnnouncer02";
                    WorldPacket data(SMSG_NOTIFICATION, (str.size() + 1));
                    data << str;
                    sWorld->SendGlobalGMMessage(&data);
                }
                /*msg += " en ";

                uint32 totalsegundos = boss->GetCombatTimer() / 1000;
                uint32 minutos = totalsegundos / 60;
                uint32 segundos = totalsegundos % 60;
                if (minutos > 0)
                {
                msg += std::to_string(minutos);
                msg += minutos > 1 ? " minutos y " : " minuto y ";
                }
                msg += std::to_string(segundos);
                msg += segundos > 1 ? " segundos " : " segundo ";*/




            }
            else
            {
                try
                {
                    std::string plr = player->GetName();
                    std::string boss_n = boss->GetName();
                    std::string tag_colour = "7bbef7";
                    std::string plr_colour = "ff0000";
                    std::string boss_colour = "18be00";

                    std::string msg = "|CFF";
                    msg += tag_colour;
                    msg += "[ANUNCIO DE BOSS]|r:|cff";
                    msg += plr_colour;
                    if (isGuildParty)
                    {
                        msg += " La Hermandad ";
                        msg += player->GetGuildName();
                    }
                    else
                    {
                        msg += " La raid de ";
                        msg += plr;
                    }
                    msg += " ha derrotado a |CFF";
                    msg += boss_colour;
                    msg += "[";
                    msg += boss_n;
                    msg += "]|r en ";

                    AreaTableEntry const* pArea = sAreaTableStore.LookupEntry(player->GetAreaId());
                    msg += pArea ? pArea->area_name[6] : "Zona desconocida";
                    sWorld->SendServerMessage(SERVER_MSG_STRING, msg.c_str());
                }
                catch (std::exception ex)
                {
                    std::string str = "";
                    str = "ERROR->BossAnnouncer03";
                    WorldPacket data(SMSG_NOTIFICATION, (str.size() + 1));
                    data << str;
                    sWorld->SendGlobalGMMessage(&data);
                }
                /*msg += " en ";

                uint32 totalsegundos = boss->GetCombatTimer() / 1000;
                uint32 minutos = totalsegundos / 60;
                uint32 segundos = totalsegundos % 60;
                if (minutos > 0)
                {
                msg += std::to_string(minutos);
                msg += minutos > 1 ? " minutos y " : " minuto y ";
                }
                msg += std::to_string(segundos);
                msg += segundos > 1 ? " segundos " : " segundo ";

                msg += " con ";
                uint32 Tank = 0;
                uint32 DPS = 0;
                uint32 Healer = 0;
                for (auto miembro : grupo->GetMemberSlots())
                {
                if (miembro.roles == 2)
                Tank++;
                else if (miembro.roles == 3)
                DPS++;
                else if (miembro.roles == 4)
                Healer++;
                }

                msg += std::to_string(Tank);
                msg += Tank > 1 ? " tanques " : " tanque ";
                msg += std::to_string(DPS);
                msg += " DPS ";
                msg += std::to_string(Healer);
                msg += Healer > 1 ? " sanadores " : " sanador ";*/


            }
        }
    };
};

void AddSC_Boss_Announcer()
{
    new Boss_Announcer;
}
