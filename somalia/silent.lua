--[[
    Somalia - Dedicated Silent Aim Module
    Renderização independente e precisa de FOV e utilitários para Silent Aim
]]

local okImgui, imgui = pcall(require, 'imgui')

local Silent = {
    lastSilentFovLogTick = 0
}

local function getScreenCenter()
    local sw, sh = getScreenResolution()
    return sw / 2, sh / 2
end

local function getWeaponConfig(config, prefix, wId)
    if not config then return nil end
    local key = prefix .. "_w_" .. tostring(wId)
    if type(config[key]) == 'table' then
        return config[key]
    end
    if config[prefix] and type(config[prefix].weapons) == 'table' then
        return config[prefix].weapons[wId] or config[prefix].weapons[tostring(wId)]
    end
    return nil
end

function Silent.getCurrentWeaponId()
    if isSampLoaded() and (not isSampAvailable() or not sampIsLocalPlayerSpawned() or sampIsDialogActive()) then
        return 0
    end
    if doesCharExist(playerPed) and not isCharDead(playerPed) then
        return getCurrentCharWeapon(playerPed)
    end
    return 0
end

-- RENDERIZAÇÃO INDEPENDENTE DO CÍRCULO DE FOV DO SILENT AIM
function Silent.renderFovCircle(config, customDrawList)
    if not okImgui or not config then return end
    
    if isSampLoaded() and (not isSampAvailable() or not sampIsLocalPlayerSpawned() or sampIsDialogActive()) then 
        return 
    end
    if not doesCharExist(playerPed) or isCharDead(playerPed) then 
        return 
    end
    
    local wId = Silent.getCurrentWeaponId()
    if wId == 0 then return end
    
    local drawList = customDrawList or (imgui.GetWindowDrawList and imgui.GetWindowDrawList())
    if not drawList then return end
    
    local cx, cy = getScreenCenter()
    local now = os.clock()

    if config.silent and config.silent.globalEnabled ~= false then
        local wSil = getWeaponConfig(config, "silent", wId)
        if wSil and wSil.enabled and wSil.showFov then
            local radiusSil = tonumber(wSil.fovSize) or 70
            local fovColorSil = imgui.ImColor(255, 80, 80, 120):GetU32() -- Vermelho/Laranja Silent FOV
            
            drawList:AddCircle(imgui.ImVec2(cx, cy), radiusSil, fovColorSil, 36)
            
            if (now - Silent.lastSilentFovLogTick) >= 2.0 then
                Silent.lastSilentFovLogTick = now
                print(string.format("[SOMALIA][SILENT FOV] enabled | weapon: %d | radius: %.1f | draw called", wId, radiusSil))
            end
        end
    end
end

return Silent
