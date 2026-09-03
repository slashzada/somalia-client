--[[
    SOMALIA - Professional Showcase Edition
    Main Entrypoint & SML UI Architecture
    Reativação Segura e Isolada de ESP e FOV Circle (Aimbot & Silent FOV)
]]

-- Configura caminho de busca dos módulos locais
local workDir = getWorkingDirectory()
package.path = workDir .. "\\?.lua;" .. workDir .. "\\somalia\\?.lua;" .. workDir .. "\\lib\\?.lua;" .. package.path

local function safeRequire(modName, fallbackName)
    local ok, mod = pcall(require, modName)
    if not ok and fallbackName then
        ok, mod = pcall(require, fallbackName)
    end
    if not ok then
        print("[SOMALIA AVISO] Falha ao carregar modulo: " .. tostring(modName) .. " (" .. tostring(mod) .. ")")
        return false, {}
    end
    return true, mod
end

-- =========================================================================
-- CONTROLE DE ISOLAMENTO DE MÓDULOS
-- =========================================================================
local ENABLE_VISUALS_RUNTIME = true   -- Reativa ESP e FOVs (Aimbot & Silent) com gating estrito de spawn
local ENABLE_GAMEPLAY_RUNTIME = false -- Mantém Aim, Player, Vehicles, World e Misc desativados

-- Carregamento seguro dos módulos do Somalia
local okTheme, Theme = safeRequire('somalia.theme', 'theme')
local okNotifications, Notifications = safeRequire('somalia.notifications', 'notifications')
local okKeybinds, Keybinds = safeRequire('somalia.keybinds', 'keybinds')
local okConfig, Config = safeRequire('somalia.config', 'config')
local okHUD, HUD = safeRequire('somalia.hud', 'hud')
local okAim, Aim = safeRequire('somalia.aim', 'aim')
local okVisuals, Visuals = safeRequire('somalia.visuals', 'visuals')
local okPlayer, Player = safeRequire('somalia.player', 'player')
local okVehicles, Vehicles = safeRequire('somalia.vehicles', 'vehicles')
local okWorld, World = safeRequire('somalia.world', 'world')
local okMisc, Misc = safeRequire('somalia.misc', 'misc')

local okImgui, imgui = pcall(require, 'imgui')

-- Configuração segura de codificação UTF-8 para acentuação no ImGui
local u8 = function(str) return str end
local okEncoding, encoding = pcall(require, 'encoding')
if okEncoding then
    encoding.default = 'CP1252'
    u8 = encoding.UTF8
end

-- Inicialização de Configurações, Temas e Notificações
if okConfig and Config.init then Config.init() end
if okNotifications and Notifications.init and okTheme then Notifications.init(Theme) end
if okTheme and Theme.initFonts then Theme.initFonts() end

-- Variáveis de controle de interface ImGui
local janelaAberta = okImgui and imgui.ImBool(false) or { v = false }

-- Navegação das Abas
local abaLateral = 1 -- 1 = Aimbot, 2 = Visuals, 3 = World, 4 = Vehicles, 5 = Players, 6 = Exploits, 7 = Configs
local lastAbaLateral = 1
local tabAlpha = 1.0

-- Animações dos componentes
local animCheckboxes = {}
local animNav = {}
local animCombos = {}

local hitboxOptions = { "Head", "Neck", "Chest", "Pelvis" }
local weaponNames = {
    "Desert Eagle", "M4", "AK-47", "Shotgun", "Sawnoff",
    "Combat Shotgun", "MP5", "Tec-9", "Micro Uzi", "Sniper Rifle",
    "Country Rifle", "Pistol (Colt 45)", "Silenced 9mm"
}
local weaponIds = {
    24, 31, 30, 25, 26,
    27, 29, 32, 28, 34,
    33, 22, 23
}

local keyWasDown = {}
local function isKeyJustPressed(key)
    if isSampLoaded() and (sampIsDialogActive() or sampIsChatInputActive()) then
        return false
    end
    if isKeyDown(key) then
        if not keyWasDown[key] then
            keyWasDown[key] = true
            return true
        end
    else
        keyWasDown[key] = false
    end
    return false
end

-- Validação de segurança: apenas quando o jogador está efetivamente spawnado e livre de login/diálogos
local function isPlayerReady()
    if isSampLoaded() then
        if not isSampAvailable() or not sampIsLocalPlayerSpawned() or sampIsDialogActive() then
            return false
        end
    end
    return doesCharExist(playerPed) and not isCharDead(playerPed)
end

-------------------------------------------------------------------------
-- COMPONENTES GRÁFICOS VETORIAIS 1:1
-------------------------------------------------------------------------

-- 1. Header com Perfil (Username, Owner, Avatar circular no canto superior direito)
local function DrawHeaderProfile(headerX, headerY, name, role)
    local drawList = imgui.GetWindowDrawList()
    local txtNameSize = imgui.CalcTextSize(name)
    local txtRoleSize = imgui.CalcTextSize(role)
    
    local avRadius = 15.0
    local avCx = headerX - avRadius - 6
    local avCy = headerY + 18
    
    local textRightEdge = avCx - avRadius - 10
    
    -- Nome do Usuário
    drawList:AddText(imgui.ImVec2(textRightEdge - txtNameSize.x, headerY + 5), imgui.ImColor(255, 255, 255, 255):GetU32(), name)
    -- Cargo / Subtítulo
    drawList:AddText(imgui.ImVec2(textRightEdge - txtRoleSize.x, headerY + 22), imgui.ImColor(135, 148, 170, 255):GetU32(), role)
    
    -- Avatar circular azul neon com anel brilhante
    drawList:AddCircleFilled(imgui.ImVec2(avCx, avCy), avRadius, imgui.ImColor(0, 128, 255, 255):GetU32())
    drawList:AddCircle(imgui.ImVec2(avCx, avCy), avRadius, imgui.ImColor(80, 180, 255, 255):GetU32(), 24)
    
    -- Silhueta branca do avatar
    drawList:AddCircleFilled(imgui.ImVec2(avCx, avCy - 4.2), 4.6, imgui.ImColor(255, 255, 255, 255):GetU32())
    drawList:AddTriangleFilled(imgui.ImVec2(avCx - 7.5, avCy + 11), imgui.ImVec2(avCx, avCy + 2), imgui.ImVec2(avCx + 7.5, avCy + 11), imgui.ImColor(255, 255, 255, 255):GetU32())
end

