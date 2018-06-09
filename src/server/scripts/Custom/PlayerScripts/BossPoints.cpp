#include "ScriptPCH.h"
#include "World.h"
#include "Group.h"
#include "Chat.h"
#include "World.h"
#include "WorldSession.h"

class Boss_Point : public PlayerScript
{
public:
    Boss_Point() : PlayerScript("Boss_Point") {}

    void OnCreatureKill(Player* player, Creature* boss)
    {

        if (!boss || (!boss->IsDungeonBoss() && !boss->isWorldBoss()))
            return;
        if (!player || !player->GetMap() || !player->GetMap()->IsRaid())
            return;
		if (boss->GetEntry() == 18847 || boss->GetEntry() == 18829 || boss->GetEntry() == 17256 || boss->GetOriginalEntry() == 36633)
            return;



        uint32 nPuntos = 2;
        try
        {
            switch (boss->GetEntry())
            {
            case 15990:
                nPuntos = 10;
                break;
            case 30061:
                nPuntos = 10;
                break;
            case 34564:
                nPuntos = 10;
                break;
            case 34566:
                nPuntos = 10;
                break;
            case 35615:
                nPuntos = 10;
                break;
            case 35616:
                nPuntos = 10;
                break;
            case 36597:
                nPuntos = 10;
                break;
            case 39166:
                nPuntos = 10;
                break;
            case 39167:
                nPuntos = 10;
                break;
            case 39168:
                nPuntos = 10;
                break;
            default:
                break;
            }
        }
        catch(exception)
        {
            std::string str = "";
            str = "ERROR->BossPoints01";
            WorldPacket data(SMSG_NOTIFICATION, (str.size() + 1));
            data << str;
            sWorld->SendGlobalGMMessage(&data);
        }

        std::string msg = "Has recibido " + std::to_string(nPuntos) + " puntos de asistencia";

        Group* grupo = player->GetGroup();
        if (grupo)
        {
            Player* member;
            for (auto miembro : grupo->GetMemberSlots())
            {
                try
                {
                    member = ObjectAccessor::FindConnectedPlayer(miembro.guid);
                    //Compruebo que exista el miembro (este conectado) y este en el mismo mapa que quien ha matado (evitar que este fuera de la raid)
                    if (member && member->GetMap()->GetId() == player->GetMap()->GetId())
                    {
                        member->GetSession()->AddVotePoints(nPuntos, false);
                        ChatHandler chat(member->GetSession());
                        chat.PSendSysMessage(msg.c_str());
                    }
                }
                catch (std::exception)
                {
                    std::string str = "";
                    str = "ERROR->BossPoints02";
                    WorldPacket data(SMSG_NOTIFICATION, (str.size() + 1));
                    data << str;
                    sWorld->SendGlobalGMMessage(&data);
                }
            }
        }
        else
        {
            try
            {
                player->GetSession()->AddVotePoints(nPuntos, false);
                ChatHandler chat(player->GetSession());
                chat.PSendSysMessage(msg.c_str());
            }
            catch (std::exception)
            {
                std::string str = "";
                str = "ERROR->BossPoints03";
                WorldPacket data(SMSG_NOTIFICATION, (str.size() + 1));
                data << str;
                sWorld->SendGlobalGMMessage(&data);
            }
        }
    }
};

void AddSC_Boss_Point()
{
    new Boss_Point;
}
