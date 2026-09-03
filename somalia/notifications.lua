--[[
    Somalia - Notifications System
    Toasts modernos animados no canto inferior/superior com fade, cores de status e anti-spam
]]

local okImgui, imgui = pcall(require, 'imgui')
local Theme = nil

local Notifications = {
    queue = {},
    maxDisplay = 5,
    defaultDuration = 3.5,
    lastMessages = {}
}

function Notifications.init(themeModule)
    Theme = themeModule
end

function Notifications.add(title, message, nType, duration)
    local now = os.clock()
    local key = (title or "") .. ":" .. (message or "")
    
    if Notifications.lastMessages[key] and (now - Notifications.lastMessages[key]) < 1.0 then
        return
    end
    Notifications.lastMessages[key] = now
    
    duration = duration or Notifications.defaultDuration
    
    table.insert(Notifications.queue, {
        title = title or "SOMALIA",
        message = message or "",
        nType = nType or "info",
        startTime = now,
        duration = duration,
        alpha = 0.0,
        slideX = 30.0
    })
    
    if #Notifications.queue > Notifications.maxDisplay then
        table.remove(Notifications.queue, 1)
    end
end

function Notifications.success(title, msg, dur)
    Notifications.add(title, msg, "success", dur)
end

function Notifications.error(title, msg, dur)
    Notifications.add(title, msg, "error", dur)
end

function Notifications.warning(title, msg, dur)
    Notifications.add(title, msg, "warning", dur)
end

function Notifications.info(title, msg, dur)
    Notifications.add(title, msg, "info", dur)
end

function Notifications.render(customDrawList)
    if not okImgui or #Notifications.queue == 0 then return end
    
    local now = os.clock()
    local sw, sh = getScreenResolution()
    local drawList = customDrawList or (imgui.GetWindowDrawList and imgui.GetWindowDrawList())
    if not drawList then return end
    
    local toastW = 240
    local toastH = 48
    local margin = 16
    local startY = sh - margin - toastH
    local startX = sw - margin - toastW
    
    local i = #Notifications.queue
    while i >= 1 do
        local toast = Notifications.queue[i]
        local elapsed = now - toast.startTime
        local remaining = toast.duration - elapsed
        
        if remaining <= 0 then
            table.remove(Notifications.queue, i)
        else
            local fadeInTime = 0.25
            local fadeOutTime = 0.35
            
            if elapsed < fadeInTime then
                local prog = elapsed / fadeInTime
                toast.alpha = prog
                toast.slideX = (1.0 - prog) * 40.0
            elseif remaining < fadeOutTime then
                local prog = remaining / fadeOutTime
                toast.alpha = prog
                toast.slideX = (1.0 - prog) * 40.0
            else
                toast.alpha = 1.0
                toast.slideX = 0.0
            end
            
            local curX = startX + toast.slideX
            local curY = startY - ((#Notifications.queue - i) * (toastH + 8))
            
            local a = math.floor(toast.alpha * 255)
            local bgCol = imgui.ImColor(10, 12, 17, math.floor(a * 0.94)):GetU32()
            local borderCol = imgui.ImColor(28, 34, 46, a):GetU32()
            
            local accentCol = imgui.ImColor(0, 128, 255, a):GetU32()
            local iconChar = "i"
            if toast.nType == "success" then
                accentCol = imgui.ImColor(30, 215, 96, a):GetU32()
                iconChar = "OK"
            elseif toast.nType == "error" then
                accentCol = imgui.ImColor(255, 60, 60, a):GetU32()
                iconChar = "X"
            elseif toast.nType == "warning" then
                accentCol = imgui.ImColor(255, 174, 25, a):GetU32()
                iconChar = "!"
            end
            
            drawList:AddRectFilled(imgui.ImVec2(curX, curY), imgui.ImVec2(curX + toastW, curY + toastH), bgCol, 6.0)
            drawList:AddRect(imgui.ImVec2(curX, curY), imgui.ImVec2(curX + toastW, curY + toastH), borderCol, 6.0, 15, 1.2)
            drawList:AddRectFilled(imgui.ImVec2(curX, curY), imgui.ImVec2(curX + 4, curY + toastH), accentCol, 2.0)
            
            drawList:AddCircleFilled(imgui.ImVec2(curX + 22, curY + (toastH / 2)), 10.0, accentCol)
            local iSize = imgui.CalcTextSize(iconChar)
            drawList:AddText(imgui.ImVec2(curX + 22 - (iSize.x / 2), curY + (toastH / 2) - (iSize.y / 2)), imgui.ImColor(255, 255, 255, a):GetU32(), iconChar)
            
            drawList:AddText(imgui.ImVec2(curX + 40, curY + 8), imgui.ImColor(255, 255, 255, a):GetU32(), toast.title)
            drawList:AddText(imgui.ImVec2(curX + 40, curY + 24), imgui.ImColor(170, 182, 205, a):GetU32(), toast.message)
        end
        i = i - 1
    end
end

return Notifications
