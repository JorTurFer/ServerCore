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
#include "sc_npc_loyalty.h"
#include "WorldSession.h"
#include "GameEventMgr.h"
#include "LinQ.hpp"
#include "AuxMuestrarioMonturas.h"

using namespace LinQ;


namespace nsNpcLoyalty
{

	bool IsAllowedItem(Player * const player, RewardsMenuItem recompensa, bool skipPatchCheck)
	{
		if (player->IsGameMaster()) return true;

		//Compruebo level min
		if (recompensa.m_levelMin != 0 && player->getLevel() < recompensa.m_levelMin) return false;
		//Compruebo level max
		if (recompensa.m_levelMax != 0 && player->getLevel() > recompensa.m_levelMax) return false;

		//Compruebo itemlevel
		if (recompensa.m_itemLevel != 0 && player->GetAverageItemLevel() < recompensa.m_itemLevel) return false;

		//Compruebo item1
		if (recompensa.m_item1 != 0 && !player->HasItemCount(recompensa.m_item1, 1, false)) return false;
		//Compruebo item2
		if (recompensa.m_item2 != 0 && !player->HasItemCount(recompensa.m_item2, 1, false)) return false;

		//Compruebo la quest de ally
		if (player->GetTeam() == ALLIANCE && recompensa.m_questA != 0 && player->GetQuestStatus(recompensa.m_questA) != QUEST_STATUS_REWARDED) return false;

		//Compruebo la quest de horda
		if (player->GetTeam() == HORDE && recompensa.m_questH != 0 && player->GetQuestStatus(recompensa.m_questH) != QUEST_STATUS_REWARDED) return false;

		//Compruebo el logro
		if (recompensa.m_archievemnt != 0 && player->HasAchieved(recompensa.m_archievemnt)) return false;

		//Compruebo la raza
		if (recompensa.m_maskRace != 0)
		{
			if ((recompensa.m_maskRace & player->getRaceMask()) == 0)
				return false;
		}
		//Compruebo la clase
		if (recompensa.m_maskClass != 0)
		{
			if ((recompensa.m_maskClass & player->getClassMask()) == 0)
				return false;
		}

		//Compruebo la guild
		if (recompensa.m_guild != 0 && player->GetGuildId() == 0) return false;

		//Compruebo los eventos activos
		if (recompensa.m_evento != 0)
		{
			GameEventMgr::GameEventDataMap const& events = sGameEventMgr->GetEventMap();
			if (recompensa.m_evento >= events.size()) return false;

			GameEventMgr::ActiveEvents const& activeEvents = sGameEventMgr->GetActiveEventList();
			if (activeEvents.find(recompensa.m_evento) == activeEvents.end())
				return false;
		}

		//Compruebo los parches
		if (!skipPatchCheck && recompensa.m_parches && !player->GetSession()->HasPatch()) return false;

		return true;
	}

	bool IsAllowedCategory(Player * const player, uint32 menuID)
	{
		bool ret = false;
		auto menu = from(m_lstMenuRewards)
			>> where([&, menuID](RewardsMenuItem const & g) {	return g.m_menuID == menuID; })
			>> select([](RewardsMenuItem const & g) {return g; })
			>> to_list();

		for (RewardsMenuItem item : menu)
		{
			if (item.m_actionID != 0 && item.m_Recompensa == 0)
			{
				if (IsAllowedCategory(player, item.m_actionID))
				{
					ret = true;
				}
			}
			else if (item.m_actionID == 0 && item.m_Recompensa != 0)
			{
				if (IsAllowedItem(player, item))
				{
					ret = true;
				}
			}
		}

		return ret;
	}

	std::list<RewardsMenuItem, std::allocator<RewardsMenuItem>> m_lstMenuRewards;
	std::list<RewardsMenuTemplate, std::allocator<RewardsMenuTemplate>> m_lstMenuRewardsTemplate;

}

