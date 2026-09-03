--[[
    Somalia - HUD & Watermark Module
    Renderização de overlay moderno e configurável (FPS, Ping, ID, Server, Coords e Active Features)
    Protegido contra chamadas antes do spawn local
]]

local okImgui, imgui = pcall(require, 'imgui')

local HUD = {
    fps = 0,
    fpsCounter = 0,
    lastFpsTime = 0
}

function HUD.render(config, customDrawList)
    if not okImgui or not config or not config.hud then return end
    
    local now = os.clock()
    HUD.fpsCounter = HUD.fpsCounter + 1
    if (now - HUD.lastFpsTime) >= 1.0 then
        HUD.fps = HUD.fpsCounter
        HUD.fpsCounter = 0
        HUD.lastFpsTime = now
    end
    
    local drawList = customDrawList or (imgui.GetWindowDrawList and imgui.GetWindowDrawList())
    if not drawList then return end
    
    local sw, sh = getScreenResolution()
    
    ---------------------------------------------------------
    -- 1. WATERMARK SUPERIOR (SOMALIA | FPS | PING | TIME)
    ---------------------------------------------------------
    if config.hud.watermark then
        local ping = 0
        local nick = "Player"
        local myId = -1
        if isSampLoaded() and isSampAvailable() and sampIsLocalPlayerSpawned() and doesCharExist(playerPed) then
            local res, id = sampGetPlayerIdByCharHandle(playerPed)
            if res then
                myId = id
                nick = sampGetPlayerNickname(id) or "Player"
                ping = sampGetPlayerPing(id) or 0
            end
        end
        
        local timeStr = os.date("%H:%M:%S")
        local text = string.format("SOMALIA  |  %s [%d]  |  %d FPS  |  %dms  |  %s", nick, myId, HUD.fps, ping, timeStr)
        local textSize = imgui.CalcTextSize(text)
        
        local padX, padY = 10, 5
        local boxW = textSize.x + (padX * 2)
        local boxH = textSize.y + (padY * 2)
        local startX = sw - boxW - 14
        local startY = 14
        
        -- Fundo escuro com borda e linha neon no topo
        local bgCol = imgui.ImColor(10, 11, 16, 220):GetU32()
        local borderCol = imgui.ImColor(30, 36, 48, 200):GetU32()
        local neonCol = imgui.ImColor(0, 128, 255, 255):GetU32()
        
        drawList:AddRectFilled(imgui.ImVec2(startX, startY), imgui.ImVec2(startX + boxW, startY + boxH), bgCol, 5.0)
        drawList:AddRect(imgui.ImVec2(startX, startY), imgui.ImVec2(startX + boxW, startY + boxH), borderCol, 5.0)
        drawList:AddRectFilled(imgui.ImVec2(startX + 2, startY), imgui.ImVec2(startX + boxW - 2, startY + 2), neonCol, 2.0)
        
        drawList:AddText(imgui.ImVec2(startX + padX, startY + padY), imgui.ImColor(240, 245, 255, 255):GetU32(), text)
    end
    
    ---------------------------------------------------------
    -- 2. COORDENADAS (CANTO INFERIOR ESQUERDO)
    ---------------------------------------------------------
    if config.hud.coords and doesCharExist(playerPed) and not isCharDead(playerPed) then
        local px, py, pz = getCharCoordinates(playerPed)
        local coordText = string.format("X: %.2f  Y: %.2f  Z: %.2f", px, py, pz)
        local cSize = imgui.CalcTextSize(coordText)
        
        local cX = 14
        local cY = sh - 28
        drawList:AddRectFilled(imgui.ImVec2(cX - 4, cY - 2), imgui.ImVec2(cX + cSize.x + 4, cY + cSize.y + 2), imgui.ImColor(10, 11, 16, 180):GetU32(), 4.0)
        drawList:AddText(imgui.ImVec2(cX, cY), imgui.ImColor(0, 160, 255, 255):GetU32(), coordText)
    end
    
    ---------------------------------------------------------
    -- 3. ACTIVE FEATURES (LISTA DE FUNÇÕES ATIVAS NA TELA)
    ---------------------------------------------------------
    if config.hud.activeFeatures then
        local activeList = {}
        if config.aim and config.aim.globalEnabled then table.insert(activeList, "Aimbot") end
        if config.silent and config.silent.globalEnabled then table.insert(activeList, "Silent Aim") end
        if config.visuals and config.visuals.enabled then table.insert(activeList, "Visuals (ESP)") end
        if config.player and config.player.cSlideAtivo then table.insert(activeList, "C-Slide") end
        if config.player and config.player.autoSlideAtivo then table.insert(activeList, "Auto Slide") end
        if config.vehicles and config.vehicles.godmode then table.insert(activeList, "Car Godmode") end
        
        if #activeList > 0 then
            local startY = 50
            for _, name in ipairs(activeList) do
                local nSize = imgui.CalcTextSize(name)
                local curX = sw - nSize.x - 22
                
                drawList:AddRectFilled(imgui.ImVec2(curX - 6, startY), imgui.ImVec2(sw - 14, startY + nSize.y + 2), imgui.ImColor(10, 11, 16, 160):GetU32(), 3.0)
                drawList:AddText(imgui.ImVec2(curX, startY + 1), imgui.ImColor(230, 238, 255, 240):GetU32(), name)
                
                startY = startY + nSize.y + 6
            end
        end
    end
end

return HUD