-- 2. Botão da Sidebar Vertical com Animação Suave e Ícones Idênticos ao Vídeo
local function DrawSidebarItem(id, label, iconType, isActive)
    if animNav[id] == nil then animNav[id] = isActive and 1.0 or 0.0 end
    local drawList = imgui.GetWindowDrawList()
    local pos = imgui.GetCursorScreenPos()
    local w, h = 135, 34
    
    imgui.PushID(id)
    local clicked = imgui.InvisibleButton("##" .. id, imgui.ImVec2(w, h))
    local hovered = imgui.IsItemHovered()
    imgui.PopID()
    
    local target = isActive and 1.0 or (hovered and 0.45 or 0.0)
    animNav[id] = animNav[id] + (target - animNav[id]) * 0.22
    local val = animNav[id]
    
    local r = math.floor(10 + (22 - 10) * val)
    local g = math.floor(14 + (38 - 14) * val)
    local b = math.floor(22 + (70 - 22) * val)
    local a = math.floor(0 + (255 - 0) * val)
    local bgColor = imgui.ImColor(r, g, b, a):GetU32()
    
    drawList:AddRectFilled(pos, imgui.ImVec2(pos.x + w, pos.y + h), bgColor, Theme.Rounding.Button)
    if val > 0.02 then
        local bA = math.floor(val * 220)
        drawList:AddRect(pos, imgui.ImVec2(pos.x + w, pos.y + h), imgui.ImColor(0, 128, 255, bA):GetU32(), Theme.Rounding.Button)
    end
    
    local ir = math.floor(120 + (0 - 120) * val)
    local ig = math.floor(135 + (140 - 135) * val)
    local ib = math.floor(155 + (255 - 155) * val)
    local iconColor = (hovered and not isActive) and imgui.ImColor(210, 225, 255, 255):GetU32() or imgui.ImColor(ir, ig, ib, 255):GetU32()
    
    local icx = pos.x + 18
    local icy = pos.y + (h / 2)
    
    if iconType == "aimbot" then
        drawList:AddCircle(imgui.ImVec2(icx, icy), 6.5, iconColor, 16)
        drawList:AddCircle(imgui.ImVec2(icx, icy), 2.5, iconColor, 12)
        drawList:AddLine(imgui.ImVec2(icx - 8, icy), imgui.ImVec2(icx - 4, icy), iconColor, 1.4)
        drawList:AddLine(imgui.ImVec2(icx + 4, icy), imgui.ImVec2(icx + 8, icy), iconColor, 1.4)
        drawList:AddLine(imgui.ImVec2(icx, icy - 8), imgui.ImVec2(icx, icy - 4), iconColor, 1.4)
        drawList:AddLine(imgui.ImVec2(icx, icy + 4), imgui.ImVec2(icx, icy + 8), iconColor, 1.4)
    elseif iconType == "visuals" then
        drawList:AddCircle(imgui.ImVec2(icx, icy), 6.0, iconColor, 16)
        drawList:AddCircleFilled(imgui.ImVec2(icx, icy), 2.4, iconColor)
    elseif iconType == "world" then
        drawList:AddCircle(imgui.ImVec2(icx, icy), 6.5, iconColor, 16)
        drawList:AddLine(imgui.ImVec2(icx - 6.5, icy), imgui.ImVec2(icx + 6.5, icy), iconColor, 1.2)
        drawList:AddLine(imgui.ImVec2(icx, icy - 6.5), imgui.ImVec2(icx + 6.5, icy), iconColor, 1.2)
        drawList:AddCircle(imgui.ImVec2(icx, icy), 3.5, iconColor, 12)
    elseif iconType == "vehicles" then
        drawList:AddRect(imgui.ImVec2(icx - 6, icy - 3), imgui.ImVec2(icx + 6, icy + 4), iconColor, 2.0)
        drawList:AddCircleFilled(imgui.ImVec2(icx - 4, icy + 4), 1.8, iconColor)
        drawList:AddCircleFilled(imgui.ImVec2(icx + 4, icy + 4), 1.8, iconColor)
    elseif iconType == "players" then
        drawList:AddCircleFilled(imgui.ImVec2(icx - 3, icy - 3), 2.5, iconColor)
        drawList:AddCircleFilled(imgui.ImVec2(icx + 3, icy - 1), 2.0, iconColor)
        drawList:AddTriangleFilled(imgui.ImVec2(icx - 6, icy + 5), imgui.ImVec2(icx - 3, icy + 1), imgui.ImVec2(icx, icy + 5), iconColor)
    elseif iconType == "exploits" then
        drawList:AddCircleFilled(imgui.ImVec2(icx, icy), 4.0, iconColor)
        drawList:AddLine(imgui.ImVec2(icx - 5, icy - 3), imgui.ImVec2(icx + 5, icy + 3), iconColor, 1.2)
        drawList:AddLine(imgui.ImVec2(icx - 5, icy + 3), imgui.ImVec2(icx + 5, icy - 3), iconColor, 1.2)
        drawList:AddLine(imgui.ImVec2(icx - 6, icy), imgui.ImVec2(icx + 6, icy), iconColor, 1.2)
    elseif iconType == "configs" then
        drawList:AddRectFilled(imgui.ImVec2(icx - 6, icy - 4), imgui.ImVec2(icx - 2, icy - 2), iconColor, 1.0)
        drawList:AddRect(imgui.ImVec2(icx - 6, icy - 2), imgui.ImVec2(icx + 6, icy + 5), iconColor, 2.0)
    end
    
    local tr = math.floor(140 + (255 - 140) * val)
    local tg = math.floor(150 + (255 - 150) * val)
    local tb = math.floor(170 + (255 - 170) * val)
    local textColor = (hovered and not isActive) and imgui.ImColor(230, 240, 255, 255):GetU32() or imgui.ImColor(tr, tg, tb, 255):GetU32()
    
    local txtSize = imgui.CalcTextSize(label)
    drawList:AddText(imgui.ImVec2(pos.x + 36, pos.y + (h - txtSize.y) / 2), textColor, label)
    
    imgui.Spacing()
    return clicked
end

