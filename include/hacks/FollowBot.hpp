/*
 *
 * FollowBot.hpp
 *
 *
 */

#include "entitycache.hpp"

namespace hacks::followbot
{
int ClassPriority(CachedEntity *ent);

bool isEnabled();
bool IsIdle();
extern int follow_target;
} // namespace hacks::followbot
