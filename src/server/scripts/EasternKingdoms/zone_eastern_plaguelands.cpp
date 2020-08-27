#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "Player.h"
#include "WorldSession.h"

// Ours

enum eEris
{
    QUEST_BALANCE_OF_LIGHT_AND_SHADOW    = 7622,
    ITEM_EYE_OF_DIVINITY                = 18646,

    NPC_INJURED_PEASANT                    = 14484,
    NPC_PLAGUED_PEASANT                    = 14485,
    NPC_SCOURGE_ARCHER                    = 14489,

    EVENT_SUMMON_PEASANTS                = 1,
    EVENT_CHECK_PLAYER                    = 2,
    EVENT_SUMMON_ARCHERS                = 3,

    SPELL_SHOOT                            = 23073,
    SPELL_DEATHS_DOOR                    = 23127,
    SPELL_SEETHING_PLAGUE                = 23072,
    SPELL_ERIS_BLESSING                    = 23108,
};

class npc_eris_hevenfire : public CreatureScript
{
public:
    npc_eris_hevenfire() : CreatureScript("npc_eris_hevenfire") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_eris_hevenfireAI(creature);
    }

    bool OnQuestAccept(Player* player, Creature* creature, Quest const* quest)
    {
        if (quest->GetQuestId() == QUEST_BALANCE_OF_LIGHT_AND_SHADOW)
        {
            creature->AI()->SetData(player->GetFaction(), 0);
            creature->AI()->SetGUID(player->GetGUID());
        }

        return true;
    }

    struct npc_eris_hevenfireAI : public ScriptedAI
    {
        npc_eris_hevenfireAI(Creature* c) : ScriptedAI(c), summons(me) {}

        SummonList summons;
        EventMap events;
        ObjectGuid _playerGUID;
        uint8 _counter;
        uint8 _savedCount;
        uint8 _deathCount;
        bool _spoken;
        uint32 _faction;

        void Reset()
        {
            _faction = 0;
            _spoken = false;
            _savedCount = 0;
            _deathCount = 0;
            _counter = 0;
            _playerGUID.Clear();
            events.Reset();
            summons.DespawnAll();
            me->SetUInt32Value(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP|UNIT_NPC_FLAG_QUESTGIVER);
        }

        void SetData(uint32 faction, uint32, Unit*) override
        {
            _faction = faction;
        }
         
        void SetGuidData(uint32 /*DataId*/, ObjectGuid guid)
        {
            _playerGUID.Set(guid);
            me->SetUInt32Value(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_NONE);
            events.Reset();
            summons.DespawnAll();

            events.ScheduleEvent(EVENT_CHECK_PLAYER, 1000);
            events.ScheduleEvent(EVENT_SUMMON_ARCHERS, 4000);
            events.ScheduleEvent(EVENT_SUMMON_PEASANTS, 8000);
        }

        bool CanBeSeen(Player const* player)
        {
            // requires this trinket to be seen 
            return player->HasItemCount(ITEM_EYE_OF_DIVINITY, 1);
        }

        void SummonArchers()
        {
            me->SummonCreature(NPC_SCOURGE_ARCHER, 3330.18f, -3078.97f, 171.814f, 0.799463f);
            me->SummonCreature(NPC_SCOURGE_ARCHER, 3328.34f, -3017.88f, 171.544f, 6.26976f);
            me->SummonCreature(NPC_SCOURGE_ARCHER, 3333.71f, -3052.4f, 174.171f, 0.391055f);
            me->SummonCreature(NPC_SCOURGE_ARCHER, 3316.22f, -3035.49f, 166.428f, 0.163288f);
            me->SummonCreature(NPC_SCOURGE_ARCHER, 3371.54f, -3067.75f, 174.942f, 1.96578f);
            me->SummonCreature(NPC_SCOURGE_ARCHER, 3379.39f, -3060.11f, 181.617f, 2.82186f);
            me->SummonCreature(NPC_SCOURGE_ARCHER, 3352.44f, -3079.01f, 179.07f, 1.32175f);
            me->SummonCreature(NPC_SCOURGE_ARCHER, 3363.07f, -3077.43f, 183.0f, 1.78121f);
            me->SummonCreature(NPC_SCOURGE_ARCHER, 3348.11f, -2991.02f, 172.304f, 4.07064f);
            me->SummonCreature(NPC_SCOURGE_ARCHER, 3377.42f, -3039.77f, 172.594f, 3.20671f);
            me->SummonCreature(NPC_SCOURGE_ARCHER, 3363.87f, -3010.4f, 185.387f, 3.81932f);
        }

        void SummonPeasants()
        {
            for (uint8 i = 0; i < 12; ++i)
            {
                float x = 3358 + frand(-6.0f, 6.0f);
                float y = -3049 + frand(-6.0f, 6.0f);
                float z = 165.25;
                float o = 2.0;
                me->SummonCreature(roll_chance_i(5) ? NPC_PLAGUED_PEASANT : NPC_INJURED_PEASANT, x, y, z, o, TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN, 2*MINUTE*IN_MILLISECONDS);
            }
        }

        void JustSummoned(Creature* creature)
        {
            summons.Summon(creature);
            if (creature->GetEntry() == NPC_INJURED_PEASANT || creature->GetEntry() == NPC_PLAGUED_PEASANT)
            {
                creature->SetFaction(_faction);
                if (!_spoken)
                {
                    _spoken = true;
                    creature->AI()->Talk(0);
                }

                if (creature->GetEntry() == NPC_PLAGUED_PEASANT)
                    creature->CastSpell(creature, SPELL_SEETHING_PLAGUE, true);

                float x = 3324+frand(-3.0f, 3.0f);
                float y = -2966+frand(-3.0f, 3.0f);
                float z = 159.65f;
                creature->SetWalk(true);
                creature->GetMotionMaster()->MovePoint(0, x, y, z);
                creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP);
            }
        }

        void DoAction(int32 action)
        {
            if (action == 1)
                _savedCount++;
            else
                _deathCount++;

            if (_savedCount > 49)
            {
                Talk(1);
                if (Player* player = ObjectAccessor::GetPlayer(*me, _playerGUID))
                    player->AreaExploredOrEventHappens(QUEST_BALANCE_OF_LIGHT_AND_SHADOW);
                EnterEvadeMode();
                return;
            }
            else if (_deathCount > 14)
            {
                Talk(2);
                if (Player* player = ObjectAccessor::GetPlayer(*me, _playerGUID))
                    player->FailQuest(QUEST_BALANCE_OF_LIGHT_AND_SHADOW);
                EnterEvadeMode();
                return;
            }

            if (action == 1 && !_spoken)
            {
                _spoken = true;
                Talk(0);
                me->CastSpell(me, SPELL_ERIS_BLESSING, false);
            }
        }

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);
            switch (events.ExecuteEvent())
            {
                case EVENT_CHECK_PLAYER:
                {
                    Player* player = ObjectAccessor::GetPlayer(*me, _playerGUID);
                    if (!player || me->GetDistance2d(player) > 100.0f)
                    {
                        EnterEvadeMode();
                        return;
                    }
                    events.Repeat(2000);
                    break;
                }
                case EVENT_SUMMON_ARCHERS:
                    SummonArchers();
                    events.PopEvent(); 
                    break;
                case EVENT_SUMMON_PEASANTS:
                    _spoken = false;
                    SummonPeasants();
                    _spoken = false;
                    events.Repeat(60*IN_MILLISECONDS);
                    break;
            }
        }
    };
};

