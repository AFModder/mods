local MOD_TAG = "[CannonFacilityBoost]"

local function log(msg)
    print(string.format("%s %s\n", MOD_TAG, msg))
end

local gen_var = nil

NotifyOnNewObject("/Script/BeltTD.CannonFacilityComponent", function(cannon_obj)
    if not cannon_obj or not cannon_obj.IsValid or not cannon_obj:IsValid() then
        return
    end

    ExecuteInGameThread(function()
        if not gen_var or not gen_var.IsValid or not gen_var:IsValid() then
            gen_var = StaticFindObject("/Game/Blueprints/Buildings/BP_Cannon.BP_Cannon_C:CannonFacility_GEN_VARIABLE")
            gen_var.CatapultMaxDistance = 99999.0
            gen_var.CatapultMinDistance = 0.1
            gen_var.SplineNumPoints = 6
            gen_var.ObstacleTraceNum = 0
            log("patched cannon facility defaults (gen_var)")
        end
    end)
end)