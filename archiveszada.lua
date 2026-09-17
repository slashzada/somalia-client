local vkeys = require 'vkeys'
local inicfg = require 'inicfg'

local configFile = "AutoSlideConfig.ini"
local configData = {
    settings = {
        scriptAtivo = false,
        margem_snp = 550,
        margem_desert = 0,
        margem_m4 = 0,
        margem_ak = 0,
        margem_shot = 0
    }
}

-- Carrega ou cria o arquivo de configuração de forma segura
local loadedConfig = inicfg.load(configData, configFile)
if not loadedConfig then loadedConfig = configData end
inicfg.save(loadedConfig, configFile)

local scriptAtivo = loadedConfig.settings.scriptAtivo
local mirandoAnteriormente = false 
local tempoUltimoTiro = 0 

local nomesParaIds = {
    ["snp"] = "margem_snp", ["sniper"] = "margem_snp",
    ["desert"] = "margem_desert", ["deagle"] = "margem_desert",
    ["m4"] = "margem_m4",
    ["ak"] = "margem_ak", ["ak47"] = "margem_ak",
    ["shot"] = "margem_shot"
}

local idParaChave = {
    [34] = "margem_snp",
    [24] = "margem_desert",
    [31] = "margem_m4",
    [30] = "margem_ak",
    [25] = "margem_shot"
}

function main()
    if not isSampLoaded() or not isSampfuncsLoaded() then return end
    while not isSampAvailable() do wait(100) end

    sampRegisterChatCommand("slx", function(arg)
        local armaNome, valStr = arg:match("(%a+)%s+(%d+)")
        if armaNome and valStr then
            local chave = nomesParaIds[armaNome:lower()]
            if chave then
                loadedConfig.settings[chave] = tonumber(valStr)
                inicfg.save(loadedConfig, configFile)
                sampAddChatMessage("{00FF00}[Slide-Save]{FFFFFF} " .. armaNome:upper() .. " atualizada para: {FFFF00}" .. valStr .. "ms", -1)
            else
                sampAddChatMessage("{FF0000}[Erro]{FFFFFF} Arma nao reconhecida.", -1)
            end
        else
            sampAddChatMessage("{FF0000}[Erro]{FFFFFF} Use: /slx [arma] [ms]", -1)
        end
    end)

    sampRegisterChatCommand("slide", function() alternarScript() end)

    while true do
        wait(0) 

        if wasKeyPressed(vkeys.VK_F5) then
            alternarScript()
        end

        if scriptAtivo then
            -- Correção do crash: Lê o botão esquerdo do mouse + botão direito para armas automáticas
            if isCharShooting(playerPed) or (isKeyDown(vkeys.VK_LBUTTON) and isKeyDown(vkeys.VK_RBUTTON)) then
                tempoUltimoTiro = os.clock()
            end

            local mirandoAgora = isKeyDown(vkeys.VK_RBUTTON)

            if mirandoAnteriormente and not mirandoAgora then
                if not sampIsChatInputActive() and not sampIsDialogActive() and (isKeyDown(vkeys.VK_A) or isKeyDown(vkeys.VK_D)) then
                    
                    local armaAtual = getCurrentCharWeapon(playerPed)
                    local chaveArma = idParaChave[armaAtual]
                    local margem = (chaveArma and loadedConfig.settings[chaveArma]) or 0

                    lua_thread.create(function()
                        local tempoPassado = (os.clock() - tempoUltimoTiro) * 1000
                        
                        if margem > 0 then
                            if tempoPassado < margem then 
                                wait(margem - tempoPassado) 
                            end
                            wait(math.random(5, 15)) 
                        end
                        
                        setGameKeyState(18, 255) 
                        if margem > 0 then wait(20) else wait(0) end
                        setGameKeyState(18, 0)
                    end)
                end
            end
            mirandoAnteriormente = mirandoAgora
        end
    end
end

function alternarScript()
    scriptAtivo = not scriptAtivo
    loadedConfig.settings.scriptAtivo = scriptAtivo
    inicfg.save(loadedConfig, configFile)
    sampAddChatMessage(scriptAtivo and "{00FF00}[Slide] ON" or "{FF0000}[Slide] OFF", -1)
end

function wasKeyPressed(key)
    if isKeyDown(key) then
        local t = os.clock()
        while isKeyDown(key) do 
            if os.clock() - t > 0.5 then break end 
            wait(0) 
        end
        return true
    end
    return false
end