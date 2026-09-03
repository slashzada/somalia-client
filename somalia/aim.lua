--[[
    Somalia - Legit Aimbot & Silent Aim Module
    Renderização separada e precisa de FOV para Aimbot e Silent Aim
]]

local okImgui, imgui = pcall(require, 'imgui')
local okMemory, memory = pcall(require, 'memory')

local Aim = {
    currentTarget = nil,
    lastFovLogTick = 0,
    lastSilentFovLogTick = 0,
    lastAimLogTick = 0
}

local boneIds = {
    [0] = 8, -- Head
    [1] = 7, -- Neck
    [2] = 3, -- Chest
    [3] = 1  -- Pelvis
}

-- 1. IDENTIFICAÇÃO CENTRALIZADA DA ARMA ATUAL
function Aim.getCurrentWeaponId()
    if isSampLoaded() and (not isSampAvailable() or not sampIsLocalPlayerSpawned() or sampIsDialogActive()) then
        return 0
    end
    if doesCharExist(playerPed) and not isCharDead(playerPed) then
        return getCurrentCharWeapon(playerPed)
    end
    return 0
end

local function getScreenCenter()
    local sw, sh = getScreenResolution()
    return sw / 2, sh / 2
end

local function normalizeAngle(a)
    while a > math.pi do a = a - 2 * math.pi end
    while a < -math.pi do a = a + 2 * math.pi end
    return a
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

-- 2. RENDERIZAÇÃO SEGURA DO CÍRCULO DE FOV (AIMBOT & SILENT AIM INDEPENDENTES)
function Aim.renderFovCircle(config, customDrawList)
    if not okImgui or not config then return end
    
    -- Trava estrita contra execução antes do spawn ou em diálogos/login
    if isSampLoaded() and (not isSampAvailable() or not sampIsLocalPlayerSpawned() or sampIsDialogActive()) then 
        return 
    end
    if not doesCharExist(playerPed) or isCharDead(playerPed) then 
        return 
    end
    
    local wId = Aim.getCurrentWeaponId()
    if wId == 0 then return end
    
    local drawList = customDrawList or (imgui.GetWindowDrawList and imgui.GetWindowDrawList())
    if not drawList then return end
    
    local cx, cy = getScreenCenter()
    local now = os.clock()

    -- 2.1 FOV DO AIMBOT LEGIT (PRESERVADO INTACTO)
    if config.aim and config.aim.globalEnabled ~= false then
        local wAim = getWeaponConfig(config, "aim", wId)
        if wAim and wAim.enabled and wAim.showFov then
            local radiusAim = tonumber(wAim.fovSize) or 60
            local fovColorAim = imgui.ImColor(0, 140, 255, 120):GetU32() -- Azul Neon Aimbot
            
            drawList:AddCircle(imgui.ImVec2(cx, cy), radiusAim, fovColorAim, 36)
            
            if (now - Aim.lastFovLogTick) >= 2.0 then
                Aim.lastFovLogTick = now
                print(string.format("[SOMALIA][FOV] ativo | arma: %d | raio: %.1f px | desenhado", wId, radiusAim))
            end
        end
    end

    -- 2.2 FOV DO SILENT AIM (INDEPENDENTE E COM IDENTIFICAÇÃO VISUAL PRÓPRIA)
    if config.silent and config.silent.globalEnabled ~= false then
        local wSil = getWeaponConfig(config, "silent", wId)
        if wSil and wSil.enabled and wSil.showFov then
            local radiusSil = tonumber(wSil.fovSize) or 70
            local fovColorSil = imgui.ImColor(255, 80, 80, 120):GetU32() -- Vermelho/Laranja Silent FOV
            
            drawList:AddCircle(imgui.ImVec2(cx, cy), radiusSil, fovColorSil, 36)
            
            if (now - Aim.lastSilentFovLogTick) >= 2.0 then
                Aim.lastSilentFovLogTick = now
                print(string.format("[SOMALIA][SILENT FOV] enabled | weapon: %d | radius: %.1f | draw called", wId, radiusSil))
            end
        end
    end
end

-- 3. AQUISIÇÃO DO MELHOR ALVO
function Aim.getBestTarget(wCfg)
    if isSampLoaded() and (not isSampAvailable() or not sampIsLocalPlayerSpawned() or sampIsDialogActive()) then return nil end
    if not doesCharExist(playerPed) or isCharDead(playerPed) or not wCfg then return nil end
    
    local px, py, pz = getCharCoordinates(playerPed)
    local cx, cy = getScreenCenter()
    
    local bestPed = nil
    local minFovDist = tonumber(wCfg.fovSize) or 60
    local maxDist3D = tonumber(wCfg.maxDist) or 150
    local targetBoneId = boneIds[wCfg.hitbox] or 8
    
    for _, handle in ipairs(getAllChars()) do
        if handle ~= playerPed and doesCharExist(handle) and not isCharDead(handle) then
            local isNpc = not isSampLoaded() or not isSampfuncsLoaded() or (sampGetPlayerIdByCharHandle(handle) == nil)
            
            if not isNpc or wCfg.allowNpc then
                local tx, ty, tz = getCharCoordinates(handle)
                local dist3D = getDistanceBetweenCoords3d(px, py, pz, tx, ty, tz)
                
                if dist3D <= maxDist3D then
                    local bx, by, bz = tx, ty, tz + 0.6
                    local ptr = getCharPointer(handle)
                    if ptr and ptr ~= 0 then
                        pcall(function()
                            bx, by, bz = getPedBonePosition(handle, targetBoneId)
                        end)
                    end
                    
                    local res, sx, sy = convert3DCoordsToScreen(bx, by, bz)
                    if res then
                        local fovDist = math.sqrt((sx - cx)^2 + (sy - cy)^2)
                        if fovDist <= minFovDist then
                            local isVisible = true
                            if wCfg.visibleCheck then
                                isVisible = processLineOfSight(px, py, pz + 0.7, bx, by, bz, true, false, false, true, false, false, false, false)
                            end
                            
                            if isVisible then
                                minFovDist = fovDist
                                bestPed = handle
                            end
                        end
                    end
                end
            end
        end
    end
    return bestPed
end

-- 4. PROCESSAMENTO DO AIMBOT (DESATIVADO NESTA ETAPA DE TESTE VISUAL)
function Aim.process(config)
    Aim.currentTarget = nil
    return
end

return Aim
