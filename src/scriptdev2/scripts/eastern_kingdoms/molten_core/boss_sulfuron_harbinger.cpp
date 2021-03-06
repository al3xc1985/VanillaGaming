﻿/* This file is part of the ScriptDev2 Project. See AUTHORS file for Copyright information
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* ScriptData
SDName: Boss_Sulfuron_Harbringer
SD%Complete: 100
SDComment:
SDCategory: 熔火之心
EndScriptData */

#include "precompiled.h"
#include "molten_core.h"

enum
{
    SPELL_DEMORALIZING_SHOUT    = 19778,                    // 挫志怒吼
    SPELL_INSPIRE               = 19779,                    // 灵感
    SPELL_HAND_OF_RAGNAROS      = 19780,                    // 拉格纳罗斯之手
    SPELL_FLAMESPEAR            = 19781,                    // 烈焰之矛

    // 火妖祭祀
    SPELL_HEAL                  = 19775,                    // 黑暗治疗
    SPELL_SHADOWWORD_PAIN       = 19776,                    // 暗言术：痛
    SPELL_DARK_STRIKE           = 19777,                    // 黑暗打击
    SPELL_IMMOLATE              = 20294                     // 献祭
};

struct boss_sulfuronAI : public ScriptedAI
{
    boss_sulfuronAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        Reset();
    }

    ScriptedInstance* m_pInstance;

    uint32 m_uiDemoralizingShoutTimer;
    uint32 m_uiInspireTimer;
    uint32 m_uiKnockdownTimer;
    uint32 m_uiFlamespearTimer;

    void Reset() override
    {
        m_uiDemoralizingShoutTimer  = 15000;
        m_uiInspireTimer            = 3000;
        m_uiKnockdownTimer          = 6000;
        m_uiFlamespearTimer         = 2000;
    }

    void Aggro(Unit* /*pWho*/) override
    {
        if (m_pInstance)
            m_pInstance->SetData(TYPE_SULFURON, IN_PROGRESS);
    }

    void JustDied(Unit* /*pKiller*/) override
    {
        if (m_pInstance)
            m_pInstance->SetData(TYPE_SULFURON, DONE);
    }

    void JustReachedHome() override
    {
        if (m_pInstance)
            m_pInstance->SetData(TYPE_SULFURON, FAIL);
    }

    void UpdateAI(const uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
            return;

        // 挫志怒吼
        if (m_uiDemoralizingShoutTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_DEMORALIZING_SHOUT) == CAST_OK)
                m_uiDemoralizingShoutTimer = urand(15000, 20000);
        }
        else
            m_uiDemoralizingShoutTimer -= uiDiff;

        // 灵感
        if (m_uiInspireTimer < uiDiff)
        {
            Creature* pTarget = NULL;
            std::list<Creature*> pList = DoFindFriendlyMissingBuff(45.0f, SPELL_INSPIRE);
            if (!pList.empty())
            {
                std::list<Creature*>::iterator i = pList.begin();
                advance(i, (rand() % pList.size()));
                pTarget = (*i);
            }

            if (!pTarget)
                pTarget = m_creature;

            if (DoCastSpellIfCan(pTarget, SPELL_INSPIRE) == CAST_OK)
                m_uiInspireTimer = 10000;
        }
        else
            m_uiInspireTimer -= uiDiff;

        // 拉格纳罗斯之手
        if (m_uiKnockdownTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_HAND_OF_RAGNAROS) == CAST_OK)
                m_uiKnockdownTimer = urand(12000, 15000);
        }
        else
            m_uiKnockdownTimer -= uiDiff;

        // 烈焰之矛
        if (m_uiFlamespearTimer < uiDiff)
        {
            if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0))
            {
                if (DoCastSpellIfCan(pTarget, SPELL_FLAMESPEAR) == CAST_OK)
                    m_uiFlamespearTimer = urand(12000, 16000);
            }
        }
        else
            m_uiFlamespearTimer -= uiDiff;

        DoMeleeAttackIfReady();
    }
};

