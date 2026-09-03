--[[
    Somalia - Vehicles Module
    Recursos para veículos: Speed Boost, Instant Repair, Flip Car, Godmode e Engine Always On
]]

local Vehicles = {
    lastRepairPress = 0,
    lastFlipPress = 0
}

function Vehicles.process(config)
    if not config or not config.vehicles then return end
    if isCharDead(playerPed) or not isCharInAnyCar(playerPed) then return end
    
    local car = storeCarCharIsInNoSave(playerPed)
    if not car or car == 0 then return end
    
    -- 1. VEHICLE GODMODE
    if config.vehicles.godmode then
        pcall(function()
            setCarProofs(car, true, true, true, true, true)
        end)
    end
    
    -- 2. ENGINE ALWAYS ON
    if config.vehicles.engineAlwaysOn then
        pcall(function()
            setCarEngineOn(car, true)
        end)
    end
    
    -- 3. SPEED BOOST
    if config.vehicles.speedBoost and not sampIsChatInputActive() and not sampIsDialogActive() then
        local boostKey = config.vehicles.speedBoostKey or 0x10 -- Shift
        if isKeyDown(boostKey) then
            pcall(function()
                local mult = config.vehicles.speedMultiplier or 1.3
                local vx, vy, vz = getCarSpeedVector(car)
                setCarSpeedVector(car, vx * mult, vy * mult, vz)
            end)
        end
    end
    
    -- 4. INSTANT REPAIR (KEYBIND)
    if config.vehicles.instantRepair and not sampIsChatInputActive() and not sampIsDialogActive() then
        local repairKey = config.vehicles.instantRepairKey or 0x52 -- R
        local now = os.clock()
        if isKeyDown(repairKey) and (now - Vehicles.lastRepairPress) > 0.8 then
            Vehicles.lastRepairPress = now
            pcall(function()
                fixCar(car)
                setCarHealth(car, 1000)
            end)
        end
    end
    
    -- 5. FLIP CAR (KEYBIND)
    if config.vehicles.flipCar and not sampIsChatInputActive() and not sampIsDialogActive() then
        local flipKey = config.vehicles.flipCarKey or 0x58 -- X
        local now = os.clock()
        if isKeyDown(flipKey) and (now - Vehicles.lastFlipPress) > 0.8 then
            Vehicles.lastFlipPress = now
            pcall(function()
                local heading = getCarHeading(car)
                setCarHeading(car, heading)
                local cx, cy, cz = getCarCoordinates(car)
                setCarCoordinates(car, cx, cy, cz + 0.6)
            end)
        end
    end
end

return Vehicles