-- 3. Checkbox com Animação Suave e Keybind Interativo
local function DrawCheckboxWithKeybind(label, val_bool, key_code, id_str, onToggle, onKeyChange)
    if animCheckboxes[id_str] == nil then animCheckboxes[id_str] = val_bool and 1.0 or 0.0 end
    local target = val_bool and 1.0 or 0.0
    animCheckboxes[id_str] = animCheckboxes[id_str] + (target - animCheckboxes[id_str]) * 0.28
    local val = animCheckboxes[id_str]
    
    local drawList = imgui.GetWindowDrawList()
    local pos = imgui.GetCursorScreenPos()
    local availW = imgui.GetContentRegionAvailWidth()
    local h = 22
    local boxSize = 16
    local boxX = pos.x + 2
    local boxY = pos.y + (h - boxSize) / 2
    
    local btnW = key_code and (availW - 75) or availW
    
    imgui.PushID(id_str)
    local clicked = imgui.InvisibleButton("##" .. id_str, imgui.ImVec2(btnW, h))
    local hovered = imgui.IsItemHovered()
    imgui.PopID()
    
    if clicked and onToggle then
        onToggle(not val_bool)
        if okConfig and Config.save then Config.save() end
    end
    
    local r = math.floor(12 + (0 - 12) * val)
    local g = math.floor(16 + (128 - 16) * val)
    local b = math.floor(26 + (255 - 26) * val)
    local bgCol = imgui.ImColor(r, g, b, 255):GetU32()
    
    local br = math.floor(36 + (0 - 36) * val)
    local bg = math.floor(44 + (140 - 44) * val)
    local bb = math.floor(60 + (255 - 60) * val)
    local borderCol = hovered and imgui.ImColor(60, 140, 230, 255):GetU32() or imgui.ImColor(br, bg, bb, 255):GetU32()
    
    drawList:AddRectFilled(imgui.ImVec2(boxX, boxY), imgui.ImVec2(boxX + boxSize, boxY + boxSize), bgCol, Theme.Rounding.Checkbox)
    drawList:AddRect(imgui.ImVec2(boxX, boxY), imgui.ImVec2(boxX + boxSize, boxY + boxSize), borderCol, Theme.Rounding.Checkbox)
    
    if val > 0.05 then
        local checkAlpha = math.min(1.0, val * 1.6)
        local checkCol = imgui.ImColor(255, 255, 255, math.floor(255 * checkAlpha)):GetU32()
        local pt1 = imgui.ImVec2(boxX + 3.0, boxY + 8.0)
        local pt2 = imgui.ImVec2(boxX + 6.0, boxY + 11.5)
        local pt3 = imgui.ImVec2(boxX + 12.5, boxY + 4.5)
        drawList:AddLine(pt1, pt2, checkCol, 1.8)
        drawList:AddLine(pt2, pt3, checkCol, 1.8)
    end
    
    local txtCol = hovered and imgui.ImColor(255, 255, 255, 255):GetU32() or imgui.ImColor(230, 235, 245, 255):GetU32()
    local txtSize = imgui.CalcTextSize(label)
    drawList:AddText(imgui.ImVec2(boxX + boxSize + 8, pos.y + (h - txtSize.y) / 2), txtCol, label)
    
    if key_code ~= nil and okKeybinds then
        imgui.SameLine(availW - 70)
        Keybinds.drawKeybindButton(id_str, key_code, onKeyChange, 68)
    end
    
    imgui.Spacing()
    return clicked
end

local function DrawVideoCheckbox(label, val_bool, id_str, onToggle)
    return DrawCheckboxWithKeybind(label, val_bool, nil, id_str, onToggle, nil)
end

-- 4. Dropdown / Combobox Estilizado
local function DrawVideoCombo(label, items, selected_idx, id_str, onSelect)
    if animCombos[id_str] == nil then animCombos[id_str] = 0.0 end
    
    if label and label ~= "" then
        imgui.TextColored(imgui.ImVec4(0.88, 0.91, 0.96, 1.0), label)
    end
    
    local drawList = imgui.GetWindowDrawList()
    local pos = imgui.GetCursorScreenPos()
    local availW = imgui.GetContentRegionAvailWidth()
    local w = availW - 4
    local h = 26
    local x = pos.x + 2
    local y = pos.y
    
    imgui.PushID(id_str)
    local clicked = imgui.InvisibleButton("##" .. id_str, imgui.ImVec2(w, h))
    local hovered = imgui.IsItemHovered()
    local active = imgui.IsItemActive()
    imgui.PopID()
    
    if clicked then
        local nextIdx = ((selected_idx or 0) + 1) % #items
        if onSelect then onSelect(nextIdx) end
        if okConfig and Config.save then Config.save() end
    end
    
    local target = (hovered or active) and 1.0 or 0.0
    animCombos[id_str] = animCombos[id_str] + (target - animCombos[id_str]) * 0.25
    local val = animCombos[id_str]
    
    local r = math.floor(11 + (18 - 11) * val)
    local g = math.floor(14 + (24 - 14) * val)
    local b = math.floor(22 + (40 - 22) * val)
    local bgCol = imgui.ImColor(r, g, b, 255):GetU32()
    
    local br = math.floor(28 + (0 - 28) * val)
    local bg = math.floor(36 + (128 - 36) * val)
    local bb = math.floor(52 + (255 - 52) * val)
    local borderCol = imgui.ImColor(br, bg, bb, 255):GetU32()
    
    drawList:AddRectFilled(imgui.ImVec2(x, y), imgui.ImVec2(x + w, y + h), bgCol, Theme.Rounding.Button)
    drawList:AddRect(imgui.ImVec2(x, y), imgui.ImVec2(x + w, y + h), borderCol, Theme.Rounding.Button)
    
    local currentText = items[(selected_idx or 0) + 1] or items[1]
    local cTxtSize = imgui.CalcTextSize(currentText)
    drawList:AddText(imgui.ImVec2(x + 10, y + (h - cTxtSize.y) / 2), imgui.ImColor(245, 248, 255, 255):GetU32(), currentText)
    
    local arrowX = x + w - 14
    local arrowY = y + (h / 2)
    local arrowCol = (hovered or active) and imgui.ImColor(0, 140, 255, 255):GetU32() or imgui.ImColor(130, 145, 170, 255):GetU32()
    drawList:AddLine(imgui.ImVec2(arrowX - 4, arrowY - 2), imgui.ImVec2(arrowX, arrowY + 2), arrowCol, 1.6)
    drawList:AddLine(imgui.ImVec2(arrowX, arrowY + 2), imgui.ImVec2(arrowX + 4, arrowY - 2), arrowCol, 1.6)
    
    imgui.Spacing()
    return clicked
end

