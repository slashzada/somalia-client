local vkeys = require 'vkeys'
local inicfg = require 'inicfg'
local ffi = require 'ffi'

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

-- Carrega config de delays do arquivo
local loadedConfig = inicfg.load(configData, configFile)
if not loadedConfig then loadedConfig = configData end
-- Sempre inicia DESATIVADO no boot do GTA para respeitar o controle do menu Somalia
loadedConfig.settings.scriptAtivo = false
pcall(function() inicfg.save(loadedConfig, configFile) end)

local scriptAtivo = false
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

-- PONTE DE MEMORIA $7501 (0x00A49960 + 0x7534)
-- Controlada com precisao absoluta pelo SomaliaNative
local function isSlideEnabled()
    pcall(function()
        local ptr = ffi.cast("uint32_t*", 0x00A49960 + 0x7534)
        if ptr[0] == 1 then
            scriptAtivo = true
        elseif ptr[0] == 0 then
            scriptAtivo = false
        end
    end)
    return scriptAtivo
end

local function getMargin(armaAtual)
    local margin = 0
    pcall(function()
        local pDelays = ffi.cast("int32_t*", 0x00A49960 + 0x7538)
        if armaAtual == 34 then margin = pDelays[0]      -- Sniper ($7502)
        elseif armaAtual == 24 then margin = pDelays[1]  -- Desert Eagle ($7503)
        elseif armaAtual == 25 then margin = pDelays[2]  -- Shotgun ($7504)
        elseif armaAtual == 31 then margin = pDelays[3]  -- M4 ($7505)
        elseif armaAtual == 30 then margin = pDelays[4]  -- AK-47 ($7506)
        end
    end)
    if margin <= 0 then
        local chaveArma = idParaChave[armaAtual]
        margin = (chaveArma and loadedConfig.settings[chaveArma]) or 0
    end
    return margin
end

function main()
    if not isSampLoaded() or not isSampfuncsLoaded() then return end
    while not isSampAvailable() do wait(100) end

    sampRegisterChatCommand("slx", function(arg)
        local armaNome, valStr = arg:match("(%a+)%s+(%d+)")
        if armaNome and valStr then
            local chave = nomesParaIds[armaNome:lower()]
            if chave then
                local valNum = tonumber(valStr)
                loadedConfig.settings[chave] = valNum
                pcall(function() inicfg.save(loadedConfig, configFile) end)
                pcall(function()
                    local pDelays = ffi.cast("int32_t*", 0x00A49960 + 0x7538)
                    if chave == "margem_snp" then pDelays[0] = valNum
                    elseif chave == "margem_desert" then pDelays[1] = valNum
                    elseif chave == "margem_shot" then pDelays[2] = valNum
                    elseif chave == "margem_m4" then pDelays[3] = valNum
                    elseif chave == "margem_ak" then pDelays[4] = valNum
                    end
                end)
                sampAddChatMessage("{00FF00}[Slide-Save]{FFFFFF} " .. armaNome:upper() .. " atualizada para: {FFFF00}" .. valStr .. "ms", -1)
            else
                sampAddChatMessage("{FF0000}[Erro]{FFFFFF} Arma nao reconhecida.", -1)
            end
        else
            sampAddChatMessage("{FF0000}[Erro]{FFFFFF} Use: /slx [arma] [ms]", -1)
        end
    end)

    sampRegisterChatCommand("slide", function()
        alternarScript()
    end)

    while true do
        wait(0) 

        local ativo = isSlideEnabled()
        if ativo then
            if isCharShooting(playerPed) or (isKeyDown(vkeys.VK_LBUTTON) and isKeyDown(vkeys.VK_RBUTTON)) then
                tempoUltimoTiro = os.clock()
            end

            local mirandoAgora = isKeyDown(vkeys.VK_RBUTTON)

            if mirandoAnteriormente and not mirandoAgora then
                if not sampIsChatInputActive() and not sampIsDialogActive() and (isKeyDown(vkeys.VK_A) or isKeyDown(vkeys.VK_D)) then
                    
                    local armaAtual = getCurrentCharWeapon(playerPed)
                    local margem = getMargin(armaAtual)

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
        else
            mirandoAnteriormente = false
        end
    end
end

function alternarScript()
    scriptAtivo = not scriptAtivo
    pcall(function()
        local ptr = ffi.cast("uint32_t*", 0x00A49960 + 0x7534)
        ptr[0] = scriptAtivo and 1 or 0
    end)
    loadedConfig.settings.scriptAtivo = scriptAtivo
    pcall(function() inicfg.save(loadedConfig, configFile) end)
end
