--[[
    Somalia - Player & Movement Module
    Preserva rigorosamente a lógica original de C-Slide e Auto-Slide (Quick Switch)
    Proteção total contra execução antes do spawn no SA-MP
]]

local okVkeys, vkeys = pcall(require, 'vkeys')
local okMemory, memory = pcall(require, 'memory')

local Player = {}

function Player.process(config)
    if not config or not config.player then return end
    
    -- Proteção rigorosa contra execução antes do spawn ou durante conexão
    if isSampLoaded() and (not isSampAvailable() or not sampIsLocalPlayerSpawned()) then
        return
    end
    
    if not doesCharExist(playerPed) or isCharDead(playerPed) or isCharInAnyCar(playerPed) then
        return
    end
    
    -- 1. INFINITE STAMINA (Trava a stamina máxima em memória)
    if config.player.infiniteStamina and okMemory then
        pcall(function()
            memory.setfloat(0xB7CEE4, 100.0)
        end)
    end
    
    -- 2. NO FALL DAMAGE (Imunidade a dano por impacto/queda)
    if config.player.noFallDamage then
        pcall(function()
            setCharProofs(playerPed, false, false, false, true, false)
        end)
    end
    
    -- 3. SPRINT SPEED MULTIPLIER
    if config.player.sprintSpeed and config.player.sprintSpeed > 1.05 then
        local isSprinting = isKeyDown(0x20) or isKeyDown(0x10) -- Space ou Shift
        if isSprinting then
            pcall(function()
                setCharAnimSpeed(playerPed, "run_civi", config.player.sprintSpeed)
                setCharAnimSpeed(playerPed, "sprint_civi", config.player.sprintSpeed)
                setCharAnimSpeed(playerPed, "run_player", config.player.sprintSpeed)
                setCharAnimSpeed(playerPed, "sprint_panic", config.player.sprintSpeed)
            end)
        end
    end
    
end

return Player