-- 5. Slider Idêntico ao Vídeo
local function DrawVideoSlider(label, current_val, min_val, max_val, suffix, id_str, onChange)
    local availW = imgui.GetContentRegionAvailWidth()
    local w = availW - 4
    local barH = 10
    local rounding = 4.0
    
    local valStr = tostring(math.floor(current_val or min_val)) .. (suffix or "")
    imgui.TextColored(imgui.ImVec4(0.88, 0.91, 0.96, 1.0), label)
    local vTxtSize = imgui.CalcTextSize(valStr)
    imgui.SameLine(availW - vTxtSize.x - 2)
    imgui.TextColored(Theme.Colors.AccentPrimary, valStr)
    
    local drawList = imgui.GetWindowDrawList()
    local pos = imgui.GetCursorScreenPos()
    local x = pos.x + 2
    local y = pos.y
    
    imgui.PushID(id_str)
    local clicked = imgui.InvisibleButton("##" .. id_str, imgui.ImVec2(w, barH + 4))
    local isItemActive = imgui.IsItemActive()
    local isItemHovered = imgui.IsItemHovered()
    imgui.PopID()
    
    local changed = false
    if isItemActive then
        local mpos = imgui.GetIO().MousePos
        local fraction = math.max(0.0, math.min(1.0, (mpos.x - x) / w))
        local newVal = math.floor(min_val + fraction * (max_val - min_val) + 0.5)
        if newVal ~= current_val then
            current_val = newVal
            changed = true
            if onChange then onChange(newVal) end
            if okConfig and Config.save then Config.save() end
        end
    end
    
    local fraction = math.max(0.0, math.min(1.0, ((current_val or min_val) - min_val) / (max_val - min_val)))
    local fillW = math.max(barH, fraction * w)
    
    local troughCol = imgui.ImColor(14, 16, 24, 255):GetU32()
    local borderCol = (isItemActive or isItemHovered) and imgui.ImColor(40, 60, 95, 255):GetU32() or imgui.ImColor(26, 30, 42, 255):GetU32()
    drawList:AddRectFilled(imgui.ImVec2(x, y + 2), imgui.ImVec2(x + w, y + 2 + barH), troughCol, rounding)
    drawList:AddRect(imgui.ImVec2(x, y + 2), imgui.ImVec2(x + w, y + 2 + barH), borderCol, rounding)
    
    if fraction > 0.01 then
        local fillCol = isItemActive and imgui.ImColor(0, 110, 230, 255):GetU32() or (isItemHovered and imgui.ImColor(20, 145, 255, 255):GetU32() or imgui.ImColor(0, 128, 255, 255):GetU32())
        drawList:AddRectFilled(imgui.ImVec2(x, y + 2), imgui.ImVec2(x + fillW, y + 2 + barH), fillCol, rounding)
    end
    
    local knobW = 8
    local knobX = math.max(x, math.min(x + w - knobW, x + fillW - knobW))
    drawList:AddRectFilled(imgui.ImVec2(knobX, y + 1), imgui.ImVec2(knobX + knobW, y + barH + 3), imgui.ImColor(255, 255, 255, 255):GetU32(), 3.0)
    drawList:AddRect(imgui.ImVec2(knobX, y + 1), imgui.ImVec2(knobX + knobW, y + barH + 3), imgui.ImColor(200, 220, 255, 255):GetU32(), 3.0)
    
    imgui.Spacing()
    return changed
end

