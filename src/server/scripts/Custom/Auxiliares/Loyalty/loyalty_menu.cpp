#include "../../Auxiliares/Loyalty/loyalty_menu.h"
#include "DBCStores.h"


#define LOYALTY_GOSSIP_PUNTOS_TIEMPO		300000
#define LOYALTY_GOSSIP_PUNTOS				300001
#define LOYALTY_SHOP_PUNTOS				    300002
#define LOYALTY_WORLDSTATE_TIEMPO			43540
#define LOYALTY_WORLDSTATE_PUNTOS			43541
#define LOYALTY_WORLDSTATE_SHOP				43542

#define GOSSIP_CLOSE             80000
#define GOSSIP_ADD_POINTS		 90000
#define GOSSIP_REWARDS           90100
#define GOSSIP_SHOW_REWARDS     100000
#define GOSSIP_NEXT_PAGE        100001
#define GOSSIP_PREVIOUS_PAGE    100002
#define GOSSIP_NOTHING		    100003
#define GOSSIP_LOYALTY			100100
#define GOSSIP_MAIN_MENU        100600

const char * getQualityName(uint32 quality)
{
	switch (quality)
	{
    case ITEM_QUALITY_POOR: return "|CFF545454";
    case ITEM_QUALITY_NORMAL: return "|CFF9d9d9d";
    case ITEM_QUALITY_UNCOMMON: return "|CFF0B5F00";
    case ITEM_QUALITY_RARE: return "|CFF004C97";
    case ITEM_QUALITY_EPIC: return "|CFF6711A2";
    case ITEM_QUALITY_LEGENDARY: return "|CFF995516";
    case ITEM_QUALITY_ARTIFACT: return "|CFF735C15";
    case ITEM_QUALITY_HEIRLOOM: return "|CFF0C7362";
    default: return "|CFFe5cc80";
	}
}


namespace
{
	// Conversion function int->string
	std::string ConvertStr(const int64 &val)
	{
		std::ostringstream ostr;
		ostr << val;
		return ostr.str();
	}

	// Conversion function intMoney->stringMoney
	std::string ConvertMoney(const uint32 &Money)
	{
		std::string Str(ConvertStr(Money));
		uint32 SizeStr = Str.length();

		if (SizeStr > 4)
			Str = Str.insert(Str.length() - 4, "O");
		if (SizeStr > 2)
			Str = Str.insert(Str.length() - 2, "P");
		Str += "c";

		return Str;
	}

