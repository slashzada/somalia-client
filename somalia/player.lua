--[[
    Somalia - Player & Movement Module
    Preserva rigorosamente a lógica original de C-Slide e Auto-Slide (Quick Switch)
    Proteção total contra execução antes do spawn no SA-MP
]]

local okVkeys, vkeys = pcall(require, 'vkeys')
local okMemory, memory = pcall(require, 'memory')

local Player = {
    mirandoAnteriormente = false,
    tempoUltimoTiro = 0,
    executandoAcao = false,
    idParaChave = {
        [34] = "margem_snp",
        [24] = "margem_desert",
        [31] = "margem_m4",
        [30] = "margem_ak",
        [25] = "margem_shot"
    }
}

function Player.process(config)
    if not config or not config.player then return end
    
    -- Proteção rigorosa contra execução antes do spawn ou durante conexão
    if isSampLoaded() and (not isSampAvailable() or not sampIsLocalPlayerSpawned()) then
        Player.mirandoAnteriormente = false
        Player.executandoAcao = false
        return
    end
    
    if not doesCharExist(playerPed) or isCharDead(playerPed) or isCharInAnyCar(playerPed) then
        Player.mirandoAnteriormente = false
        Player.executandoAcao = false
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
    
    -- 4. LÓGICA ORIGINAL DE C-SLIDE E AUTO-SLIDE (PRESERVADA 1:1)
    if config.player.scriptAtivo then
        if isCharShooting(playerPed) or isKeyDown(0x01) then -- Mouse 1
            Player.tempoUltimoTiro = os.clock()
        end

        local mirandoAgora = isKeyDown(0x02) -- Mouse 2

        -- Detecção instantânea da transição: MIRANDO -> SOLTOU A MIRA
        if Player.mirandoAnteriormente and not mirandoAgora then
            if not sampIsChatInputActive() and not sampIsDialogActive() then
                local armaAtual = getCurrentCharWeapon(playerPed)
                local chaveArma = Player.idParaChave[armaAtual]
                local margem = (chaveArma and config.player[chaveArma]) or 0
                local segurandoMovimento = isKeyDown(0x41) or isKeyDown(0x44) or isKeyDown(0x57) or isKeyDown(0x53) or (getCharSpeed(playerPed) > 0.4) -- A, D, W, S

                if not Player.executandoAcao then
                    Player.executandoAcao = true
                    lua_thread.create(function()
                        local tempoPassado = (os.clock() - Player.tempoUltimoTiro) * 1000
                        if margem > 0 and tempoPassado < margem then
                            wait(margem - tempoPassado)
                        end

                        -- 1. C-SLIDE FLUIDO (Agachar e Deslizar)
                        if config.player.cSlideAtivo and segurandoMovimento then
                            local duracao = math.max(10, config.player.duracaoC or 10)
                            setGameKeyState(18, 255)
                            setGameKeyState(1, 255)
                            pcall(function() setVirtualKeyDown(0x43, true) end) -- C
                            wait(duracao)
                            setGameKeyState(18, 0)
                            setGameKeyState(1, 0)
                            pcall(function() setVirtualKeyDown(0x43, false) end)
                        end

                        -- 2. AUTO SLIDE / TROCA RÁPIDA PARA O SOCO (SLOT 0)
                        if config.player.autoSlideAtivo and okMemory then
                            local delayTroca = config.player.delayTroca or 0
                            if delayTroca > 0 then wait(delayTroca) end

                            if doesCharExist(playerPed) and not isCharDead(playerPed) and not isCharInAnyCar(playerPed) and not sampIsChatInputActive() and not sampIsDialogActive() then
                                local ptr = getCharPointer(playerPed)
                                if ptr and ptr ~= 0 then
                                    local curSlot = memory.getuint8(ptr + 0x718)
                                    if curSlot > 0 then
                                        memory.setuint8(ptr + 0x718, 0)
                                        setGameKeyState(16, 255)
                                        pcall(function() setVirtualKeyDown(0x51, true) end) -- Q
                                        wait(30)
                                        setGameKeyState(16, 0)
                                        pcall(function() setVirtualKeyDown(0x51, false) end)
                                    end
                                end
                            end
                        end

                        wait(30)
                        Player.executandoAcao = false
                    end)
                end
            end
        end
        Player.mirandoAnteriormente = mirandoAgora
    else
        Player.mirandoAnteriormente = false
    end
end

return Player