-------------------------------------------------------------------------
-- RENDERIZAÇÃO DA TELA IMGUI (OVERLAY VISUAL PROTEGIDO)
-------------------------------------------------------------------------
if okImgui then
    function imgui.OnDrawFrame()
        local sw, sh = getScreenResolution()

        -- 1. OVERLAY DE ESP E FOV: Renderiza somente se o jogador estiver spawnado e fora de diálogos/login
        if isPlayerReady() and ENABLE_VISUALS_RUNTIME then
            imgui.SetNextWindowPos(imgui.ImVec2(0, 0), imgui.Cond.Always)
            imgui.SetNextWindowSize(imgui.ImVec2(sw, sh), imgui.Cond.Always)
            local overlayFlags = imgui.WindowFlags.NoTitleBar + imgui.WindowFlags.NoResize + imgui.WindowFlags.NoMove + imgui.WindowFlags.NoScrollbar + imgui.WindowFlags.NoScrollWithMouse + imgui.WindowFlags.NoInputs + imgui.WindowFlags.NoSavedSettings
            
            imgui.PushStyleColor(imgui.Col.WindowBg, imgui.ImVec4(0, 0, 0, 0))
            imgui.PushStyleVar(imgui.StyleVar.WindowPadding, imgui.ImVec2(0, 0))
            
            if imgui.Begin("##SOMALIA_FULLSCREEN_OVERLAY", nil, overlayFlags) then
                local drawList = imgui.GetWindowDrawList()
                if drawList then
                    if okVisuals and Visuals.render then pcall(Visuals.render, Config.data, drawList) end
                    if okAim and Aim.renderFovCircle then pcall(Aim.renderFovCircle, Config.data, drawList) end
                    if okHUD and HUD.render then pcall(HUD.render, Config.data, drawList) end
                    if okNotifications and Notifications.render then pcall(Notifications.render, drawList) end
                end
                imgui.End()
            end
            imgui.PopStyleVar(1)
            imgui.PopStyleColor(1)
        end

        -- 2. JANELA PRINCIPAL DO MENU (Só renderiza se janelaAberta.v for true)
        if janelaAberta.v then
            imgui.SetNextWindowPos(imgui.ImVec2(sw / 2, sh / 2), imgui.Cond.FirstUseEver, imgui.ImVec2(0.5, 0.5))
            imgui.SetNextWindowSize(Theme.Sizes.Window, imgui.Cond.FirstUseEver)

            local flags = imgui.WindowFlags.NoCollapse + imgui.WindowFlags.NoResize + imgui.WindowFlags.NoTitleBar
            imgui.Begin("##SOMALIA_MAIN_FRAME", janelaAberta, flags)
            
            local azulNeon = Theme.Colors.AccentPrimary
            local branco = Theme.Colors.TextPrimary
            local winPos = imgui.GetWindowPos()
            local winWidth = imgui.GetWindowWidth()

            -- Transição de aba
            if abaLateral ~= lastAbaLateral then
                tabAlpha = 0.0
                lastAbaLateral = abaLateral
            end
            tabAlpha = math.min(1.0, tabAlpha + 0.12)

            -------------------------------------------------------------
            -- TOP HEADER BAR (Logo SML + Perfil no canto direito)
            -------------------------------------------------------------
            imgui.SetCursorPosX(16)
            imgui.SetCursorPosY(10)
            
            if Theme.Fonts.Logo then imgui.PushFont(Theme.Fonts.Logo) end
            imgui.TextColored(branco, "SM")
            imgui.SameLine(0, 0)
            imgui.TextColored(azulNeon, "L")
            if Theme.Fonts.Logo then imgui.PopFont() end
            
            DrawHeaderProfile(winPos.x + winWidth - 14, winPos.y + 8, "sx", "Owner")
            
            imgui.SetCursorPosY(44)
            imgui.Spacing()

            -------------------------------------------------------------
            -- 1. SIDEBAR ESQUERDA (7 Abas Vetoriais 1:1)
            -------------------------------------------------------------
            imgui.BeginChild("##SidebarLeft", imgui.ImVec2(Theme.Sizes.SidebarWidth, 0), false, imgui.WindowFlags.NoScrollbar)
                if DrawSidebarItem("side_aimbot", "Aimbot", "aimbot", (abaLateral == 1)) then abaLateral = 1 end
                if DrawSidebarItem("side_visuals", "Visuals", "visuals", (abaLateral == 2)) then abaLateral = 2 end
                if DrawSidebarItem("side_world", "World", "world", (abaLateral == 3)) then abaLateral = 3 end
                if DrawSidebarItem("side_vehicles", "Vehicles", "vehicles", (abaLateral == 4)) then abaLateral = 4 end
                if DrawSidebarItem("side_players", "Players", "players", (abaLateral == 5)) then abaLateral = 5 end
                if DrawSidebarItem("side_exploits", "Exploits", "exploits", (abaLateral == 6)) then abaLateral = 6 end
                if DrawSidebarItem("side_configs", "Configs", "configs", (abaLateral == 7)) then abaLateral = 7 end
            imgui.EndChild()

            imgui.SameLine(0, 10)

            -------------------------------------------------------------
            -- 2. ÁREA DE CONTEÚDO PRINCIPAL (COM CARDS LADO A LADO)
            -------------------------------------------------------------
            imgui.BeginChild("##MainContentArea", imgui.ImVec2(0, 0), false, imgui.WindowFlags.NoScrollbar)
                imgui.PushStyleVar(imgui.StyleVar.Alpha, tabAlpha)
                local larguraColuna = (imgui.GetWindowWidth() - 12) / 2

                ---------------------------------------------------------
                -- ABA 1: AIMBOT (SELETOR DE ARMA PRIMEIRO + SMOOTH TOTAL)
                ---------------------------------------------------------
                if abaLateral == 1 then
                    -- CARD 1: AIMBOT (LEGIT)
                    imgui.BeginChild("##CardAimbot", imgui.ImVec2(larguraColuna, 0), true)
                        imgui.SetCursorPosX(12); imgui.SetCursorPosY(10)
                        if Theme.Fonts.Title then imgui.PushFont(Theme.Fonts.Title) end
                        imgui.TextColored(branco, "Aimbot")
                        if Theme.Fonts.Title then imgui.PopFont() end
                        imgui.Spacing()

                        -- 1. SELETOR DE ARMA (PRIMEIRO COMPONENTE)
                        DrawVideoCombo("Arma", weaponNames, Config.data.aim.selectedWeaponIndex or 0, "combo_aim_weapon", function(idx)
                            Config.data.aim.selectedWeaponIndex = idx
                        end)

                        local curWIdx = (Config.data.aim.selectedWeaponIndex or 0) + 1
                        local curWId = weaponIds[curWIdx] or 24
                        local wAim = Config.getAimWeapon(curWId)

                        -- 2. MASTER SWITCH & ATIVADO PARA ESTA ARMA
                        DrawCheckboxWithKeybind("Aimbot Master", Config.data.aim.globalEnabled, Config.data.aim.key, "chk_aim_master", function(v)
                            Config.data.aim.globalEnabled = v
                            if okNotifications then Notifications.info("AIMBOT", v and "Aimbot Master Ativado" or "Aimbot Master Desativado") end
                        end, function(id, k) Config.data.aim.key = k end)

                        DrawVideoCheckbox("Ativado para esta Arma", wAim.enabled, "chk_w_aim_en_" .. curWId, function(v) wAim.enabled = v end)
                        
                        -- 3. FOV & SMOOTH
                        DrawVideoSlider("FOV Size", wAim.fovSize, 5, 360, "px", "sld_w_aim_fov_" .. curWId, function(v) wAim.fovSize = v end)
                        DrawVideoSlider("Smooth Horizontal", wAim.smoothH, 1, 50, "", "sld_w_aim_smooth_h_" .. curWId, function(v) wAim.smoothH = v end)
                        DrawVideoSlider("Smooth Vertical", wAim.smoothV, 1, 50, "", "sld_w_aim_smooth_v_" .. curWId, function(v) wAim.smoothV = v end)

                        -- 4. HITBOX & CHECKS
                        DrawVideoCombo("Alvo (Hitbox)", hitboxOptions, wAim.hitbox, "combo_w_aim_hitbox_" .. curWId, function(idx) wAim.hitbox = idx end)
                        DrawVideoCheckbox("Show FOV Circle", wAim.showFov, "chk_w_aim_showfov_" .. curWId, function(v) wAim.showFov = v end)
                        DrawVideoCheckbox("Visible Check (Raycast)", wAim.visibleCheck, "chk_w_aim_vischeck_" .. curWId, function(v) wAim.visibleCheck = v end)
                        DrawVideoCheckbox("Allow NPC", wAim.allowNpc, "chk_w_aim_allownpc_" .. curWId, function(v) wAim.allowNpc = v end)
                        DrawVideoSlider("Max Distance", wAim.maxDist, 10, 300, "m", "sld_w_aim_maxdist_" .. curWId, function(v) wAim.maxDist = v end)
                    imgui.EndChild()

                    imgui.SameLine(0, 10)

                    -- CARD 2: SILENT AIM (SELETOR DE ARMA PRIMEIRO)
                    imgui.BeginChild("##CardSilent", imgui.ImVec2(larguraColuna, 0), true)
                        imgui.SetCursorPosX(12); imgui.SetCursorPosY(10)
                        if Theme.Fonts.Title then imgui.PushFont(Theme.Fonts.Title) end
                        imgui.TextColored(branco, "Silent")
                        if Theme.Fonts.Title then imgui.PopFont() end
                        imgui.Spacing()

                        -- 1. SELETOR DE ARMA (PRIMEIRO COMPONENTE)
                        DrawVideoCombo("Arma", weaponNames, Config.data.silent.selectedWeaponIndex or 0, "combo_sil_weapon", function(idx)
                            Config.data.silent.selectedWeaponIndex = idx
                        end)

                        local curSilWIdx = (Config.data.silent.selectedWeaponIndex or 0) + 1
                        local curSilWId = weaponIds[curSilWIdx] or 24
                        local wSil = Config.getSilentWeapon(curSilWId)

                        -- 2. MASTER SWITCH & ATIVADO PARA ESTA ARMA
                        DrawCheckboxWithKeybind("Silent Master", Config.data.silent.globalEnabled, Config.data.silent.key, "chk_sil_master", function(v)
                            Config.data.silent.globalEnabled = v
                            if okNotifications then Notifications.info("SILENT AIM", v and "Silent Master Ativado" or "Silent Master Desativado") end
                        end, function(id, k) Config.data.silent.key = k end)

                        DrawVideoCheckbox("Ativado para esta Arma", wSil.enabled, "chk_w_sil_en_" .. curSilWId, function(v) wSil.enabled = v end)
                        DrawVideoSlider("FOV Size", wSil.fovSize, 5, 360, "px", "sld_w_sil_fov_" .. curSilWId, function(v) wSil.fovSize = v end)
                        DrawVideoCombo("Alvo (Hitbox)", hitboxOptions, wSil.hitbox, "combo_w_sil_hitbox_" .. curSilWId, function(idx) wSil.hitbox = idx end)
                        DrawVideoCheckbox("Show FOV Circle", wSil.showFov, "chk_w_sil_showfov_" .. curSilWId, function(v) wSil.showFov = v end)
                        DrawVideoCheckbox("Visible Check (Raycast)", wSil.visibleCheck, "chk_w_sil_vischeck_" .. curSilWId, function(v) wSil.visibleCheck = v end)
                        DrawVideoCheckbox("Allow NPC", wSil.allowNpc, "chk_w_sil_allownpc_" .. curSilWId, function(v) wSil.allowNpc = v end)
                    imgui.EndChild()

                ---------------------------------------------------------
                -- ABA 2: VISUALS (Player ESP + Overlays)
                ---------------------------------------------------------
                elseif abaLateral == 2 then
                    imgui.BeginChild("##CardVis1", imgui.ImVec2(larguraColuna, 0), true)
                        imgui.SetCursorPosX(12); imgui.SetCursorPosY(10)
                        if Theme.Fonts.Title then imgui.PushFont(Theme.Fonts.Title) end
                        imgui.TextColored(branco, "Player ESP")
                        if Theme.Fonts.Title then imgui.PopFont() end
                        imgui.Spacing()

                        DrawVideoCheckbox("Master ESP", Config.data.visuals.enabled, "vis_master", function(v)
                            Config.data.visuals.enabled = v
                            if okNotifications then Notifications.info("VISUALS", v and "ESP Ativado" or "ESP Desativado") end
                        end)
                        DrawVideoCheckbox("2D Box ESP", Config.data.visuals.boxEsp, "vis_box", function(v) Config.data.visuals.boxEsp = v end)
                        DrawVideoCheckbox("Corner Box", Config.data.visuals.cornerBox, "vis_corner", function(v) Config.data.visuals.cornerBox = v end)
                        DrawVideoCheckbox("Skeleton ESP", Config.data.visuals.skeleton, "vis_skel", function(v) Config.data.visuals.skeleton = v end)
                        DrawVideoCheckbox("Health Bar", Config.data.visuals.healthBar, "vis_hp", function(v) Config.data.visuals.healthBar = v end)
                        DrawVideoCheckbox("Armor Bar", Config.data.visuals.armorBar, "vis_arm", function(v) Config.data.visuals.armorBar = v end)
                        DrawVideoCheckbox("Player Names", Config.data.visuals.playerNames, "vis_name", function(v) Config.data.visuals.playerNames = v end)
                        DrawVideoCheckbox("Player ID", Config.data.visuals.playerId, "vis_id", function(v) Config.data.visuals.playerId = v end)
                    imgui.EndChild()

                    imgui.SameLine(0, 10)

                    imgui.BeginChild("##CardVis2", imgui.ImVec2(larguraColuna, 0), true)
                        imgui.SetCursorPosX(12); imgui.SetCursorPosY(10)
                        if Theme.Fonts.Title then imgui.PushFont(Theme.Fonts.Title) end
                        imgui.TextColored(branco, "World ESP & Overlays")
                        if Theme.Fonts.Title then imgui.PopFont() end
                        imgui.Spacing()

                        DrawVideoCheckbox("Snaplines", Config.data.visuals.snaplines, "vis_snap", function(v) Config.data.visuals.snaplines = v end)
                        DrawVideoCheckbox("Distance ESP", Config.data.visuals.distance, "vis_dist", function(v) Config.data.visuals.distance = v end)
                        DrawVideoSlider("Max ESP Distance", Config.data.visuals.maxDistance, 50, 400, "m", "sld_vis_dist", function(v) Config.data.visuals.maxDistance = v end)
                    imgui.EndChild()

                ---------------------------------------------------------
                -- ABA 3: WORLD (Clima e Horário)
                ---------------------------------------------------------
                elseif abaLateral == 3 then
                    imgui.BeginChild("##CardWorld", imgui.ImVec2(larguraColuna, 0), true)
                        imgui.SetCursorPosX(12); imgui.SetCursorPosY(10)
                        if Theme.Fonts.Title then imgui.PushFont(Theme.Fonts.Title) end
                        imgui.TextColored(branco, "World Controller")
                        if Theme.Fonts.Title then imgui.PopFont() end
                        imgui.Spacing()

                        DrawVideoCheckbox("Custom Time Enabled", Config.data.world.customTimeEnabled, "w_time_en", function(v) Config.data.world.customTimeEnabled = v end)
                        DrawVideoSlider("Hour (0h - 23h)", Config.data.world.customHour, 0, 23, "h", "w_hour", function(v) Config.data.world.customHour = v end)
                        
                        imgui.Spacing(); imgui.Separator(); imgui.Spacing()
                        
                        DrawVideoCheckbox("Custom Weather Enabled", Config.data.world.customWeatherEnabled, "w_weath_en", function(v) Config.data.world.customWeatherEnabled = v end)
                        DrawVideoSlider("Weather ID", Config.data.world.weatherId, 0, 45, "", "w_wid", function(v) Config.data.world.weatherId = v end)
                    imgui.EndChild()

                ---------------------------------------------------------
                -- ABA 4: VEHICLES
                ---------------------------------------------------------
                elseif abaLateral == 4 then
                    imgui.BeginChild("##CardVeh", imgui.ImVec2(larguraColuna, 0), true)
                        imgui.SetCursorPosX(12); imgui.SetCursorPosY(10)
                        if Theme.Fonts.Title then imgui.PushFont(Theme.Fonts.Title) end
                        imgui.TextColored(branco, "Vehicle Enhancements")
                        if Theme.Fonts.Title then imgui.PopFont() end
                        imgui.Spacing()

                        DrawCheckboxWithKeybind("Speed Boost", Config.data.vehicles.speedBoost, Config.data.vehicles.speedBoostKey, "chk_v_boost", function(v)
                            Config.data.vehicles.speedBoost = v
                        end, function(id, k) Config.data.vehicles.speedBoostKey = k end)
                        
                        DrawCheckboxWithKeybind("Instant Repair (R)", Config.data.vehicles.instantRepair, Config.data.vehicles.instantRepairKey, "chk_v_rep", function(v)
                            Config.data.vehicles.instantRepair = v
                        end, function(id, k) Config.data.vehicles.instantRepairKey = k end)

                        DrawCheckboxWithKeybind("Flip Car (X)", Config.data.vehicles.flipCar, Config.data.vehicles.flipCarKey, "chk_v_flip", function(v)
                            Config.data.vehicles.flipCar = v
                        end, function(id, k) Config.data.vehicles.flipCarKey = k end)

                        DrawVideoCheckbox("Vehicle Godmode", Config.data.vehicles.godmode, "chk_v_god", function(v) Config.data.vehicles.godmode = v end)
                        DrawVideoCheckbox("Engine Always On", Config.data.vehicles.engineAlwaysOn, "chk_v_eng", function(v) Config.data.vehicles.engineAlwaysOn = v end)
                    imgui.EndChild()

                ---------------------------------------------------------
                -- ABA 5: PLAYERS (C-SLIDE & MOVIMENTO)
                ---------------------------------------------------------
                elseif abaLateral == 5 then
                    -- CARD 1: C-Slide & Mecânicas
                    imgui.BeginChild("##CardSlide1", imgui.ImVec2(larguraColuna, 0), true)
                        imgui.SetCursorPosX(12); imgui.SetCursorPosY(10)
                        if Theme.Fonts.Title then imgui.PushFont(Theme.Fonts.Title) end
                        imgui.TextColored(branco, "C-Slide & Movement")
                        if Theme.Fonts.Title then imgui.PopFont() end
                        imgui.Spacing()

                        DrawVideoCheckbox("Script Geral (Master)", Config.data.player.scriptAtivo, "chk_slide_master", function(v) Config.data.player.scriptAtivo = v end)
                        DrawVideoCheckbox("C-Slide Ativo", Config.data.player.cSlideAtivo, "chk_slide_cslide", function(v) Config.data.player.cSlideAtivo = v end)
                        DrawVideoCheckbox("Auto Slide (Quick Switch)", Config.data.player.autoSlideAtivo, "chk_slide_auto", function(v) Config.data.player.autoSlideAtivo = v end)

                        DrawVideoSlider("Duração da Tecla C", Config.data.player.duracaoC, 5, 100, "ms", "sld_slide_durc", function(v) Config.data.player.duracaoC = v end)
                        DrawVideoSlider("Delay pós-tiro", Config.data.player.delayTroca, 0, 250, "ms", "sld_slide_delay", function(v) Config.data.player.delayTroca = v end)
                        
                        imgui.Spacing(); imgui.Separator(); imgui.Spacing()
                        DrawVideoCheckbox("Infinite Stamina", Config.data.player.infiniteStamina, "chk_p_infstam", function(v) Config.data.player.infiniteStamina = v end)
                        DrawVideoCheckbox("No Fall Damage", Config.data.player.noFallDamage, "chk_p_nofall", function(v) Config.data.player.noFallDamage = v end)
                    imgui.EndChild()

                    imgui.SameLine(0, 10)

                    -- CARD 2: Margens por Arma
                    imgui.BeginChild("##CardSlide2", imgui.ImVec2(larguraColuna, 0), true)
                        imgui.SetCursorPosX(12); imgui.SetCursorPosY(10)
                        if Theme.Fonts.Title then imgui.PushFont(Theme.Fonts.Title) end
                        imgui.TextColored(branco, "Weapon Margins")
                        if Theme.Fonts.Title then imgui.PopFont() end
                        imgui.Spacing()

                        DrawVideoSlider("Desert Eagle (Deagle)", Config.data.player.margem_desert, 0, 1000, "ms", "sld_m_deagle", function(v) Config.data.player.margem_desert = v end)
                        DrawVideoSlider("Shotgun", Config.data.player.margem_shot, 0, 1000, "ms", "sld_m_shot", function(v) Config.data.player.margem_shot = v end)
                        DrawVideoSlider("Sniper Rifle", Config.data.player.margem_snp, 0, 1000, "ms", "sld_m_snp", function(v) Config.data.player.margem_snp = v end)
                        DrawVideoSlider("M4 Assault", Config.data.player.margem_m4, 0, 1000, "ms", "sld_m_m4", function(v) Config.data.player.margem_m4 = v end)
                        DrawVideoSlider("AK-47", Config.data.player.margem_ak, 0, 1000, "ms", "sld_m_ak", function(v) Config.data.player.margem_ak = v end)
                    imgui.EndChild()

                ---------------------------------------------------------
                -- ABA 6: EXPLOITS & HUD
                ---------------------------------------------------------
                elseif abaLateral == 6 then
                    imgui.BeginChild("##CardExpl", imgui.ImVec2(larguraColuna, 0), true)
                        imgui.SetCursorPosX(12); imgui.SetCursorPosY(10)
                        if Theme.Fonts.Title then imgui.PushFont(Theme.Fonts.Title) end
                        imgui.TextColored(branco, "Exploits")
                        if Theme.Fonts.Title then imgui.PopFont() end
                        imgui.Spacing()

                        DrawVideoCheckbox("No Recoil", Config.data.misc.noRecoil, "e_norec", function(v) Config.data.misc.noRecoil = v end)
                        DrawVideoCheckbox("No Spread", Config.data.misc.noSpread, "e_nospread", function(v) Config.data.misc.noSpread = v end)
                        DrawVideoCheckbox("Fast Reload", Config.data.misc.fastReload, "e_fastrel", function(v) Config.data.misc.fastReload = v end)
                    imgui.EndChild()

                    imgui.SameLine(0, 10)

                    imgui.BeginChild("##CardHUD", imgui.ImVec2(larguraColuna, 0), true)
                        imgui.SetCursorPosX(12); imgui.SetCursorPosY(10)
                        if Theme.Fonts.Title then imgui.PushFont(Theme.Fonts.Title) end
                        imgui.TextColored(branco, "HUD & Watermark")
                        if Theme.Fonts.Title then imgui.PopFont() end
                        imgui.Spacing()

                        DrawVideoCheckbox("Watermark Superior", Config.data.hud.watermark, "hud_wm", function(v) Config.data.hud.watermark = v end)
                        DrawVideoCheckbox("Coordenadas X/Y/Z", Config.data.hud.coords, "hud_coords", function(v) Config.data.hud.coords = v end)
                        DrawVideoCheckbox("Active Features List", Config.data.hud.activeFeatures, "hud_active", function(v) Config.data.hud.activeFeatures = v end)
                    imgui.EndChild()

                ---------------------------------------------------------
                -- ABA 7: CONFIGS & PERFIS
                ---------------------------------------------------------
                elseif abaLateral == 7 then
                    imgui.BeginChild("##CardCfg", imgui.ImVec2(larguraColuna, 0), true)
                        imgui.SetCursorPosX(12); imgui.SetCursorPosY(10)
                        if Theme.Fonts.Title then imgui.PushFont(Theme.Fonts.Title) end
                        imgui.TextColored(branco, "Config Profiles")
                        if Theme.Fonts.Title then imgui.PopFont() end
                        imgui.Spacing()

                        imgui.TextColored(Theme.Colors.TextSecondary, u8"Perfil Ativo: " .. string.upper(Config.currentProfile))
                        imgui.Spacing()

                        for idx, pName in ipairs(Config.profilesList) do
                            local isSelected = (Config.currentProfile == pName)
                            if imgui.Button((isSelected and "> " or "  ") .. string.upper(pName) .. "##prof_" .. idx, imgui.ImVec2(-1, 28)) then
                                Config.load(pName)
                                if okNotifications then Notifications.success("CONFIG", "Perfil " .. string.upper(pName) .. " Carregado") end
                            end
                        end
                        
                        imgui.Spacing()
                        if imgui.Button("Salvar Configuracoes##btn_save", imgui.ImVec2(-1, 30)) then
                            Config.save()
                            if okNotifications then Notifications.success("CONFIG", "Configuracoes Salvas!") end
                        end

                        if imgui.Button("Resetar Padrao##btn_reset", imgui.ImVec2(-1, 30)) then
                            Config.reset()
                            if okNotifications then Notifications.warning("CONFIG", "Configuracao Resetada") end
                        end
                    imgui.EndChild()

                    imgui.SameLine(0, 10)

                    imgui.BeginChild("##CardCfg2", imgui.ImVec2(larguraColuna, 0), true)
                        imgui.SetCursorPosX(12); imgui.SetCursorPosY(10)
                        if Theme.Fonts.Title then imgui.PushFont(Theme.Fonts.Title) end
                        imgui.TextColored(branco, "System & Keys")
                        if Theme.Fonts.Title then imgui.PopFont() end
                        imgui.Spacing()

                        imgui.TextColored(Theme.Colors.TextSecondary, u8"Menu Keybind:")
                        if okKeybinds then
                            Keybinds.drawKeybindButton("menu_key", Config.data.system.menuKey, function(id, k) Config.data.system.menuKey = k end, 120)
                        end
                        
                        imgui.Spacing(); imgui.Separator(); imgui.Spacing()
                        imgui.TextColored(Theme.Colors.TextSecondary, u8"Comandos de Chat:")
                        imgui.TextColored(branco, "/sml  |  /somalia  |  /slide  |  /menu")
                        imgui.Spacing()
                        imgui.TextColored(Theme.Colors.TextMuted, "Somalia Client Edition v1.3.0")
                    imgui.EndChild()
                end

                imgui.PopStyleVar(1)
            imgui.EndChild()

            imgui.End()
        end
    end