	// Display categories
	bool AffichCat(Player * const player, WorldObject* const me, uint32 action, uint32 offset = 0)
	{
		uint32 cantidad = from(m_lstMenuRewards)
			>> where([&, action, player](RewardsMenuItem const & g) {	return g.m_menuID == action && IsAllowedItem(player, g); })
			>> select([](RewardsMenuItem const & g) {return g; })
			>> skip(offset * 29)
			>> take(30)
			>> count();

		auto menu = from(m_lstMenuRewards)
			>> where([&, action, player](RewardsMenuItem const & g) {	return g.m_menuID == action && IsAllowedItem(player, g); })
			>> select([](RewardsMenuItem const & g) {return g; })
			>> skip(offset * 29)
			>> take(29)
			>> to_list();

		for (RewardsMenuItem item : menu)
		{

			if (item.m_actionID != 0 && item.m_Recompensa == 0)
			{
				if (IsAllowedCategory(player, item.m_actionID))
				{
					AddGossipItemFor(player, 3, /*player->HasPatchs() ? item.m_name : */item.m_name_sin_parches, GOSSIP_SHOW_REWARDS, item.m_actionID); //book categorie	
				}
			}
			else if (item.m_actionID == 0 && item.m_Recompensa != 0)
			{
				std::string msg = "";
				std::string msgConfirmacion = "";
				ItemTemplate const* itemProto = sObjectMgr->GetItemTemplate(item.m_Recompensa);
				if (itemProto)
				{
					ItemDisplayInfoEntry const* info = GetItemDisplayInfoEntry(itemProto->DisplayInfoID);
					if (info)
					{
						msg = "|TInterface/Icons/";
						msg += info->m_inventoryIcon;
						msg += ":25:25|t";
					}
					msg += getQualityName(itemProto->Quality);
				}
				msg += /*player->HasPatchs() ? item.m_name :*/ item.m_name_sin_parches;
				if (item.m_popup_text == "")
				{
					msgConfirmacion = "Seguro que quieres comprar:\n" + msg;
					msgConfirmacion += "|r\n\nCoste: ";
					msgConfirmacion += std::to_string(item.m_cost);
					msgConfirmacion += " puntos";
				}
				else msgConfirmacion = item.m_popup_text;


				AddGossipItemFor(player, 3, msg, item.m_menuID, item.m_optionID, msgConfirmacion, 0, false); //book categorie	

			}
		}


		if (cantidad == 30)
		{
			AddGossipItemFor(player, 3, "Siguiente Hoja->", GOSSIP_NEXT_PAGE, action);
		}
		if (offset > 0)
		{
			AddGossipItemFor(player, 3, "<-Hoja Anterior", GOSSIP_PREVIOUS_PAGE, action);
		}


		RewardsMenuItem sender = from(m_lstMenuRewards)
			>> where([&, action](RewardsMenuItem const & g) {	return g.m_actionID == action; })
			>> select([](RewardsMenuItem const & g) {return g; })
			>> first_or_default();

		if (sender.m_menuID == 0 && sender.m_actionID == 0 && sender.m_optionID == 0)
			AddGossipItemFor(player, 3, "<-Menu Anterior", GOSSIP_MAIN_MENU, 0);
		else
			AddGossipItemFor(player, 3, "<-Menu Anterior", GOSSIP_SHOW_REWARDS, sender.m_menuID); //book categorie	





		SendGossipMenuFor(player, LOYALTY_GOSSIP_PUNTOS, me->GetGUID());
		return true;
	}

	bool _ShowCategory(Player * const player, WorldObject* const me)
	{
		//Extraigo los valores almacenados
		uint32 category = player->GetLoyaltyShopCategory();
		uint32 offset = player->GetLoyaltyShopOffset();
		uint32 limit = player->GetLoyaltyShopLimit();
		//Añado un hueco en caso de limit max para que pueda enviar el texto de atras
		if (limit >= 32)
			limit = 30;
		auto menu = from(m_lstMenuRewards)
			>> where([&, category, player](RewardsMenuItem const & g) {	return g.m_categoria == category && g.m_Recompensa != 0 && IsAllowedItem(player, g); })
			>> select([](RewardsMenuItem const & g) {return g; })
			>> orderby_ascending([](RewardsMenuItem const & g) {return g.m_codigo; })
			>> skip(offset)
			>> take(limit)
			>> to_list();

		for (RewardsMenuItem item : menu)
		{
			AddGossipItemFor(player, 3, item.m_codigo, item.m_menuID, item.m_optionID); //book categorie	
		}
		std::string strCodigoAtras = "|X-0000000|M-0000000|P-0|O-1|N-1|S_X|000000|000000|000000|000000|000000|000000|000000|000000|000000|000000|000000|000000|000000|C-SENDBACK";
		AddGossipItemFor(player, 3, strCodigoAtras, GOSSIP_NOTHING, GOSSIP_NOTHING); //Back
		AddGossipItemFor(player, 3, strCodigoAtras, GOSSIP_NOTHING, GOSSIP_NOTHING); //Back	


		SendGossipMenuFor(player, LOYALTY_SHOP_PUNTOS, me->GetGUID());
		return true;
	}

