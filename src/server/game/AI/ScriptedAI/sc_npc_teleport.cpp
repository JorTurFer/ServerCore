/*
 * Copyright (C) 20??-2008 Wilibald09
 * Copyright (C) 2011-2015 ArkCORE <http://www.arkania.net/>
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Player.h"
#include "sc_npc_teleport.h"
#include "WorldSession.h"
#include "DatabaseEnv.h"
#include "GameEventMgr.h"

#define TELE    nsNpcTel::CatDest
#define PAGE    nsNpcTel::Page
#define PAGEI   PAGE::Instance

nsNpcTel::VCatDest nsNpcTel::TabCatDest;

uint32 PAGE::operator [] (Player * const player) const
{
    for (VInst_t i(0); i < m_TabInstance.size(); ++i)
    {
        if (m_TabInstance[i].GetPlayer() == player)
            return m_TabInstance[i].GetPageId();
    }
    return 0;
}

PAGE::Instance & PAGE::operator () (Player * const player)
{
    for (VInst_t i(0); i < m_TabInstance.size(); ++i)
    {
        if (m_TabInstance[i].GetPlayer() == player)
            return m_TabInstance[i];
    }
    m_TabInstance.push_back(Instance(player));
    return m_TabInstance.back();
}

PAGE::Instance & PAGEI::operator = (const uint32 &id)
{
    m_PageId = id;
    return *this;
}

PAGE::Instance & PAGEI::operator ++ (void)
{
    ++m_PageId;
    return *this;
}

PAGE::Instance PAGEI::operator ++ (int32)
{
    Instance tmp(*this);
    ++m_PageId;
    return tmp;
}

PAGE::Instance & PAGEI::operator -- (void)
{
    --m_PageId;
    return *this;
}

PAGE::Instance PAGEI::operator -- (int32)
{
    Instance tmp(*this);
    --m_PageId;
    return tmp;
}

TELE::CatDest(const CatValue cat, const CatName catname)
    : m_catvalue(cat), m_catname(catname)
{
    m_TabDest.clear();
}

std::string TELE::GetName(const uint8 loc /* = 0 */, const bool IsGM /* = false */) const
{
    std::string icon = m_catname.icon;
    std::string size_v = m_catname.m_size_v;
    std::string size_h = m_catname.m_size_h;
    std::string colour = m_catname.colour;
    std::string name = m_catname.name[loc];
    if (name.length() == 0)
        name = m_catname.name[0];

    return "|T" + icon + ":" + size_v + ":" + size_h + "|t|c" + colour + " " + name;;
}

bool TELE::IsAllowedDestine(Player * const player, uint32 categoria, uint32 posicion) const
{
    if (player->IsGameMaster()) return true;
    Dest destino = TabCatDest[categoria].GetDest(posicion);

    //Compruebo level min
    if (destino.m_levelMin != 0 && player->getLevel() < destino.m_levelMin) return false;
    //Compruebo level max
    if (destino.m_levelMax != 0 && player->getLevel() > destino.m_levelMax) return false;

    //Compruebo itemlevel
    if (destino.m_itemLevel != 0 && player->GetAverageItemLevel() < destino.m_itemLevel) return false;

    //Compruebo item1
    if (destino.m_item1 != 0 && !player->HasItemCount(destino.m_item1, 1, false)) return false;
    //Compruebo item2
    if (destino.m_item2 != 0 && !player->HasItemCount(destino.m_item2, 1, false)) return false;

    //Compruebo la quest de ally
    if (player->GetTeam() == ALLIANCE && destino.m_questA != 0 && player->GetQuestStatus(destino.m_questA) != QUEST_STATUS_REWARDED) return false;

    //Compruebo la quest de horda
    if (player->GetTeam() == HORDE && destino.m_questH != 0 && player->GetQuestStatus(destino.m_questH) != QUEST_STATUS_REWARDED) return false;

    //Compruebo el logro
    if (destino.m_archievemnt != 0 && player->HasAchieved(destino.m_archievemnt)) return false;

    //Compruebo la raza
    if (destino.m_maskRace != 0)
    {
        uint32 debugRaceMask = player->getRaceMask();
        uint32 debugRace = player->getRace();
        if ((destino.m_maskRace & player->getRaceMask()) == 0)
            //if (!(destino.m_maskRace & player->getRaceMask()))
            return false;
    }

    //Compruebo la guild
    if (destino.m_guild != 0 && player->GetGuildId() == 0) return false;

    //Compruebo los eventos activos
    if (destino.m_evento != 0)
    {
        GameEventMgr::GameEventDataMap const& events = sGameEventMgr->GetEventMap();
        if (destino.m_evento >= events.size()) return false;

        GameEventMgr::ActiveEvents const& activeEvents = sGameEventMgr->GetActiveEventList();
        return activeEvents.find(destino.m_evento) != activeEvents.end();
    }

    //Compruebo los parches
    if (destino.m_parches && !player->GetSession()->HasPatch()) return false;

    return true;
}