end

-------------------------------------------------------------------------
-- MAIN THREAD DO MOONLOADER - BOOT SEGURO E GATING VISUAL
-------------------------------------------------------------------------
function main()
    print("[SOMALIA][BOOT] 01 - main iniciado")
    
    local isSamp = isSampLoaded()
    print("[SOMALIA][BOOT] 02 - status SA-MP carregado: " .. tostring(isSamp))
    print("[SOMALIA][BOOT] 03 - status ImGui carregado: " .. tostring(okImgui))
    print("[SOMALIA][BOOT] 04 - config e temas carregados")

    -- Aguarda o SA-MP ficar completamente pronto
    while not isSampAvailable() do 
        wait(100) 
    end
    print("[SOMALIA][BOOT] 05 - SA-MP esta pronto e disponivel!")
    print("[SOMALIA][BOOT] 06 - MODO VISUAL (ESP + FOV AIMBOT + FOV SILENT ATIVOS)")

    local function toggleMenu()
        if okImgui then
            janelaAberta.v = not janelaAberta.v
            sampAddChatMessage("{0080FF}[SOMALIA]{FFFFFF} Menu alternado para: " .. tostring(janelaAberta.v), -1)
        else
            sampAddChatMessage("{FF0000}[SOMALIA ERRO]{FFFFFF} Biblioteca 'imgui' nao encontrada em moonloader/lib!", -1)
        end
    end

    local function commandHandler()
        sampAddChatMessage("{0080FF}[SOMALIA]{FFFFFF} COMANDO DETECTADO!", -1)
        toggleMenu()
    end

    sampRegisterChatCommand("sml", commandHandler)
    sampRegisterChatCommand("somalia", commandHandler)
    sampRegisterChatCommand("slide", commandHandler)
    sampRegisterChatCommand("menu", commandHandler)

    if okTheme and Theme.applyStyle then Theme.applyStyle() end

    if okImgui then
        sampAddChatMessage("{0080FF}[SOMALIA]{FFFFFF} Somalia Carregado! Pressione {0080FF}F5{FFFFFF} ou digite {0080FF}/sml{FFFFFF}.", -1)
        if okNotifications then Notifications.success("SOMALIA", "Carregado com sucesso! [F5]") end
        print("[SOMALIA][BOOT] 07 - Somalia inicializado com sucesso! Atalho F5 pronto.")
    end

    print("[SOMALIA][BOOT] 08 - entrando no loop principal")

    while true do
        wait(0)

        -- Escuta de tecla para redefinição de keybinds interativos (apenas se fora de diálogos/chat)
        if okKeybinds and Keybinds.processListening then
            Keybinds.processListening(function(id, newKey)
                if id == "menu_key" then
                    Config.data.system.menuKey = newKey
                elseif id == "chk_aim_master" then
                    Config.data.aim.key = newKey
                elseif id == "chk_sil_master" then
                    Config.data.silent.key = newKey
                elseif id == "chk_v_boost" then
                    Config.data.vehicles.speedBoostKey = newKey
                elseif id == "chk_v_rep" then
                    Config.data.vehicles.instantRepairKey = newKey
                elseif id == "chk_v_flip" then
                    Config.data.vehicles.flipCarKey = newKey
                end
                if okConfig and Config.save then Config.save() end
                if okNotifications then Notifications.info("KEYBIND", "Atalho atualizado: " .. Keybinds.getKeyName(newKey)) end
            end)
        end

        -- Abertura e fechamento do Menu via Keybind configurável ou F5 direto (ignora durante diálogos/chat)
        local menuKey = (okConfig and Config.data.system.menuKey) or 0x74 -- F5
        if isKeyJustPressed(menuKey) or isKeyJustPressed(0x74) then
            toggleMenu()
        end

        local inGame = isPlayerReady()
        local visualsActive = inGame and Config.data.visuals and Config.data.visuals.enabled or false
        local curW = okAim and Aim.getCurrentWeaponId and Aim.getCurrentWeaponId() or 0
        
        -- Checagem de FOV Aimbot e Silent Aim ativos para a arma atual
        local wAim = okConfig and Config.getAimWeapon and Config.getAimWeapon(curW) or nil
        local wSil = okConfig and Config.getSilentWeapon and Config.getSilentWeapon(curW) or nil
        local fovAimActive = inGame and (curW > 0) and (Config.data.aim and Config.data.aim.globalEnabled and wAim and wAim.enabled and wAim.showFov) or false
        local fovSilActive = inGame and (curW > 0) and (Config.data.silent and Config.data.silent.globalEnabled and wSil and wSil.enabled and wSil.showFov) or false
        local fovAnyActive = fovAimActive or fovSilActive

        -- CONTROLE SEGURO DE IMGUI: Processa frames somente se menu aberto OU jogador spawnado com ESP/FOV
        if okImgui then
            imgui.Process = janelaAberta.v or (inGame and (visualsActive or fovAnyActive))
            imgui.ShowCursor = janelaAberta.v
            imgui.DisableInput = not janelaAberta.v
        end

        -- PROCESSAMENTO DE GAMEPLAY (PERMANECE DESATIVADO NESTA ETAPA)
        if ENABLE_GAMEPLAY_RUNTIME and inGame then
            if okPlayer and Player.process then pcall(Player.process, Config.data) end
            if okAim and Aim.process then pcall(Aim.process, Config.data) end
            if okVehicles and Vehicles.process then pcall(Vehicles.process, Config.data) end
            if okWorld and World.process then pcall(World.process, Config.data) end
            if okMisc and Misc.process then pcall(Misc.process, Config.data) end
        end

        -- DIAGNÓSTICO THROTTLED A CADA 2.0 SEGUNDOS
        lastDebugTick = lastDebugTick or 0
        local nowClock = os.clock()
        if (nowClock - lastDebugTick) >= 2.0 then
            lastDebugTick = nowClock
            print(string.format(
                "[SOMALIA][STATUS] InGame: %s | Menu: %s | ESP: %s | FOV Aim: %s | FOV Silent: %s (Arma %d)",
                tostring(inGame),
                tostring(janelaAberta.v),
                tostring(visualsActive),
                tostring(fovAimActive),
                tostring(fovSilActive),
                curW
            ))
        end
    end
end