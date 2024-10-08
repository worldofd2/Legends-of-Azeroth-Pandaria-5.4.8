/*
* This file is part of the Pandaria 5.4.8 Project. See THANKS file for Copyright information
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

#include "ScriptMgr.h"
#include "InstanceScript.h"
#include "ruby_sanctum.h"
#include "Player.h"
#include "TemporarySummon.h"
#include "WorldPacket.h"

static std::vector<DoorData> const doorData =
{
    {GO_FIRE_FIELD,     DATA_BALTHARUS_THE_WARBORN, DOOR_TYPE_PASSAGE,  BOUNDARY_E   },
};

class instance_ruby_sanctum : public InstanceMapScript
{
    public:
        instance_ruby_sanctum() : InstanceMapScript(RSScriptName, 724) { }

        struct instance_ruby_sanctum_InstanceMapScript : public InstanceScript
        {
            instance_ruby_sanctum_InstanceMapScript(InstanceMap* map) : InstanceScript(map)
            {
                SetBossNumber(EncounterCount);
                LoadDoorData(doorData);
                BaltharusTheWarbornGUID = ObjectGuid::Empty;
                GeneralZarithrianGUID = ObjectGuid::Empty;
                SavianaRagefireGUID = ObjectGuid::Empty;
                HalionGUID = ObjectGuid::Empty;
                TwilightHalionGUID = ObjectGuid::Empty;
                OrbCarrierGUID = ObjectGuid::Empty;
                OrbRotationFocusGUID = ObjectGuid::Empty;
                HalionControllerGUID = ObjectGuid::Empty;
                CrystalChannelTargetGUID = ObjectGuid::Empty;
                XerestraszaGUID = ObjectGuid::Empty;
                BaltharusSharedHealth    = 0;
                FlameWallsGUID = ObjectGuid::Empty;
                FlameRingGUID = ObjectGuid::Empty;

                memset(ZarithrianSpawnStalkerGUID, 0, 2 * sizeof(uint64));
                memset(BurningTreeGUID, 0, 4 * sizeof(uint64));
                instance->SetWorldState(instance->Is25ManRaid() ? WORLDSTATE_TWILIGHT_DESTROYER_25 : WORLDSTATE_TWILIGHT_DESTROYER_10, 1);
            }

            void OnPlayerEnter(Player* /*player*/) override
            {
                if (!GetData64(DATA_HALION_CONTROLLER) && GetBossState(DATA_HALION) != DONE && GetBossState(DATA_GENERAL_ZARITHRIAN) == DONE)
                {
                    instance->LoadGrid(HalionControllerSpawnPos.GetPositionX(), HalionControllerSpawnPos.GetPositionY());
                    if (Creature* halionController = instance->SummonCreature(NPC_HALION_CONTROLLER, HalionControllerSpawnPos))
                        halionController->AI()->DoAction(ACTION_INTRO_HALION);
                }
            }

            void OnCreatureCreate(Creature* creature) override
            {
                switch (creature->GetEntry())
                {
                    case NPC_BALTHARUS_THE_WARBORN:
                        BaltharusTheWarbornGUID = creature->GetGUID();
                        break;
                    case NPC_GENERAL_ZARITHRIAN:
                        GeneralZarithrianGUID = creature->GetGUID();
                        break;
                    case NPC_SAVIANA_RAGEFIRE:
                        SavianaRagefireGUID = creature->GetGUID();
                        break;
                    case NPC_HALION:
                        HalionGUID = creature->GetGUID();
                        break;
                    case NPC_TWILIGHT_HALION:
                        TwilightHalionGUID = creature->GetGUID();
                        break;
                    case NPC_HALION_CONTROLLER:
                        HalionControllerGUID = creature->GetGUID();
                        break;
                    case NPC_ORB_CARRIER:
                        OrbCarrierGUID = creature->GetGUID();
                        break;
                    case NPC_ORB_ROTATION_FOCUS:
                        OrbRotationFocusGUID = creature->GetGUID();
                        break;
                    case NPC_BALTHARUS_TARGET:
                        CrystalChannelTargetGUID = creature->GetGUID();
                        break;
                    case NPC_XERESTRASZA:
                        XerestraszaGUID = creature->GetGUID();
                        break;
                    case NPC_ZARITHRIAN_SPAWN_STALKER:
                        if (!ZarithrianSpawnStalkerGUID[0])
                            ZarithrianSpawnStalkerGUID[0] = creature->GetGUID();
                        else
                            ZarithrianSpawnStalkerGUID[1] = creature->GetGUID();
                        break;
                    default:
                        break;
                }
            }

            void OnGameObjectCreate(GameObject* go) override
            {
                switch (go->GetEntry())
                {
                    case GO_FIRE_FIELD:
                        AddDoor(go, true);
                        break;
                    case GO_FLAME_WALLS:
                        FlameWallsGUID = go->GetGUID();
                        if (GetBossState(DATA_SAVIANA_RAGEFIRE) == DONE && GetBossState(DATA_BALTHARUS_THE_WARBORN) == DONE)
                            HandleGameObject(FlameWallsGUID, true, go);
                        break;
                    case GO_FLAME_RING:
                        FlameRingGUID = go->GetGUID();
                        break;
                    case GO_TWILIGHT_FLAME_RING:
                        TwilightFlameRingGUID = go->GetGUID();
                        break;
                    case GO_BURNING_TREE_1:
                        BurningTreeGUID[0] = go->GetGUID();
                        if (GetBossState(DATA_GENERAL_ZARITHRIAN) == DONE)
                            HandleGameObject(BurningTreeGUID[0], true);
                        break;
                    case GO_BURNING_TREE_2:
                        BurningTreeGUID[1] = go->GetGUID();
                        if (GetBossState(DATA_GENERAL_ZARITHRIAN) == DONE)
                            HandleGameObject(BurningTreeGUID[1], true);
                        break;
                    case GO_BURNING_TREE_3:
                        BurningTreeGUID[2] = go->GetGUID();
                        if (GetBossState(DATA_GENERAL_ZARITHRIAN) == DONE)
                            HandleGameObject(BurningTreeGUID[2], true);
                        break;
                    case GO_BURNING_TREE_4:
                        BurningTreeGUID[3] = go->GetGUID();
                        if (GetBossState(DATA_GENERAL_ZARITHRIAN) == DONE)
                            HandleGameObject(BurningTreeGUID[3], true);
                        break;
                    default:
                        break;
                }
            }

            void OnUnitDeath(Unit* unit) override
            {
                Creature* creature = unit->ToCreature();
                if (!creature)
                    return;

                if (creature->GetEntry() == NPC_GENERAL_ZARITHRIAN && GetBossState(DATA_HALION) != DONE)
                {
                    instance->LoadGrid(HalionControllerSpawnPos.GetPositionX(), HalionControllerSpawnPos.GetPositionY());
                    if (Creature* halionController = instance->SummonCreature(NPC_HALION_CONTROLLER, HalionControllerSpawnPos))
                        halionController->AI()->DoAction(ACTION_INTRO_HALION);
                }
            }

            ObjectGuid GetGuidData(uint32 type) const override
            {
                switch (type)
                {
                    case DATA_BALTHARUS_THE_WARBORN:
                        return BaltharusTheWarbornGUID;
                    case DATA_CRYSTAL_CHANNEL_TARGET:
                        return CrystalChannelTargetGUID;
                    case DATA_XERESTRASZA:
                        return XerestraszaGUID;
                    case DATA_SAVIANA_RAGEFIRE:
                        return SavianaRagefireGUID;
                    case DATA_GENERAL_ZARITHRIAN:
                        return GeneralZarithrianGUID;
                    case DATA_ZARITHRIAN_SPAWN_STALKER_1:
                    case DATA_ZARITHRIAN_SPAWN_STALKER_2:
                        return ZarithrianSpawnStalkerGUID[type - DATA_ZARITHRIAN_SPAWN_STALKER_1];
                    case DATA_HALION:
                        return HalionGUID;
                    case DATA_TWILIGHT_HALION:
                        return TwilightHalionGUID;
                    case DATA_ORB_CARRIER:
                        return OrbCarrierGUID;
                    case DATA_ORB_ROTATION_FOCUS:
                        return OrbRotationFocusGUID;
                    case DATA_HALION_CONTROLLER:
                        return HalionControllerGUID;
                    case DATA_BURNING_TREE_1:
                    case DATA_BURNING_TREE_2:
                    case DATA_BURNING_TREE_3:
                    case DATA_BURNING_TREE_4:
                        return BurningTreeGUID[type - DATA_BURNING_TREE_1];
                    case DATA_FLAME_RING:
                        return FlameRingGUID;
                    case DATA_TWILIGHT_FLAME_RING:
                        return TwilightFlameRingGUID;
                    default:
                        break;
                }

                return ObjectGuid::Empty;
            }

            bool SetBossState(uint32 type, EncounterState state) override
            {
                if (!InstanceScript::SetBossState(type, state))
                    return false;

                switch (type)
                {
                    case DATA_BALTHARUS_THE_WARBORN:
                    {
                        if (state == DONE && GetBossState(DATA_SAVIANA_RAGEFIRE) == DONE)
                        {
                            HandleGameObject(FlameWallsGUID, true);
                            if (Creature* zarithrian = instance->GetCreature(GeneralZarithrianGUID))
                                zarithrian->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_NOT_SELECTABLE);
                        }
                        break;
                    }
                    case DATA_SAVIANA_RAGEFIRE:
                    {
                        if (state == DONE && GetBossState(DATA_BALTHARUS_THE_WARBORN) == DONE)
                        {
                            HandleGameObject(FlameWallsGUID, true);
                            if (Creature* zarithrian = instance->GetCreature(GeneralZarithrianGUID))
                                zarithrian->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_NOT_SELECTABLE);
                        }
                        break;
                    }
                    case DATA_GENERAL_ZARITHRIAN:
                    {
                        if (GetBossState(DATA_SAVIANA_RAGEFIRE) == DONE && GetBossState(DATA_BALTHARUS_THE_WARBORN) == DONE)
                            HandleGameObject(FlameWallsGUID, state != IN_PROGRESS);
                        break;
                    }
                    case DATA_HALION:
                    {
                        DoUpdateWorldState(WORLDSTATE_CORPOREALITY_TOGGLE, 0);
                        DoUpdateWorldState(WORLDSTATE_CORPOREALITY_TWILIGHT, 0);
                        DoUpdateWorldState(WORLDSTATE_CORPOREALITY_MATERIAL, 0);

                        // Reopen rings on wipe or success
                        if (state == DONE || state == FAIL)
                        {
                            HandleGameObject(FlameRingGUID, true);
                            HandleGameObject(TwilightFlameRingGUID, true);
                        }
                        break;
                    }
                    default:
                        break;
                }

                return true;
            }

            void SetData(uint32 type, uint32 data) override
            {
                if (type != DATA_BALTHARUS_SHARED_HEALTH)
                    return;

                BaltharusSharedHealth = data;
            }

            uint32 GetData(uint32 type) const override
            {
                if (type != DATA_BALTHARUS_SHARED_HEALTH)
                    return 0;

                return BaltharusSharedHealth;
            }

            std::string GetSaveData() override
            {
                OUT_SAVE_INST_DATA;

                std::ostringstream saveStream;
                saveStream << "R S " << GetBossSaveData();

                OUT_SAVE_INST_DATA_COMPLETE;
                return saveStream.str();
            }

            void FillInitialWorldStates(WorldStateBuilder& builder) override
            {
                builder.AppendState(WORLDSTATE_CORPOREALITY_MATERIAL, 50);
                builder.AppendState(WORLDSTATE_CORPOREALITY_TWILIGHT, 50);
                builder.AppendState(WORLDSTATE_CORPOREALITY_TOGGLE, 0);
            }

            void Load(char const* str) override
            {
                if (!str)
                {
                    OUT_LOAD_INST_DATA_FAIL;
                    return;
                }

                OUT_LOAD_INST_DATA(str);

                char dataHead1, dataHead2;

                std::istringstream loadStream(str);
                loadStream >> dataHead1 >> dataHead2;

                if (dataHead1 == 'R' && dataHead2 == 'S')
                {
                    for (uint8 i = 0; i < EncounterCount; ++i)
                    {
                        uint32 tmpState;
                        loadStream >> tmpState;
                        if (tmpState == IN_PROGRESS || tmpState > SPECIAL)
                            tmpState = NOT_STARTED;

                        SetBossState(i, EncounterState(tmpState));
                    }
                }
                else
                    OUT_LOAD_INST_DATA_FAIL;

                OUT_LOAD_INST_DATA_COMPLETE;
            }

        protected:
            ObjectGuid BaltharusTheWarbornGUID;
            ObjectGuid GeneralZarithrianGUID;
            ObjectGuid SavianaRagefireGUID;
            ObjectGuid HalionGUID;
            ObjectGuid TwilightHalionGUID;
            ObjectGuid HalionControllerGUID;
            ObjectGuid OrbCarrierGUID;
            ObjectGuid OrbRotationFocusGUID;
            ObjectGuid CrystalChannelTargetGUID;
            ObjectGuid XerestraszaGUID;
            ObjectGuid FlameWallsGUID;
            ObjectGuid ZarithrianSpawnStalkerGUID[2];
            ObjectGuid BurningTreeGUID[4];
            ObjectGuid FlameRingGUID;
            ObjectGuid TwilightFlameRingGUID;

            uint32 BaltharusSharedHealth;
        };

        InstanceScript* GetInstanceScript(InstanceMap* map) const override
        {
            return new instance_ruby_sanctum_InstanceMapScript(map);
        }
};

void AddSC_instance_ruby_sanctum()
{
    new instance_ruby_sanctum();
}