bool TELE::IsAllowedCategory(Player * const player) const
{
    for (int j = 0; j < TabCatDest.size(); j++)
    {
        if (TabCatDest[j].GetCatID() != m_catvalue.catid) continue;
        for (uint32 i = 0; i < TabCatDest[j].size(); i++)
        {
            if (TabCatDest[j].IsAllowedDestine(player, j, i)) return true;
        }
    }
    return false;
}

uint32 TELE::CountOfCategoryAllowedBy(Player * const player)
{
    uint32 count(0);
    for (VCatDest_t i(0); i < TabCatDest.size(); ++i)
    {
        if (TabCatDest[i].IsAllowedCategory(player))
            ++count;
    }
    return count;
}

bool nsNpcTel::IsValidData(CatValue catvalue) //  const uint32 &cat, const Flag &flag, const uint64 &data0, const uint32 &data1
{
    return true;
}
bool nsNpcTel::IsAllowedTele(Player * const player, TeleportMenuItem recompensa)
{
    //Compruebo level min
    if (recompensa.m_levelMin != 0 && player->getLevel() < recompensa.m_levelMin) return false;
    //Compruebo level max
    if (recompensa.m_levelMax != 0 && player->getLevel() > recompensa.m_levelMax) return false;

    //Compruebo la raza
    if (recompensa.m_maskRace != 0)
    {
        if ((recompensa.m_maskRace & player->getRaceMask()) == 0)
            return false;
    }

    return true;
}
namespace nsNpcTel
{
    std::list<TeleportMenuItem, std::allocator<TeleportMenuItem>> m_lstMenuTeleport;
}