class npc_balance_of_light_and_shadow : public CreatureScript
{
public:
    npc_balance_of_light_and_shadow() : CreatureScript("npc_balance_of_light_and_shadow") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_balance_of_light_and_shadowAI (creature);
    }

    struct npc_balance_of_light_and_shadowAI : public ScriptedAI
    {
        npc_balance_of_light_and_shadowAI(Creature* creature) : ScriptedAI(creature) { timer = 0; _targetGUID.Clear(); }

        bool CanBeSeen(Player const* player)
        {
            // requires this trinket to be seen
            return player->HasItemCount(ITEM_EYE_OF_DIVINITY, 1);
        }

        uint32 timer;
        ObjectGuid _targetGUID;

        void SpellHit(Unit*, const SpellInfo* spellInfo)
        {
            if (spellInfo->Id == SPELL_SHOOT && roll_chance_i(7))
                me->CastSpell(me, SPELL_DEATHS_DOOR, true);
        }

        void MovementInform(uint32 type, uint32 pointId)
        {
            if (type != POINT_MOTION_TYPE)
                return;

            if (TempSummon* summon = me->ToTempSummon())
                if (Unit* creature = summon->GetSummoner())
                    creature->GetAI()->DoAction(1);

            me->DespawnOrUnsummon(1);
        }

        void JustDied(Unit*)
        {
            if (TempSummon* summon = me->ToTempSummon())
                if (Unit* creature = summon->GetSummoner())
                    creature->GetAI()->DoAction(2);
        }

        void UpdateAI(uint32 diff)
        {
            if (me->GetEntry() != NPC_SCOURGE_ARCHER)
                return;

            timer += diff;
            if (timer >= 4000)
            {
                Unit* target = _targetGUID ? ObjectAccessor::GetUnit(*me, _targetGUID) : NULL;
                if (!target)
                    target = me->FindNearestCreature(NPC_INJURED_PEASANT, 60.0f);

                if (target)
                {
                    _targetGUID.Set(target->GetGUID());
                    me->CastSpell(target, SPELL_SHOOT, true);
                }

                timer = urand(0,3000);
            }
        }
    };
};


