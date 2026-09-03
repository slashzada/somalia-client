--[[
    Somalia - Visuals & ESP Module
    Renderização DirectX com cálculo robusto de 3D->2D, suporte a múltiplos peds e teste visual imediato
]]

local okImgui, imgui = pcall(require, 'imgui')

local Visuals = {
    lastLogTick = 0
}

-- Mapa dos ossos do esqueleto GTA SA
local skeletonPairs = {
    {8, 7},   -- Cabeça -> Pescoço
    {7, 3},   -- Pescoço -> Tronco
    {3, 1},   -- Tronco -> Pélvis
    {7, 5},   -- Pescoço -> Ombro Esq
    {5, 4},   -- Ombro Esq -> Cotovelo Esq
    {4, 34},  -- Cotovelo Esq -> Mão Esq
    {7, 6},   -- Pescoço -> Ombro Dir
    {6, 7},   -- Ombro Dir -> Cotovelo Dir
    {7, 24},  -- Cotovelo Dir -> Mão Dir
    {1, 42},  -- Pélvis -> Joelho Esq
    {42, 41}, -- Joelho Esq -> Pé Esq
    {1, 52},  -- Pélvis -> Joelho Dir
    {52, 51}  -- Joelho Dir -> Pé Dir
}

local function drawCornerBox(drawList, x, y, w, h, color)
    local lineW = math.floor(w * 0.28)
    local lineH = math.floor(h * 0.22)
    -- Top Left
    drawList:AddLine(imgui.ImVec2(x, y), imgui.ImVec2(x + lineW, y), color, 1.5)
    drawList:AddLine(imgui.ImVec2(x, y), imgui.ImVec2(x, y + lineH), color, 1.5)
    -- Top Right
    drawList:AddLine(imgui.ImVec2(x + w, y), imgui.ImVec2(x + w - lineW, y), color, 1.5)
    drawList:AddLine(imgui.ImVec2(x + w, y), imgui.ImVec2(x + w, y + lineH), color, 1.5)
    -- Bottom Left
    drawList:AddLine(imgui.ImVec2(x, y + h), imgui.ImVec2(x + lineW, y + h), color, 1.5)
    drawList:AddLine(imgui.ImVec2(x, y + h), imgui.ImVec2(x, y + h - lineH), color, 1.5)
    -- Bottom Right
    drawList:AddLine(imgui.ImVec2(x + w, y + h), imgui.ImVec2(x + w - lineW, y + h), color, 1.5)
    drawList:AddLine(imgui.ImVec2(x + w, y + h), imgui.ImVec2(x + w, y + h - lineH), color, 1.5)
end