	bool GiveItem(Player* player, uint32 sender, uint32 action, bool shop = false)
	{
		std::vector<RewardsMenuTemplate> recompensas;

		RewardsMenuItem item = from(m_lstMenuRewards)
			>> where([&, sender, action](RewardsMenuItem const & g) {	return g.m_menuID == sender && g.m_optionID == action; })
			>> select([](RewardsMenuItem const & g) {return g; })
			>> first_or_default();

		//Compruebo que exista valor por seguridad
		if (item.m_menuID == 0 && item.m_actionID == 0 && item.m_optionID == 0)
		{
			return false;
		}

		if (item.m_Cantidad > 0)
		{
			RewardsMenuTemplate recompensa;
			recompensa.itemID = item.m_Recompensa;
			recompensa.cantidad = item.m_Cantidad;
			recompensas.push_back(recompensa);
		}
		else
		{
			recompensas = from(m_lstMenuRewardsTemplate)
				>> where([&, item](RewardsMenuTemplate const & g) {	return g.templateID == item.m_Recompensa; })
				>> select([](RewardsMenuTemplate const & g) {return g; })
				>> to_vector();
		}


		if (player->GetSession()->GetVotePoints() >= item.m_cost)
		{
			std::string msg;
			if (player->SendShopItem(recompensas))
			{
				player->GetSession()->RemoveVotePoints(item.m_cost);
				if (shop)
					msg = "|CFFDAF7A6Tienda del juego:|r ";
				else
					msg = "|CFFDAF7A6Servicio de fidelidad|r";

				if (recompensas.size() == 1)
				{
					msg += "|CFFFCFFE2: Se ha enviado el objeto|r |c";
					msg += "FFF4DA3B"; //Color del item, (sin c|)
					msg += "|Hitem:";
					msg += std::to_string(recompensas.begin()->itemID);
					msg += ":0:0:0:0:0:0:0:20|h[";
					msg += sObjectMgr->GetItemTemplate(recompensas.begin()->itemID)->Name1;
					msg += "]|h|r |CFFFCFFE2a tu buzon!|r";
				}
				else
				{
					msg += "|CFFFCFFE2: Se ha enviado el conjunto|r |c";
					msg += "FFF4DA3B"; //Color del item, (sin c|)
					msg += /*player->HasPatchs() ? item.m_name : */item.m_name_sin_parches;
					msg += "|r |CFFFCFFE2a tu buzon!|r";
				}
			}
			else
			{
				msg = "Se ha producido un error, vuelve a intentarlo pasado unos minutos, si el error persiste, ponete en contacto con un GM.";
			}
			ChatHandler handler = ChatHandler(player->GetSession());
			handler.PSendSysMessage(msg.c_str());
		}
		else
		{
			ChatHandler handler = ChatHandler(player->GetSession());
			handler.PSendSysMessage("Saldo insuficiente");
		}
		return true;
	}
}

