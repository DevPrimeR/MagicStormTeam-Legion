/*
* Copyright (C) 2008-2017 TrinityCore <http://www.trinitycore.org/>
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

#include "AreaTrigger.h"
#include "GameObject.h"
#include "GameObjectAI.h"
#include "Garrison.h"
#include "ObjectMgr.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "ScriptedEscortAI.h"


enum
{
    QUEST_THE_HOME_OF_THE_FROSTWOLFES   = 33868,
    QUEST_A_SONG_OF_FROST_AND_FIRE      = 33815,
    QUEST_OF_WOLFES_AND_WARRIORS        = 34402,

    QUEST_ESTABLISH_YOUR_GARRISON       = 34378,
};

enum
{
    SpellEarthrendingSlam = 165907,
    SpellRampage          = 148852
};

enum
{
    EventEarthrendingSlam = 1,
    EventRampage          = 2,
};

enum
{
    NPC_DUROTAN_BEGIN                       = 78272,
    NPC_OF_WOLFES_AND_WARRIOR_KILL_CREDIT   = 78407,
    NPC_ESTABLISH_YOUR_GARRISON_KILL_CREDIT = 79757,
};

enum
{
    SCENE_LANDING_TO_TOP_OF_HILL    = 771,
    SCENE_TOP_OF_HILL_TO_GARRISON   = 778,
    SCENE_PORTAL_OPENING            = 789,
    SCENE_DUROTAN_DEPARTS           = 798,
};

/// Passive Scene Object
class playerScript_frostridge_landing_to_hill : public PlayerScript
{
public:
    playerScript_frostridge_landing_to_hill() : PlayerScript("playerScript_frostridge_landing_to_hill") { }

    void OnSceneTriggerEvent(Player* player, uint32 p_SceneInstanceID, std::string p_Event) override
    {
        if (!player->GetSceneMgr().HasScene(p_SceneInstanceID, SCENE_LANDING_TO_TOP_OF_HILL))
            return;

        if (p_Event == "durotanIntroduced")
        {
            if (Creature* durotan = player->FindNearestCreature(NPC_DUROTAN_BEGIN, 50.0f))
                durotan->AI()->Talk(0);

            player->KilledMonsterCredit(NPC_DUROTAN_BEGIN);
        }
    }
};

/// Passive Scene Object
class playerScript_frostridge_hill_to_garrison : public PlayerScript
{
public:
    playerScript_frostridge_hill_to_garrison() : PlayerScript("playerScript_frostridge_hill_to_garrison") { }

    void OnSceneComplete(Player* p_Player, uint32 p_SceneInstanceId) override
    {
        if (!p_Player->GetSceneMgr().HasScene(p_SceneInstanceId, SCENE_TOP_OF_HILL_TO_GARRISON))
            return;

        p_Player->KilledMonsterCredit(NPC_OF_WOLFES_AND_WARRIOR_KILL_CREDIT);
    }
};

// 76411 - Drek'Thar - D�but Givrefeu
class npc_drekthar_frostridge_begin : public CreatureScript
{
public:
    npc_drekthar_frostridge_begin() : CreatureScript("npc_drekthar_frostridge_begin") { }

    bool OnQuestAccept(Player* player, Creature* creature, Quest const* quest)
    {
        if (quest->GetQuestId() == QUEST_A_SONG_OF_FROST_AND_FIRE)
        {
            player->GetSceneMgr().PlaySceneByPackageId(SCENE_LANDING_TO_TOP_OF_HILL);
        }

        return true;
    }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_drekthar_frostridge_beginAI(creature);
    }

    struct npc_drekthar_frostridge_beginAI : public ScriptedAI
    {
        npc_drekthar_frostridge_beginAI(Creature* creature) : ScriptedAI(creature) { }

        uint32 waitTime;

        void Reset()
        {
            waitTime = 1000;
        }

        void UpdateAI(const uint32 p_Diff)
        {
            if (waitTime > p_Diff)
            {
                waitTime -= p_Diff;
                return;
            }

            waitTime = 1000;

            std::list<Player*> playerList;
            me->GetPlayerListInGrid(playerList, 30.0f);

            for (Player* player : playerList)
            {
                if (player->GetQuestStatus(QUEST_THE_HOME_OF_THE_FROSTWOLFES) != QUEST_STATE_NONE)
                    continue;

                if (const Quest* quest = sObjectMgr->GetQuestTemplate(QUEST_THE_HOME_OF_THE_FROSTWOLFES))
                    player->AddQuest(quest, me);
            }
        }
    };
};

// 78272 - Durotan - D�but Givrefeu
class npc_durotan_frostridge_begin : public CreatureScript
{
public:
    npc_durotan_frostridge_begin() : CreatureScript("npc_durotan_frostridge_begin") { }

    bool OnQuestAccept(Player* player, Creature* creature, Quest const* quest)
    {
        if (quest->GetQuestId() == QUEST_OF_WOLFES_AND_WARRIORS)
        {
            player->GetSceneMgr().PlaySceneByPackageId(SCENE_TOP_OF_HILL_TO_GARRISON);
        }

        return true;
    }
};

// Revendication 169421
class spell_frostridge_claiming : public SpellScriptLoader
{
public:
    spell_frostridge_claiming() : SpellScriptLoader("spell_frostridge_claiming") { }

    class spell_frostridge_claiming_spellscript : public SpellScript
    {
        PrepareSpellScript(spell_frostridge_claiming_spellscript);

        void HandleDummy(SpellEffIndex effIndex)
        {
            if (!GetCaster())
                return;

            Player* player = GetCaster()->ToPlayer();

            if (!player)
                return;

            player->GetSceneMgr().PlaySceneByPackageId(SCENE_PORTAL_OPENING);
        }

        void Register()
        {
            OnEffectHitTarget += SpellEffectFn(spell_frostridge_claiming_spellscript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_frostridge_claiming_spellscript();
    }
};

// 233664 - Niveau de Geom�tre
class go_frostridge_master_surveyor : public GameObjectScript
{
public:
    go_frostridge_master_surveyor() : GameObjectScript("go_frostridge_master_surveyor") { }

    struct go_frostridge_master_surveyorAI : public GameObjectAI
    {
        go_frostridge_master_surveyorAI(GameObject* p_Go) : GameObjectAI(p_Go)
        {
            waitTimer = 1000;
        }

        uint32 waitTimer;

        void UpdateAI(uint32 diff) override
        {
            if (waitTimer > diff)
            {
                waitTimer -= diff;
                return;
            }

            waitTimer = 1000;

            std::list<Player*> playerList;
            go->GetPlayerListInGrid(playerList, 10.0f);

            for (Player* player : playerList)
            {
                if (player->GetQuestStatus(QUEST_ESTABLISH_YOUR_GARRISON) != QUEST_STATUS_INCOMPLETE &&
                    player->GetQuestStatus(QUEST_ESTABLISH_YOUR_GARRISON) != QUEST_STATUS_COMPLETE)
                    continue;

                if (!player->GetGarrison(GARRISON_TYPE_GARRISON))
                {
                    player->CreateGarrison(player->IsInAlliance() ? GARRISON_SITE_WOD_ALLIANCE : GARRISON_SITE_WOD_HORDE);

                    uint32 movieID = player->GetGarrison(GARRISON_TYPE_GARRISON)->GetSiteLevel()->MovieID;
                    uint32 mapID = player->GetGarrison(GARRISON_TYPE_GARRISON)->GetSiteLevel()->MapID;

                    player->AddMovieDelayedTeleport(movieID, mapID, 5698.020020f, 4512.1635574f, 127.401695f, 2.8622720f);
                    player->SendMovieStart(movieID);

                    player->KilledMonsterCredit(NPC_ESTABLISH_YOUR_GARRISON_KILL_CREDIT);
                }
            }
        }

    };

    GameObjectAI* GetAI(GameObject* p_Go) const
    {
        return new go_frostridge_master_surveyorAI(p_Go);
    }
};

// 80167 - Groog
class npc_groog : public CreatureScript
{
    public:
        npc_groog() : CreatureScript("npc_groog") { }

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_groogAI(creature);
        }

        struct npc_groogAI : public ScriptedAI
        {
            npc_groogAI(Creature* creature) : ScriptedAI(creature) { }

            EventMap m_Events;

            void Reset()
            {
                me->setFaction(14);
            }

            void EnterCombat(Unit* p_Victim)
            {
                m_Events.Reset();

                m_Events.ScheduleEvent(EventEarthrendingSlam, 3000);
                m_Events.ScheduleEvent(EventRampage, 7000);
            }

            void UpdateAI(uint32 const p_Diff)
            {
                m_Events.Update(p_Diff);

                if (me->HasUnitState(UNIT_STATE_CASTING) || !UpdateVictim())
                    return;

                switch (m_Events.ExecuteEvent())
                {
                    case EventEarthrendingSlam:
                        me->CastSpell(me, SpellEarthrendingSlam, false);
                        m_Events.ScheduleEvent(EventEarthrendingSlam, 15000);
                        break;
                    case EventRampage:
                        me->AddAura(SpellRampage, me);
                        m_Events.ScheduleEvent(EventRampage, 15000);
                        break;
                }

                DoMeleeAttackIfReady();
            }
        };
};

/// Rampage - 148852
class spell_groog_rampage : public SpellScriptLoader
{
    public:
        spell_groog_rampage() : SpellScriptLoader("spell_groog_rampage") { }

        class spell_groog_rampage_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_groog_rampage_AuraScript);

            void OnTick(AuraEffect const* aurEff)
            {
                Unit* l_Caster = GetCaster();

                if (!l_Caster)
                    return;

                PreventDefaultAction();

                std::list<Player*> l_PlayerList;
                l_Caster->GetPlayerListInGrid(l_PlayerList, 2.0f);

                l_Caster->HandleEmoteCommand(EMOTE_ONESHOT_ATTACK1H);

                for (Player* l_Player : l_PlayerList)
                {
                    if (l_Player->HasUnitState(UNIT_STATE_ROOT))
                        continue;

                    l_Player->KnockbackFrom(l_Player->m_positionX, l_Player->m_positionY, 10.0f, 10.0f);
                }
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_groog_rampage_AuraScript::OnTick, EFFECT_1, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_groog_rampage_AuraScript();
        }
};

void AddSC_frostfire_ridge()
{
    /* BEGIN */
    new playerScript_frostridge_landing_to_hill();
    new playerScript_frostridge_hill_to_garrison();

    new npc_drekthar_frostridge_begin();
    new npc_durotan_frostridge_begin();

    new go_frostridge_master_surveyor();

    new spell_frostridge_claiming();

    /* BOSS */
    new npc_groog();
    new spell_groog_rampage();
}