function Visuals.render(config, customDrawList)
    if not okImgui or not config or not config.visuals or not config.visuals.enabled then return end
    
    -- Proteção rigorosa contra execução antes do spawn ou durante conexão / login / diálogos
    if isSampLoaded() and (not isSampAvailable() or not sampIsLocalPlayerSpawned() or sampIsDialogActive()) then 
        return 
    end
    if not doesCharExist(playerPed) or isCharDead(playerPed) then 
        return 
    end
    
    local drawList = customDrawList or (imgui.GetWindowDrawList and imgui.GetWindowDrawList())
    if not drawList then return end

    local px, py, pz = getCharCoordinates(playerPed)
    local sw, sh = getScreenResolution()
    local maxDist = tonumber(config.visuals.maxDistance) or 250
    
    -- 0. INDICADOR VISUAL MÍNIMO DE TESTE (Prova que a camada de desenho do ImGui está ativa na tela)
    local testText = "[ SOMALIA ESP ACTIVE ]"
    local testSize = imgui.CalcTextSize(testText)
    local testX = (sw - testSize.x) / 2
    local testY = 25
    drawList:AddRectFilled(imgui.ImVec2(testX - 6, testY - 2), imgui.ImVec2(testX + testSize.x + 6, testY + testSize.y + 2), imgui.ImColor(10, 15, 25, 200):GetU32(), 4.0)
    drawList:AddText(imgui.ImVec2(testX, testY), imgui.ImColor(0, 200, 255, 255):GetU32(), testText)

    local pedsEncontrados = 0
    local pedsValidos = 0
    local coordsValidas = 0
    local boxesDesenhadas = 0
    
    for _, handle in ipairs(getAllChars()) do
        pedsEncontrados = pedsEncontrados + 1
        
        -- Validação de ped válido: exclui o jogador local e peds mortos
        if handle ~= playerPed and doesCharExist(handle) and not isCharDead(handle) then
            pedsValidos = pedsValidos + 1
            local tx, ty, tz = getCharCoordinates(handle)
            local dist = getDistanceBetweenCoords3d(px, py, pz, tx, ty, tz)
            
            if dist <= maxDist then
                -- Conversão 3D -> 2D robusta (Pé, Cabeça e Centro)
                local resFoot, fx, fy = convert3DCoordsToScreen(tx, ty, tz - 0.95)
                local resHead, hx, hy = convert3DCoordsToScreen(tx, ty, tz + 0.85)
                local resCenter, cx, cy = convert3DCoordsToScreen(tx, ty, tz)
                
                if resFoot or resHead or resCenter then
                    coordsValidas = coordsValidas + 1
                    
                    local boxH = 0
                    local boxW = 0
                    local boxX = 0
                    local boxY = 0
                    
                    if resFoot and resHead then
                        boxH = math.abs(fy - hy)
                        boxW = boxH * 0.52
                        boxX = fx - (boxW / 2)
                        boxY = math.min(fy, hy)
                    elseif resCenter then
                        boxH = math.max(12, (sh / math.max(1, dist)) * 1.15)
                        boxW = boxH * 0.52
                        boxX = cx - (boxW / 2)
                        boxY = cy - (boxH / 2)
                        fx, fy = cx, cy + (boxH / 2)
                    end
                    
                    if boxH > 4 and boxW > 2 then
                        boxesDesenhadas = boxesDesenhadas + 1
                        
                        local isEnemy = true
                        local colorBox = isEnemy and imgui.ImColor(255, 60, 60, 240):GetU32() or imgui.ImColor(0, 140, 255, 240):GetU32()
                        local colorText = imgui.ImColor(245, 248, 255, 255):GetU32()
                        
                        -- 1. 2D Box / Corner Box ESP
                        if config.visuals.cornerBox then
                            drawCornerBox(drawList, boxX, boxY, boxW, boxH, colorBox)
                        elseif config.visuals.boxEsp then
                            drawList:AddRect(imgui.ImVec2(boxX, boxY), imgui.ImVec2(boxX + boxW, boxY + boxH), colorBox, 0.0)
                        end
                        
                        -- 2. Snaplines
                        if config.visuals.snaplines and resFoot then
                            drawList:AddLine(imgui.ImVec2(sw / 2, sh), imgui.ImVec2(fx, fy), imgui.ImColor(255, 60, 60, 140):GetU32(), 1.0)
                        end
                        
                        -- 3. Health Bar & Armor Bar
                        local hp = getCharHealth(handle)
                        local arm = getCharArmour(handle)
                        
                        if config.visuals.healthBar then
                            local barX = boxX - 6
                            local hpPercent = math.max(0, math.min(100, hp)) / 100.0
                            local fillH = boxH * hpPercent
                            drawList:AddRectFilled(imgui.ImVec2(barX, boxY), imgui.ImVec2(barX + 3, boxY + boxH), imgui.ImColor(20, 20, 20, 200):GetU32(), 0.0)
                            drawList:AddRectFilled(imgui.ImVec2(barX, boxY + boxH - fillH), imgui.ImVec2(barX + 3, boxY + boxH), imgui.ImColor(30, 215, 96, 255):GetU32(), 0.0)
                        end
                        
                        if config.visuals.armorBar and arm > 0 then
                            local barX = boxX + boxW + 3
                            local armPercent = math.max(0, math.min(100, arm)) / 100.0
                            local fillH = boxH * armPercent
                            drawList:AddRectFilled(imgui.ImVec2(barX, boxY), imgui.ImVec2(barX + 3, boxY + boxH), imgui.ImColor(20, 20, 20, 200):GetU32(), 0.0)
                            drawList:AddRectFilled(imgui.ImVec2(barX, boxY + boxH - fillH), imgui.ImVec2(barX + 3, boxY + boxH), imgui.ImColor(230, 235, 255, 255):GetU32(), 0.0)
                        end
                        
                        -- 4. Nickname & ID
                        local label = ""
                        if isSampLoaded() and isSampAvailable() then
                            local res, id = sampGetPlayerIdByCharHandle(handle)
                            if res then
                                local nick = sampGetPlayerNickname(id) or "Player"
                                if config.visuals.playerNames and config.visuals.playerId then
                                    label = string.format("%s [%d]", nick, id)
                                elseif config.visuals.playerNames then
                                    label = nick
                                elseif config.visuals.playerId then
                                    label = string.format("[%d]", id)
                                end
                            end
                        end
                        
                        if label ~= "" then
                            local tSize = imgui.CalcTextSize(label)
                            drawList:AddText(imgui.ImVec2(boxX + (boxW - tSize.x) / 2, boxY - tSize.y - 3), colorText, label)
                        end
                        
                        -- 5. Distância
                        if config.visuals.distance then
                            local distText = string.format("%dm", math.floor(dist))
                            local dSize = imgui.CalcTextSize(distText)
                            drawList:AddText(imgui.ImVec2(boxX + (boxW - dSize.x) / 2, boxY + boxH + 3), imgui.ImColor(180, 195, 220, 240):GetU32(), distText)
                        end
                        
                        -- 6. Skeleton ESP com validação de ponteiro
                        if config.visuals.skeleton then
                            local ptr = getCharPointer(handle)
                            if ptr and ptr ~= 0 then
                                for _, pair in ipairs(skeletonPairs) do
                                    local okBone, errBone = pcall(function()
                                        local b1x, b1y, b1z = getPedBonePosition(handle, pair[1])
                                        local b2x, b2y, b2z = getPedBonePosition(handle, pair[2])
                                        local r1, s1x, s1y = convert3DCoordsToScreen(b1x, b1y, b1z)
                                        local r2, s2x, s2y = convert3DCoordsToScreen(b2x, b2y, b2z)
                                        if r1 and r2 then
                                            drawList:AddLine(imgui.ImVec2(s1x, s1y), imgui.ImVec2(s2x, s2y), imgui.ImColor(255, 255, 255, 200):GetU32(), 1.0)
                                        end
                                    end)
                                    if not okBone and errBone then
                                        print("[SOMALIA][ESP ERRO SKELETON] " .. tostring(errBone))
                                    end
                                end
                            end
                        end
                    end
                end
            end
        end
    end
    
    local now = os.clock()
    if (now - Visuals.lastLogTick) >= 2.0 then
        Visuals.lastLogTick = now
        print("[SOMALIA][ESP] ENABLED")
        print(string.format("[SOMALIA][ESP] render chamado | peds encontrados: %d | peds validos: %d | coords validas: %d | boxes desenhadas: %d", 
            pedsEncontrados, pedsValidos, coordsValidas, boxesDesenhadas))
    end
end

return Visuals