// Theirs
class npc_ghoul_flayer : public CreatureScript
{
public:
    npc_ghoul_flayer() : CreatureScript("npc_ghoul_flayer") { }

    struct npc_ghoul_flayerAI : public ScriptedAI
    {
        npc_ghoul_flayerAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset() { }

        void EnterCombat(Unit* /*who*/) { }

        void JustDied(Unit* killer)
        {
            if (killer->GetTypeId() == TYPEID_PLAYER)
                me->SummonCreature(11064, 0.0f, 0.0f, 0.0f, 0.0f, TEMPSUMMON_TIMED_DESPAWN, 60000);
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_ghoul_flayerAI(creature);
    }
};

/*######
## npc_augustus_the_touched
######*/

class npc_augustus_the_touched : public CreatureScript
{
public:
    npc_augustus_the_touched() : CreatureScript("npc_augustus_the_touched") { }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action)
    {
        player->PlayerTalkClass->ClearMenus();
        if (action == GOSSIP_ACTION_TRADE)
            player->GetSession()->SendListInventory(creature->GetGUID());
        return true;
    }

    bool OnGossipHello(Player* player, Creature* creature)
    {
        if (creature->IsQuestGiver())
            player->PrepareQuestMenu(creature->GetGUID());

        if (creature->IsVendor() && player->GetQuestRewardStatus(6164))
            AddGossipItemFor(player, GOSSIP_ICON_VENDOR, GOSSIP_TEXT_BROWSE_GOODS, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_TRADE);

        SendGossipMenuFor(player, player->GetGossipTextId(creature), creature->GetGUID());
        return true;
    }
};

/*######
## npc_darrowshire_spirit
######*/

enum DarrowshireSpirit
{
    SPELL_SPIRIT_SPAWNIN    = 17321
};

class npc_darrowshire_spirit : public CreatureScript
{
public:
    npc_darrowshire_spirit() : CreatureScript("npc_darrowshire_spirit") { }

    bool OnGossipHello(Player* player, Creature* creature)
    {
        SendGossipMenuFor(player, 3873, creature->GetGUID());
        player->TalkedToCreature(creature->GetEntry(), creature->GetGUID());
        creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
        return true;
    }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_darrowshire_spiritAI(creature);
    }

    struct npc_darrowshire_spiritAI : public ScriptedAI
    {
        npc_darrowshire_spiritAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset()
        {
            DoCast(me, SPELL_SPIRIT_SPAWNIN);
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
        }

        void EnterCombat(Unit* /*who*/) { }
    };
};

/*######
## npc_tirion_fordring
######*/

#define GOSSIP_HELLO    "I am ready to hear your tale, Tirion."
#define GOSSIP_SELECT1  "Thank you, Tirion.  What of your identity?"
#define GOSSIP_SELECT2  "That is terrible."
#define GOSSIP_SELECT3  "I will, Tirion."

class npc_tirion_fordring : public CreatureScript
{
public:
    npc_tirion_fordring() : CreatureScript("npc_tirion_fordring") { }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action)
    {
        player->PlayerTalkClass->ClearMenus();
        switch (action)
        {
            case GOSSIP_ACTION_INFO_DEF+1:
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, GOSSIP_SELECT1, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
                SendGossipMenuFor(player, 4493, creature->GetGUID());
                break;
            case GOSSIP_ACTION_INFO_DEF+2:
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, GOSSIP_SELECT2, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
                SendGossipMenuFor(player, 4494, creature->GetGUID());
                break;
            case GOSSIP_ACTION_INFO_DEF+3:
                AddGossipItemFor(player, GOSSIP_ICON_CHAT, GOSSIP_SELECT3, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 4);
                SendGossipMenuFor(player, 4495, creature->GetGUID());
                break;
            case GOSSIP_ACTION_INFO_DEF+4:
                CloseGossipMenuFor(player);
                player->AreaExploredOrEventHappens(5742);
                break;
        }
        return true;
    }

    bool OnGossipHello(Player* player, Creature* creature)
    {
        if (creature->IsQuestGiver())
            player->PrepareQuestMenu(creature->GetGUID());

        if (player->GetQuestStatus(5742) == QUEST_STATUS_INCOMPLETE && player->GetStandState() == UNIT_STAND_STATE_SIT)
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, GOSSIP_HELLO, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);

        SendGossipMenuFor(player, player->GetGossipTextId(creature), creature->GetGUID());

        return true;
    }
};

void AddSC_eastern_plaguelands()
{
    // Ours
    new npc_eris_hevenfire();
    new npc_balance_of_light_and_shadow();

    // Theirs
    new npc_ghoul_flayer();
    new npc_augustus_the_touched();
    new npc_darrowshire_spirit();
    new npc_tirion_fordring();
}
