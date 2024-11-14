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

#include "AuctionHousePackets.h"

void WorldPackets::AuctionHouse::AuctionReplicateItems::Read()
{
    _worldPacket >> Count;
    _worldPacket >> ChangeNumberCursor;
    _worldPacket >> ChangeNumberGlobal;
    _worldPacket >> ChangeNumberTombstone;

    Auctioneer[6] = _worldPacket.ReadBit();
    Auctioneer[0] = _worldPacket.ReadBit();
    Auctioneer[1] = _worldPacket.ReadBit();
    Auctioneer[5] = _worldPacket.ReadBit();
    Auctioneer[3] = _worldPacket.ReadBit();
    Auctioneer[2] = _worldPacket.ReadBit();
    Auctioneer[4] = _worldPacket.ReadBit();
    Auctioneer[7] = _worldPacket.ReadBit();

    _worldPacket.ReadByteSeq(Auctioneer[7]);
    _worldPacket.ReadByteSeq(Auctioneer[0]);
    _worldPacket.ReadByteSeq(Auctioneer[6]);
    _worldPacket.ReadByteSeq(Auctioneer[4]);
    _worldPacket.ReadByteSeq(Auctioneer[1]);
    _worldPacket.ReadByteSeq(Auctioneer[5]);
    _worldPacket.ReadByteSeq(Auctioneer[3]);
    _worldPacket.ReadByteSeq(Auctioneer[2]);
}

WorldPacket const* WorldPackets::AuctionHouse::AuctionReplicateResponse::Write()
{
    ByteBuffer dataBuffer;

    _worldPacket << uint32(Result);
    _worldPacket << uint32(ChangeNumberGlobal);
    _worldPacket << uint32(DesiredDelay);
    _worldPacket << uint32(ChangeNumberTombstone);
    _worldPacket << uint32(ChangeNumberCursor);

    _worldPacket.WriteBits(Items.size(), 17);
    for (auto const& item : Items)
    {
        ObjectGuid guid = item.ItemGuid;
        ObjectGuid owner = item.Owner;
        ObjectGuid bidder = item.Bidder;

        _worldPacket.WriteBit(bidder[3]);
        _worldPacket.WriteBit(bidder[1]);
        _worldPacket.WriteBit(guid[0]);
        _worldPacket.WriteBit(owner[1]);
        _worldPacket.WriteBit(owner[6]);
        _worldPacket.WriteBit(bidder[5]);
        _worldPacket.WriteBit(owner[4]);
        _worldPacket.WriteBit(bidder[7]);
        _worldPacket.WriteBit(owner[7]);
        _worldPacket.WriteBit(guid[2]);
        _worldPacket.WriteBit(guid[1]);
        _worldPacket.WriteBit(owner[5]);
        _worldPacket.WriteBit(owner[2]);
        _worldPacket.WriteBit(owner[3]);
        _worldPacket.WriteBit(bidder[4]);
        _worldPacket.WriteBit(guid[7]);
        _worldPacket.WriteBit(guid[5]);
        _worldPacket.WriteBit(bidder[0]);
        _worldPacket.WriteBit(owner[0]);
        _worldPacket.WriteBit(guid[3]);
        _worldPacket.WriteBits(item.Enchantments.size(), 20);
        _worldPacket.WriteBit(bidder[2]);
        _worldPacket.WriteBit(bidder[6]);
        _worldPacket.WriteBit(guid[6]);
        _worldPacket.WriteBit(guid[4]);

        dataBuffer << uint32(0);
        dataBuffer.WriteByteSeq(bidder[5]);
        dataBuffer.WriteByteSeq(bidder[1]);
        dataBuffer.WriteByteSeq(guid[1]);
        dataBuffer.WriteByteSeq(owner[4]);
        dataBuffer.WriteByteSeq(owner[0]);
        dataBuffer.WriteByteSeq(guid[5]);

        dataBuffer << uint32(item.Item.RandomPropertiesSeed); // *
        dataBuffer << uint32(item.DurationLeft);
        for (auto const& enchant : item.Enchantments)
        {
            dataBuffer << uint32(enchant.Expiration);
            dataBuffer << int32(enchant.Charges);
            dataBuffer << uint8(enchant.Slot);
            dataBuffer << int32(enchant.ID);
        }
        dataBuffer.WriteByteSeq(owner[5]);
        dataBuffer.WriteByteSeq(bidder[4]);
        dataBuffer.WriteByteSeq(owner[6]);

        dataBuffer << uint32(item.Item.RandomPropertiesID);
        dataBuffer << uint64(item.BuyoutPrice);
        dataBuffer << uint32(item.AuctionItemID); // *
        dataBuffer.WriteByteSeq(owner[2]);
        dataBuffer.WriteByteSeq(guid[2]);
        dataBuffer.WriteByteSeq(guid[0]);
        dataBuffer.WriteByteSeq(bidder[7]);
        dataBuffer.WriteByteSeq(bidder[2]);

        dataBuffer << uint32(0); // **

        dataBuffer.WriteByteSeq(owner[7]);
        dataBuffer.WriteByteSeq(bidder[0]);

        dataBuffer << uint32(item.Count);
        dataBuffer << uint64(item.BidAmount);
        dataBuffer.WriteByteSeq(bidder[3]);
        dataBuffer.WriteByteSeq(guid[4]);
        dataBuffer.WriteByteSeq(bidder[6]);

        dataBuffer << uint32(item.EndTime); // *
        dataBuffer.WriteByteSeq(guid[7]);
        dataBuffer.WriteByteSeq(guid[6]);

        dataBuffer << uint64(item.MinIncrement);
        dataBuffer.WriteByteSeq(owner[1]);

        dataBuffer << uint64(item.MinBid);
        dataBuffer.WriteByteSeq(guid[3]);
        dataBuffer.WriteByteSeq(owner[3]);

        dataBuffer << uint32(item.Charges);
        dataBuffer << uint32(item.Item.ItemID);
        dataBuffer << uint32(item.Flags); // *
    }

    _worldPacket.append(dataBuffer);
    // *  not verified (not exposed to players/addons; client internal use)
    // ** probably a mask (temporarily zeroed out)

    return &_worldPacket;
}