bool Loyalty_Gossip::GossipSelect(Player *player, WorldObject* source, uint32 menuId, uint32 gossipListId)
{

	uint32 sender = player->PlayerTalkClass->GetGossipOptionSender(gossipListId);
	uint32 action = player->PlayerTalkClass->GetGossipOptionAction(gossipListId);
	ClearGossipMenuFor(player);
	ObjectGuid guid = source->GetGUID();
	switch (sender)
	{
		// Display rewards
	case GOSSIP_SHOW_REWARDS:		
		player->SetLoyaltyLastMenuOffset(0);
		if (!AffichCat(player, source, action))
			GossipHello(player, source);
		break;
	case GOSSIP_NEXT_PAGE:
		player->SetLoyaltyLastMenuOffset(player->GetLoyaltyLastMenuOffset() + 1);
		if (!AffichCat(player, source, action, player->GetLoyaltyLastMenuOffset()))
			GossipHello(player, source);
		break;
	case GOSSIP_PREVIOUS_PAGE:
		player->SetLoyaltyLastMenuOffset(player->GetLoyaltyLastMenuOffset() - 1);
		if (!AffichCat(player, source, action, player->GetLoyaltyLastMenuOffset()))
			GossipHello(player, source);
		break;
		// Display main menu
	case GOSSIP_MAIN_MENU:
		GossipHello(player, source);
		break;


	case GOSSIP_ADD_POINTS:
		//La comprobacion esta en el primer menu
		player->GetSession()->AddVotePoints(sWorld->getIntConfig(CONFIG_LOYALTY_NPC_POINTS), true);
		if (guid.IsCreature())
			source->ToCreature()->Whisper("Has Marcado tu asistencia, sigue acumulando puntos", LANG_UNIVERSAL, player);
		else if (guid.IsPlayer())
		{
			ChatHandler handler = ChatHandler(player->GetSession());
			handler.PSendSysMessage("Has Marcado tu asistencia, sigue acumulando puntos");
		}
		GossipHello(player, source);
		break;
	case GOSSIP_CLOSE:
		CloseGossipMenuFor(player);
		break;
	case GOSSIP_NOTHING:
		ShowCategory(player, source);
		break;
		//Additem
	default:
		GiveItem(player, sender, action, player->IsCurrentNPCGossip(LOYALTY_SHOP_HELLO_REQUEST));
		CloseGossipMenuFor(player);
		break;
	}
	return true;
}

bool Loyalty_Gossip::ShowCategory(Player * const player, WorldObject* source)
{
	CloseGossipMenuFor(player);
	ClearGossipMenuFor(player);
	return _ShowCategory(player, source);
}

bool Loyalty_Gossip::GossipHello(Player* player, WorldObject* source)
{
	player->SetCurrentNPCGossip(LOYALTY_GOSSIP_HELLO_REQUEST);
	CloseGossipMenuFor(player);
	ClearGossipMenuFor(player);
	uint32 unlastVote = 0;
	tm dateCambioVotacion;
	uint32 votePoints = player->GetSession()->GetVotePoints();
	time_t lastVote = player->GetSession()->GetLastVote();
    time_t nextVote;


	time_t timenow = time(0);
    localtime_r(&timenow, &dateCambioVotacion);
    dateCambioVotacion.tm_hour = 0;
    dateCambioVotacion.tm_sec = 0;
    dateCambioVotacion.tm_min = 0;

    nextVote = mktime(&dateCambioVotacion);

	if (lastVote <= nextVote)
	{
		//Actualizo el worldstate de la cabecera del gossip
		player->SendUpdateWorldState(LOYALTY_WORLDSTATE_PUNTOS, votePoints);


		std::string s = Trinity::StringFormat("Marcar Asistencia: obtendras %u puntos", sWorld->getIntConfig(CONFIG_LOYALTY_NPC_POINTS));
		AddGossipItemFor(player, 7, s, GOSSIP_ADD_POINTS, 0);
		AddGossipItemFor(player, 7, "Servicios", GOSSIP_SHOW_REWARDS, 1);
		AddGossipItemFor(player, 7, "Salir", GOSSIP_CLOSE, 0);
		SendGossipMenuFor(player, LOYALTY_GOSSIP_PUNTOS, source->GetGUID());
	}
	else
	{

		time_t timeDiff = (nextVote + (24 * 60 * 60)) - timenow;
		//Actualizo el worldstate de la cabecera del gossip
		player->SendUpdateWorldState(LOYALTY_WORLDSTATE_TIEMPO, time(NULL) + timeDiff);
		player->SendUpdateWorldState(LOYALTY_WORLDSTATE_PUNTOS, votePoints);

		AddGossipItemFor(player, 7, "Servicios", GOSSIP_SHOW_REWARDS, 1);
		AddGossipItemFor(player, 7, "Salir", GOSSIP_CLOSE, 0);
		SendGossipMenuFor(player, LOYALTY_GOSSIP_PUNTOS_TIEMPO, source->GetGUID());
	}
	return true;
}
