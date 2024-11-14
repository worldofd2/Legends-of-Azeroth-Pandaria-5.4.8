/*
 * This file is part of the Legends of Azeroth Pandaria Project. See THANKS file for Copyright information
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

#include "ItemPacketsCommon.h"
#include "Player.h"

void WorldPackets::Item::ItemInstance::Initialize(::Item const* item)
{
    ItemID               = item->GetEntry();
    RandomPropertiesSeed = item->GetItemSuffixFactor();
    RandomPropertiesID   = item->GetItemRandomPropertyId();
    Mask                 = item->GetUInt32Value(ITEM_FIELD_MODIFIERS_MASK);

    if (Mask)
        for (uint32 i = 0; i < ITEM_MODIFIER_INDEX_MAX; ++i)
            if (Mask & (1 << i))
                Modifications.emplace_back(item->GetDynamicUInt32Value(ITEM_DYNAMIC_MODIFIERS, i));
}
