--[[
    Somalia - Universal Keybind System
    Captura e gerenciamento de teclas do teclado e mouse sem loops bloqueantes
    Proteção contra interferência em diálogos e chat do SA-MP
]]

local okVkeys, vkeys = pcall(require, 'vkeys')
local okImgui, imgui = pcall(require, 'imgui')

local Keybinds = {
    listeningFor = nil,
    keyStates = {},
    keyNames = {
        [0x01] = "Mouse 1",
        [0x02] = "Mouse 2",
        [0x04] = "Mouse 3",
        [0x05] = "Mouse 4",
        [0x06] = "Mouse 5",
        [0x08] = "Backspace",
        [0x09] = "Tab",
        [0x0D] = "Enter",
        [0x10] = "Shift",
        [0x11] = "Ctrl",
        [0x12] = "Alt",
        [0x13] = "Pause",
        [0x14] = "Caps Lock",
        [0x1B] = "Escape",
        [0x20] = "Space",
        [0x21] = "Page Up",
        [0x22] = "Page Down",
        [0x23] = "End",
        [0x24] = "Home",
        [0x25] = "Left",
        [0x26] = "Up",
        [0x27] = "Right",
        [0x28] = "Down",
        [0x2D] = "Insert",
        [0x2E] = "Delete",
        [0x70] = "F1",
        [0x71] = "F2",
        [0x72] = "F3",
        [0x73] = "F4",
        [0x74] = "F5",
        [0x75] = "F6",
        [0x76] = "F7",
        [0x77] = "F8",
        [0x78] = "F9",
        [0x79] = "F10",
        [0x7A] = "F11",
        [0x7B] = "F12"
    }
}

for i = 0x30, 0x39 do
    Keybinds.keyNames[i] = string.char(i)
end
for i = 0x41, 0x5A do
    Keybinds.keyNames[i] = string.char(i)
end
for i = 0x60, 0x69 do
    Keybinds.keyNames[i] = "Num " .. (i - 0x60)
end

function Keybinds.getKeyName(keyCode)
    if not keyCode or keyCode == 0 then return "None" end
    return Keybinds.keyNames[keyCode] or ("Key 0x" .. string.format("%X", keyCode))
end

function Keybinds.isJustPressed(keyCode)
    if not keyCode or keyCode == 0 then return false end
    if isSampLoaded() and (sampIsDialogActive() or sampIsChatInputActive()) then return false end
    local isDown = isKeyDown(keyCode)
    local wasDown = Keybinds.keyStates[keyCode] or false
    Keybinds.keyStates[keyCode] = isDown
    return isDown and not wasDown
end

function Keybinds.isPressed(keyCode)
    if not keyCode or keyCode == 0 then return false end
    if isSampLoaded() and (sampIsDialogActive() or sampIsChatInputActive()) then return false end
    return isKeyDown(keyCode)
end

function Keybinds.processListening(onKeyBound)
    if not Keybinds.listeningFor then return end
    if isSampLoaded() and (sampIsDialogActive() or sampIsChatInputActive()) then return end
    
    if isKeyDown(0x1B) then -- VK_ESCAPE
        local id = Keybinds.listeningFor
        Keybinds.listeningFor = nil
        if onKeyBound then onKeyBound(id, 0) end
        return
    end
    
    for k = 1, 255 do
        if k ~= 0x1B and isKeyDown(k) then
            local id = Keybinds.listeningFor
            Keybinds.listeningFor = nil
            if onKeyBound then onKeyBound(id, k) end
            return
        end
    end
end

function Keybinds.drawKeybindButton(idStr, currentKey, onKeyChange, width)
    if not okImgui then return false end
    
    width = width or 75
    local isListening = (Keybinds.listeningFor == idStr)
    local label = isListening and "[ ... ]" or ("[ " .. Keybinds.getKeyName(currentKey) .. " ]")
    
    local pos = imgui.GetCursorScreenPos()
    local drawList = imgui.GetWindowDrawList()
    
    imgui.PushID("kb_" .. idStr)
    local clicked = imgui.InvisibleButton("##btn_" .. idStr, imgui.ImVec2(width, 20))
    local hovered = imgui.IsItemHovered()
    imgui.PopID()
    
    if clicked then
        if Keybinds.listeningFor == idStr then
            Keybinds.listeningFor = nil
        else
            Keybinds.listeningFor = idStr
        end
    end
    
    local bgCol = isListening and imgui.ImColor(0, 100, 200, 200):GetU32() or (hovered and imgui.ImColor(25, 32, 45, 255):GetU32() or imgui.ImColor(14, 18, 26, 255):GetU32())
    local borderCol = isListening and imgui.ImColor(0, 180, 255, 255):GetU32() or (hovered and imgui.ImColor(0, 128, 255, 180):GetU32() or imgui.ImColor(35, 45, 60, 200):GetU32())
    local txtCol = isListening and imgui.ImColor(255, 255, 255, 255):GetU32() or (hovered and imgui.ImColor(0, 160, 255, 255):GetU32() or imgui.ImColor(140, 158, 185, 255):GetU32())
    
    drawList:AddRectFilled(pos, imgui.ImVec2(pos.x + width, pos.y + 20), bgCol, 4.0)
    drawList:AddRect(pos, imgui.ImVec2(pos.x + width, pos.y + 20), borderCol, 4.0)
    
    local txtSize = imgui.CalcTextSize(label)
    drawList:AddText(imgui.ImVec2(pos.x + (width - txtSize.x) / 2, pos.y + (20 - txtSize.y) / 2), txtCol, label)
    
    return clicked
end

return Keybinds