struct mob_flamewaker_priestAI : public ScriptedAI
{
    mob_flamewaker_priestAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        m_pInstance = (ScriptedInstance*)pCreature->GetInstanceData();
        Reset();
    }

    uint32 m_uiHealTimer;
    uint32 m_uiShadowWordPainTimer;
    uint32 m_uiDarkstrikeTimer;
    uint32 m_uiImmolateTimer;

    ScriptedInstance* m_pInstance;

    void Reset() override
    {
        m_uiHealTimer               = urand(15000, 30000);
        m_uiShadowWordPainTimer     = 2000;
        m_uiDarkstrikeTimer         = 10000;
        m_uiImmolateTimer           = 8000;
    }

    void UpdateAI(const uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->getVictim())
        {
            // 主人进入战斗自己也进入战斗
            if (Creature* pSulfuron = m_pInstance->GetSingleCreatureFromStorage(NPC_SULFURON))
            {
                if (pSulfuron->isInCombat())
                {
                    if (Unit* pTarget = pSulfuron->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0))
                    {
                        if (pTarget->GetTypeId() == TYPEID_PLAYER)
                            { m_creature->AttackedBy(pTarget); }
                    }
                }
            }
            return;
        }

        // 黑暗治疗
        if (m_uiHealTimer < uiDiff)
        {
            if (Unit* pUnit = DoSelectLowestHpFriendly(60.0f, 1))
            {
                if (DoCastSpellIfCan(pUnit, SPELL_HEAL) == CAST_OK)
                    m_uiHealTimer = urand(15000, 20000);
            }
        }
        else
            m_uiHealTimer -= uiDiff;

        // 暗言术：痛
        if (m_uiShadowWordPainTimer < uiDiff)
        {
            if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0))
            {
                if (DoCastSpellIfCan(pTarget, SPELL_SHADOWWORD_PAIN) == CAST_OK)
                    m_uiShadowWordPainTimer = urand(18000, 26000);
            }
        }
        else
            m_uiShadowWordPainTimer -= uiDiff;

        // 黑暗打击
        if (m_uiDarkstrikeTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature->getVictim(), SPELL_DARK_STRIKE) == CAST_OK)
                { m_uiDarkstrikeTimer = urand(15000, 18000); }
        }
        else
            { m_uiDarkstrikeTimer -= uiDiff; }

        // 献祭
        if (m_uiImmolateTimer < uiDiff)
        {
            if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0))
            {
                if (DoCastSpellIfCan(pTarget, SPELL_IMMOLATE) == CAST_OK)
                    m_uiImmolateTimer = urand(15000, 25000);
            }
        }
        else
            m_uiImmolateTimer -= uiDiff;

        // 自己进入战斗主人也进入战斗
        if (Creature* pSulfuron = m_pInstance->GetSingleCreatureFromStorage(NPC_SULFURON))
        {
            if (pSulfuron->isAlive() && !pSulfuron->isInCombat())
            {
                if (Unit* pTarget = m_creature->SelectAttackingTarget(ATTACKING_TARGET_RANDOM, 0))
                {
                    if (pTarget->GetTypeId() == TYPEID_PLAYER)
                        { pSulfuron->AttackedBy(pTarget); }
                }
            }
        }

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_sulfuron(Creature* pCreature)
{
    return new boss_sulfuronAI(pCreature);
}

CreatureAI* GetAI_mob_flamewaker_priest(Creature* pCreature)
{
    return new mob_flamewaker_priestAI(pCreature);
}

void AddSC_boss_sulfuron()
{
    Script* pNewScript;

    pNewScript = new Script;
    pNewScript->Name = "boss_sulfuron";
    pNewScript->GetAI = &GetAI_boss_sulfuron;
    pNewScript->RegisterSelf();

    pNewScript = new Script;
    pNewScript->Name = "mob_flamewaker_priest";
    pNewScript->GetAI = &GetAI_mob_flamewaker_priest;
    pNewScript->RegisterSelf();
}
