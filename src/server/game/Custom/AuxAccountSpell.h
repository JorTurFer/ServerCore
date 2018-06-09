#ifndef AUXACCOUNTSPELL_H
#define AUXACCOUNTSPELL_H

#include "ScriptMgr.h"
#include "DatabaseEnv.h"
#include "Log.h"

#include <Map.h>

enum AccountSpellType
{
    AccountSpellTypeNONE = -1,
    AccountSpellTypeMount = 0,
    AccountSpellTypePet = 1
};

struct AccountSpellDataRow
{
    uint32	m_maskRace;
    uint32	m_maskClass;
    bool	m_learn;
    uint32  m_visualSpell;
    AccountSpellType m_SpellType;
};

//Defino el mapa de items de cada categoria
typedef std::multimap<uint32, AccountSpellDataRow> AccountSpellDataTable;


class TC_GAME_API FiltroSpells
{
private:
    AccountSpellDataTable m_SpellsAccount;
public:
    static FiltroSpells* instance();
    void Load_Account_Spell_Data();
    AccountSpellDataRow GetRow(uint32 SpellID);
    AccountSpellDataTable GetAll() { return m_SpellsAccount; }

};
#define sFiltroSpells FiltroSpells::instance()
#endif
