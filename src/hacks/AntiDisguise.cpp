/*
 * AntiDisguise.cpp
 *
 *  Created on: Nov 16, 2016
 *      Author: nullifiedcat
 */

#include <settings/Bool.hpp>
#include "common.hpp"

namespace hacks::antidisguise
{
#if ENABLE_TEXTMODE
static settings::Boolean enable{ "remove.disguise", "true" };
#else
static settings::Boolean enable{ "remove.disguise", "true" };
#endif
static settings::Boolean no_invisibility{ "remove.cloak", "true" };

static void CreateMove()
{
    if (!*enable && !*no_invisibility)
        return;

    for (const auto &ent : entity_cache::player_cache)
    {
        if (CE_BAD(ent) || ent == LOCAL_E || ent->m_Type() != ENTITY_PLAYER || CE_INT(ent, netvar.iClass) != tf_class::tf_spy)
            continue;
        if (*enable)
            RemoveCondition<TFCond_Disguised>(ent);
        if (*no_invisibility)
        {
            RemoveCondition<TFCond_Cloaked>(ent);
            RemoveCondition<TFCond_CloakFlicker>(ent);
        }
    }
}

static InitRoutine EC(
    []()
    {
        EC::Register(EC::CreateMove, CreateMove, "antidisguise", EC::average);
        EC::Register(EC::CreateMoveWarp, CreateMove, "antidisguise_w", EC::average);
    });
} // namespace hacks::antidisguise