void LoadNpcLoyalty(void)
{
	const char *Table[] =
	{
		"custom_npc_loyalty_rewards",
		"custom_npc_loyalty_template",
	};


	//Ahora, leo los items y los coloco en mi matriz preformada
	QueryResult result = WorldDatabase.PQuery(
		"SELECT `menuID`, `optionID`, `actionID`, `name` , `name_sin_parches`, "
		//         0          1           2         3              4          
		"`reward`, `cantidad`,`levelMin`, `levelMax`, `itemLevel`, `item1`, `item2`, `questA`, `questH`, "
		//  5          6           7             8        9        10       11        12          13
		"`achievement`,`raceMask`,`guild`,`cost`,`popup`,`evento`,`parches`,`classMask`,`categoria`,`codigo`"
		//  14             15       16      17     18       19        20		21			22		   23	  	
		"FROM `%s` "
		"ORDER BY `menuID`, `optionID`", Table[0]);



	if (result)
	{
		nsNpcLoyalty::m_lstMenuRewards.clear();
		TC_LOG_INFO("server.loading", "Loading %s...", Table[0]);
		do
		{
			Field *fields = result->Fetch();

			nsNpcLoyalty::RewardsMenuItem item =
			{
				fields[0].GetUInt32(),          //menuID
				fields[1].GetUInt32(),          //optionID
				fields[2].GetUInt32(),          //actionID
				fields[3].GetString(),			//name
				fields[4].GetString(),			//name sin parches
				fields[5].GetUInt32(),			//recompensa
				fields[6].GetUInt32(),			//cantidad
				fields[7].GetUInt8(),			//levelMin
				fields[8].GetUInt8(),			//levelMax
				fields[9].GetUInt16(),			//itemLevel
				fields[10].GetUInt32(),			//Item1
				fields[11].GetUInt32(),			//Item2
				fields[12].GetUInt32(),			//QuestA
				fields[13].GetUInt32(),			//QuestH
				fields[14].GetUInt32(),			//Archievement
				fields[15].GetUInt32(),			//RaceMask
				fields[16].GetBool(),			//Guild
				fields[17].GetUInt32(),			//Cost
				fields[18].GetString(),			//popup
				fields[19].GetUInt32(),			//evento
				fields[20].GetBool(),			//Parches
				fields[21].GetUInt32(),			//ClassMask
				fields[22].GetUInt32(),			//Categoria			
				fields[23].GetString()			//Codigo
			};
			nsNpcLoyalty::m_lstMenuRewards.push_back(item);
		} while (result->NextRow());


		TC_LOG_INFO("server.loading", "Loaded %u Loyalty Rewards.", nsNpcLoyalty::m_lstMenuRewards.size());
	}
	else
		TC_LOG_ERROR("misc", "WARNING >> Loaded 0 Loyalty Rewards.");


	//Ahora, leo los templates 
	result = WorldDatabase.PQuery(
		"SELECT `templateID`, `itemID`, `cantidad`"
		//			 0			 1           2              		
		"FROM `%s` "
		"ORDER BY `templateID`", Table[1]);



	if (result)
	{
		nsNpcLoyalty::m_lstMenuRewardsTemplate.clear();
		TC_LOG_INFO("server.loading", "Loading %s...", Table[1]);
		do
		{
			Field *fields = result->Fetch();

			nsNpcLoyalty::RewardsMenuTemplate rewardTemplate =
			{
				fields[0].GetUInt32(),          //templateID
				fields[1].GetUInt32(),          //itemID
				fields[2].GetUInt32(),          //cantidad				
			};
			nsNpcLoyalty::m_lstMenuRewardsTemplate.push_back(rewardTemplate);
		} while (result->NextRow());


		TC_LOG_INFO("server.loading", "Loaded %u Loyalty Rewards Templates.", nsNpcLoyalty::m_lstMenuRewardsTemplate.size());
	}
	else
		TC_LOG_ERROR("misc", "WARNING >> Loaded 0 Loyalty Rewards Templates.");

	sMuestrarioMonturas->LoadMuestrario(nsNpcLoyalty::m_lstMenuRewards);
}

#undef LOYALTY
