#include "Channel.h"
#include "ScriptMgr.h"
#include "Chat.h"


class System_Taberna : public PlayerScript
{
public:
	System_Taberna() : PlayerScript("System_Taberna") { }

    void Faction_Icon(Player* player, std::string& msg, uint32 lang)
    {
		if (player->isGMChat()) 
			return;
        std::string _UI_FriendsFrame_A = "|TInterface/FriendsFrame/PlusManz-Alliance:20:20|t",
                    _UI_FriendsFrame_H = "|TInterface/FriendsFrame/PlusManz-Horde:20:20|t";
                    
		msg = (player->GetTeam() == HORDE) ? _UI_FriendsFrame_H + msg : _UI_FriendsFrame_A + msg;

    };

    void OnChat(Player* player, uint32 /*type*/, uint32 lang, std::string& msg, Channel* channel) override
    {
		std::string canal = channel->GetName();
		std::transform(canal.begin(), canal.end(), canal.begin(), ::tolower);
		if(canal == "taberna")  Faction_Icon(player, msg, lang);
    }
};

void AddSC_System_Taberna()
{
    new System_Taberna();
}
