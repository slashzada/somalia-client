local vkeys = require 'vkeys'
local inicfg = require 'inicfg'
local ffi = require 'ffi'

ffi.cdef[[
    typedef struct {
        uint32_t magic;         // 0x534F4D41 ("SOMA")
        uint8_t  enabled;       // 1 = ON, 0 = OFF
        uint8_t  luaActive;     // 1 = Lua running
        uint8_t  _pad[2];
        int32_t  margin_snp;    // Delay Sniper (ms)
        int32_t  margin_desert; // Delay Deagle (ms)
        int32_t  margin_shot;   // Delay Shotgun (ms)
        int32_t  margin_m4;     // Delay M4 (ms)
        int32_t  margin_ak;     // Delay AK-47 (ms)
        uint32_t lastHeartbeat; // Tick count
    } LuaSlideBridgeStruct;

    void* OpenFileMappingA(uint32_t dwDesiredAccess, int bInheritHandle, const char* lpName);
    void* MapViewOfFile(void* hFileMappingObject, uint32_t dwDesiredAccess, uint32_t dwFileOffsetHigh, uint32_t dwFileOffsetLow, size_t dwNumberOfBytesToMap);
    int CloseHandle(void* hObject);
    void* GetModuleHandleA(const char* lpModuleName);
    void* GetProcAddress(void* hModule, const char* lpProcName);
    uint32_t GetTickCount(void);
]]

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

-- Carrega ou cria o arquivo de configuracao de forma segura
local loadedConfig = inicfg.load(configData, configFile)
if not loadedConfig then loadedConfig = configData end
pcall(function() inicfg.save(loadedConfig, configFile) end)

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

local s_Bridge = nil
local function getBridge()
    if s_Bridge ~= nil then return s_Bridge end
    
    -- 1. Named Shared Memory no Windows
    local FILE_MAP_ALL_ACCESS = 0xF001F
    local hMap = ffi.C.OpenFileMappingA(FILE_MAP_ALL_ACCESS, 0, "SomaliaSlideBridge")
    if hMap ~= nil then
        local pBuf = ffi.C.MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, ffi.sizeof("LuaSlideBridgeStruct"))
        if pBuf ~= nil then
            local ptr = ffi.cast("LuaSlideBridgeStruct*", pBuf)
            if ptr.magic == 0x534F4D41 then
                s_Bridge = ptr
                s_Bridge.luaActive = 1
                return s_Bridge
            end
        end
    end

    -- 2. Fallback por GetModuleHandle
    local candidates = { "SomaliaNative.asi", "Somalia.asi", "SomaliaNative.dll" }
    for _, name in ipairs(candidates) do
        local hMod = ffi.C.GetModuleHandleA(name)
        if hMod ~= nil then
            local proc = ffi.C.GetProcAddress(hMod, "GetLuaSlideBridge")
            if proc ~= nil then
                local fn = ffi.cast("LuaSlideBridgeStruct* (*)()", proc)
                local ptr = fn()
                if ptr ~= nil and ptr.magic == 0x534F4D41 then
                    s_Bridge = ptr
                    s_Bridge.luaActive = 1
                    return s_Bridge
                end
            end
        end
    end
    return nil
end

local function getMargin(armaAtual)
    local bridge = getBridge()
    if bridge ~= nil then
        if armaAtual == 34 then return bridge.margin_snp
        elseif armaAtual == 24 then return bridge.margin_desert
        elseif armaAtual == 31 then return bridge.margin_m4
        elseif armaAtual == 30 then return bridge.margin_ak
        elseif armaAtual == 25 then return bridge.margin_shot
        end
        return 0
    end
    
    -- Fallback: recarrega do INI caso a bridge de memoria nao esteja disponivel
    local fresh = inicfg.load(configData, configFile)
    if fresh and fresh.settings then loadedConfig = fresh end
    local chaveArma = idParaChave[armaAtual]
    return (chaveArma and loadedConfig.settings[chaveArma]) or 0
end

local lastIniCheck = 0
local function isSlideEnabled()
    local bridge = getBridge()
    if bridge ~= nil then
        bridge.luaActive = 1
        pcall(function() bridge.lastHeartbeat = ffi.C.GetTickCount() end)
        scriptAtivo = (bridge.enabled == 1)
        return scriptAtivo
    end

    -- Fallback: recarrega do INI a cada 100ms se ainda nao conectou na memoria
    local now = os.clock()
    if now - lastIniCheck > 0.1 then
        lastIniCheck = now
        local fresh = inicfg.load(configData, configFile)
        if fresh and fresh.settings then
            loadedConfig = fresh
            local val = fresh.settings.scriptAtivo
            scriptAtivo = (val == true or val == "true" or val == 1 or val == "1")
        end
    end
    return scriptAtivo
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
                local bridge = getBridge()
                if bridge ~= nil then
                    if chave == "margem_snp" then bridge.margin_snp = valNum
                    elseif chave == "margem_desert" then bridge.margin_desert = valNum
                    elseif chave == "margem_m4" then bridge.margin_m4 = valNum
                    elseif chave == "margem_ak" then bridge.margin_ak = valNum
                    elseif chave == "margem_shot" then bridge.margin_shot = valNum
                    end
                end
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
    local bridge = getBridge()
    if bridge ~= nil then
        bridge.enabled = (bridge.enabled == 1) and 0 or 1
        scriptAtivo = (bridge.enabled == 1)
    else
        scriptAtivo = not scriptAtivo
    end
    loadedConfig.settings.scriptAtivo = scriptAtivo
    pcall(function() inicfg.save(loadedConfig, configFile) end)
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
