--[[
    Somalia - World Module
    Controle em tempo real de Horário e Clima com trava persistente
]]

local World = {}

function World.process(config)
    if not config or not config.world then return end
    
    -- 1. CUSTOM TIME
    if config.world.customTimeEnabled then
        local hour = config.world.customHour or 12
        local minute = config.world.customMinute or 0
        pcall(function()
            setTimeOfDay(hour, minute)
        end)
    end
    
    -- 2. CUSTOM WEATHER
    if config.world.customWeatherEnabled then
        local wId = config.world.weatherId or 1
        pcall(function()
            forceWeatherNow(wId)
        end)
    end
end

return World
