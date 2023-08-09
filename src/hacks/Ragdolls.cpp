/*
 * Ragdolls.cpp
 *
 *  Created on: Nov 6, 2019
 *      Author: Roboot
 */
#include "common.hpp"
#include "sdk/dt_recv_redef.h"
#include "Ragdolls.hpp"

namespace hacks::ragdolls
{
static settings::Int mode{ "visual.ragdoll-mode", "0" };
static settings::Boolean only_local{ "visual.ragdoll-only-local", "0" };

// Ragdoll override style
enum RagdollOverride_t
{
    NONE         = 0,
    GIB          = 1,
};
ProxyFnHook gib_hook;

bool ragdollKillByLocal(void *ragdoll)
{
    // Get the owner of the ragdoll (TFPlayer)
    auto owner = g_IEntityList->GetClientEntity(NET_INT(ragdoll, netvar.m_iPlayerIndex));
    if (!owner || owner->IsDormant() || owner->entindex() == g_pLocalPlayer->entity_idx)
        return false;

    // Check to see if the owner is spectating the local player
    auto owner_observer = g_IEntityList->GetClientEntityFromHandle(NET_VAR(owner, netvar.hObserverTarget, CBaseHandle));
    return owner_observer && !owner_observer->IsDormant() && owner_observer->entindex() == g_pLocalPlayer->entity_idx;
}
void overrideGib(const CRecvProxyData *data, void *structure, void *out)
{
    auto gib = reinterpret_cast<bool *>(out);
    if (*mode == RagdollOverride_t::GIB && (!*only_local || ragdollKillByLocal(structure)))
        *gib = true;
    else
        *gib = data->m_Value.m_Int;
}
 void hook()
{
    for (auto dt_class = g_IBaseClient->GetAllClasses(); dt_class; dt_class = dt_class->m_pNext)
    {
        auto table = dt_class->m_pRecvTable;
        if (strcmp(table->m_pNetTableName, "DT_TFRagdoll") != 0)
            continue;

        for (int i = 0; i < table->m_nProps; ++i)
        {
            auto prop = reinterpret_cast<RecvPropRedef *>(&table->m_pProps[i]);
            if (!prop || strcmp(prop->m_pVarName, "m_bGib") != 0)
                continue;

            gib_hook.init(prop);
            gib_hook.setHook(overrideGib);
        }
    }
} 
/**
 * Restore the RecvVarProxyFns that were swapped out earlier
 */
void unhook()
{
    gib_hook.restore();
}

static InitRoutine init(
    []()
    {
        hook();
        EC::Register(EC::Shutdown, unhook, "ragdoll_shutdown");
    });

} // namespace hacks::ragdolls