void LoadNpcTele(void)
{
    const char *Table[] =
    {
        "custom_npc_tele_category",
        "custom_npc_tele_destination",
        "custom_npc_tele_association",
    };

    nsNpcTel::m_lstMenuTeleport.clear();
    //0       1         2           3           4       5      6     7
    QueryResult result = WorldDatabase.PQuery("select a.cat_id ,d.id, d.levelMin,d.levelMax, d.raceMask,d.cost,d.map, d.pos_X,"
        // 8        9         10          11
        "d.pos_Y,d.pos_Z,d.orientation, d.popup from custom_npc_tele_destination d inner join custom_npc_tele_association a on a.dest_id = d.id");

    if (result)
    {
        do
        {
            Field *fields = result->Fetch();
            nsNpcTel::TeleportMenuItem item =
            {
                fields[0].GetUInt32(),          //Categoria
                fields[1].GetUInt32(),          //ID
                fields[2].GetUInt8(),          //Min Level
                fields[3].GetUInt8(),          //Max Level
                fields[4].GetUInt32(),          //Race Mask
                fields[5].GetUInt32(),          //Cost
                fields[6].GetUInt32(),          //Map
                fields[7].GetFloat(),           //X
                fields[8].GetFloat(),           //Y
                fields[9].GetFloat(),           //Z
                fields[10].GetFloat(),          //Orientacion
                fields[11].GetString(),         //PopUp
            };
            nsNpcTel::m_lstMenuTeleport.push_back(item);
        } while (result->NextRow());
    }





    result = WorldDatabase.PQuery(
        "SELECT "/*`flag`, `data0`, `data1`,*/ "`cat_id`, C.`icon` `iconcat`, C.`size_v`,C.`size_h`,  C.`colour` `colourcat`, C.`name` `namecat`, "
        //       0       1        2        3        0             1                   2                        3                    4          
        "C.`name_loc1`, C.`name_loc2`, C.`name_loc3`, C.`name_loc4`, C.`name_loc5`, C.`name_loc6`, C.`name_loc7`, C.`name_loc8`, "
        //     5              6               7             8               9            10               11            12
        "D.`icon` `icondest`, D.`size_v`,D.`size_h`, D.`colour` `colourdest`, D.`name` `namedest`, "
        //    13                      14                    15                      16
        "D.`name_loc1`, D.`name_loc2`, D.`name_loc3`, D.`name_loc4`, D.`name_loc5`, D.`name_loc6`, D.`name_loc7`, D.`name_loc8`, "
        //  17             18             19             20             21             22             23             24
        "`pos_X`, `pos_Y`, `pos_Z`, `orientation`, `map`, `levelMin`, `levelMax`, `itemLevel`, `item1`, `item2`, `questA`, `questH`, "
        //25       26       27       28             29     30			31			  32		 33		  34		35		  36
        "`achievement`,`raceMask`,`guild`,`cost`,`coin`, `spells`, `auras`, `popup`, `descuento`,`tipogrupo`,`evento`,`parches`"
        //  37              38        39     40     41       42		43		  44		45			46			47       48
        "FROM `%s` C, `%s` D, `%s` A "
        "WHERE C.`id` = `cat_id` AND D.`id` = `dest_id` "
        "ORDER BY `namecat`, `cat_id`, `namedest`", Table[0], Table[1], Table[2]);

    nsNpcTel::TabCatDest.clear();

    if (result)
    {
        TC_LOG_INFO("server.loading", "Loading %s, %s and %s...", Table[0], Table[1], Table[2]);

        uint32 catid = 0;
        uint32 nbDest = 0;
        bool IsValidCat = true;
        bool FirstTime = true;

        do
        {
            Field *fields = result->Fetch();

            if (!IsValidCat && catid == fields[3].GetUInt32() && !FirstTime)
                continue;

            IsValidCat = true;
            FirstTime = false;

            nsNpcTel::CatValue catvalue =
            {
                fields[0].GetUInt32(),
                /*   (nsNpcTel::Flag)fields[0].GetUInt8(),
                   fields[1].GetUInt64(),
                   fields[2].GetUInt32()*/
            };

            if (!nsNpcTel::IsValidData(catvalue))
            {
                IsValidCat = false;
                catid = catvalue.catid;
                continue;
            }

            if (catid != catvalue.catid)
            {
                catid = catvalue.catid;
                nsNpcTel::CatName catname =
                {
                    fields[1].GetString(),  // Cat icon
                    fields[2].GetString(),  // Cat size Vertical
                    fields[3].GetString(),	// Cat size Horizontal
                    fields[4].GetString(),  // Cat colour
                    {
                        fields[5].GetString(),  // Cat Name
                        fields[6].GetString(),  // Cat Name Loc1
                        fields[7].GetString(),  // Cat Name Loc2
                        fields[8].GetString(),  // Cat Name Loc3
                        fields[9].GetString(),  // Cat Name Loc4
                        fields[10].GetString(),  // Cat Name Loc5
                        fields[11].GetString(), // Cat Name Loc6
                        fields[12].GetString(), // Cat Name Loc7
                        fields[13].GetString()  // Cat Name Loc8
                    }
                };

                nsNpcTel::CatDest categorie(catvalue, catname);
                nsNpcTel::TabCatDest.push_back(categorie);
            }

            nsNpcTel::Dest item =
            {
                fields[14].GetString(),    // Dest icon
                fields[15].GetString(),    // Dest size Vertical
                fields[16].GetString(),    // Dest size Horizontal
                fields[17].GetString(),    // Dest colour
                {
                    fields[18].GetString(),    // Dest Name
                    fields[19].GetString(),    // Dest Name_loc1
                    fields[20].GetString(),    // Dest Name_loc2
                    fields[21].GetString(),    // Dest Name_loc3
                    fields[22].GetString(),    // Dest Name_loc4
                    fields[23].GetString(),    // Dest Name_loc5
                    fields[24].GetString(),    // Dest Name_loc6
                    fields[25].GetString(),    // Dest Name_loc7
                    fields[26].GetString()     // Dest Name_loc8
                },
                fields[27].GetFloat(),      // X
                fields[28].GetFloat(),      // Y
                fields[29].GetFloat(),      // Z
                fields[30].GetFloat(),      // Orientation
                fields[31].GetUInt16(),     // Map
                fields[32].GetUInt8(),      // LevelMin
                fields[33].GetUInt8(),      // LevelMax
                fields[34].GetUInt32(),     // itemLevel
                fields[35].GetUInt32(),		// item1
                fields[36].GetUInt32(),     // item2
                fields[37].GetUInt32(),		// questA
                fields[38].GetUInt32(),     // questH
                fields[39].GetUInt32(),		// archievement
                fields[40].GetUInt32(),     // raceMask
                fields[41].GetUInt32(),		// guild
                fields[42].GetUInt32(),     // cost
                fields[43].GetUInt32(),		// coin
                fields[44].GetString(),		// spells
                fields[45].GetString(),		// auras		
                fields[46].GetString(),		// texto popup
                fields[47].GetUInt32(),	  	// descuento
                fields[48].GetUInt8(),      // tipogrupo
                fields[49].GetUInt32(),	  	// evento
                fields[50].GetBool()	  	// parches
            };

            nsNpcTel::TabCatDest.back().AddDest(item);

            ++nbDest;
        } while (result->NextRow());

        TC_LOG_INFO("server.loading", " ");
        TC_LOG_INFO("server.loading", "Loaded %u npc_teleport.", nbDest);
    }
    else
        TC_LOG_ERROR("misc", "WARNING >> Loaded 0 npc_teleport.");
}


#undef TELE
