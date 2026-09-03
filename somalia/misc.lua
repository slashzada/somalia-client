--[[
    Somalia - Misc & Exploits Module
    No Recoil, No Spread e Fast Reload com patches de memória seguros
]]

local okMemory, memory = pcall(require, 'memory')

local Misc = {}

function Misc.process(config)
    if not config or not config.misc or not okMemory then return end
    if isCharDead(playerPed) then return end
    
    -- 1. NO RECOIL / NO SPREAD
    if config.misc.noRecoil or config.misc.noSpread then
        pcall(function()
            local camPtr = 0xB6F248
            if camPtr and camPtr ~= 0 then
                memory.setfloat(0xB7CD98, 0.0) -- Camera Shake
            end
        end)
    end
    
    -- 2. FAST RELOAD
    if config.misc.fastReload then
        if isCharShooting(playerPed) then
            pcall(function()
                local weapon = getCurrentCharWeapon(playerPed)
                local ammo = getAmmoInCharWeapon(playerPed, weapon)
                if ammo <= 1 then
                    setCharAmmo(playerPed, weapon, 999)
                end
            end)
        end
    end
end

return Misc
