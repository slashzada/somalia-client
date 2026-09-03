--[[
    Somalia - Multi-Profile Configuration System
    Gerenciamento de múltiplos perfis 100% compatível com a estrutura de seções do inicfg
]]

local okInicfg, inicfg = pcall(require, 'inicfg')

local Config = {
    currentProfile = "default",
    profilesList = {"default", "legit", "movement", "rage", "visuals"},
    data = {},
    weaponsList = {
        { id = 24, name = "Desert Eagle" },
        { id = 31, name = "M4" },
        { id = 30, name = "AK-47" },
        { id = 25, name = "Shotgun" },
        { id = 26, name = "Sawnoff" },
        { id = 27, name = "Combat Shotgun" },
        { id = 29, name = "MP5" },
        { id = 32, name = "Tec-9" },
        { id = 28, name = "Micro Uzi" },
        { id = 34, name = "Sniper Rifle" },
        { id = 33, name = "Country Rifle" },
        { id = 22, name = "Pistol (Colt 45)" },
        { id = 23, name = "Silenced 9mm" }
    }
}

-- Configurações padrão individuais por arma
local function getDefaultWeaponSettings(wId)
    local isSniper = (wId == 34 or wId == 33)
    local isShotgun = (wId == 25 or wId == 26 or wId == 27)
    local isFast = (wId == 28 or wId == 29 or wId == 30 or wId == 31 or wId == 32)
    
    return {
        aim = {
            enabled = true,
            showFov = true,
            fovSize = isSniper and 35 or (isShotgun and 80 or (isFast and 50 or 65)),
            smoothH = isSniper and 6 or (isFast and 15 or 12),
            smoothV = isSniper and 8 or (isFast and 16 or 14),
            maxDist = isSniper and 250 or (isShotgun and 50 or 150),
            hitbox = (isSniper or wId == 24) and 0 or 2, -- 0 = Head, 2 = Chest
            allowNpc = false,
            visibleCheck = true
        },
        silent = {
            enabled = false,
            showFov = false,
            fovSize = isSniper and 45 or 70,
            hitbox = 0,
            allowNpc = false,
            visibleCheck = true
        }
    }
end

-- Estrutura de dados padrão de um perfil (com seções planas para o inicfg)
local function getDefaultSettings()
    local settings = {
        -- Sistema & Interface
        system = {
            menuKey = 0x74,           -- F5
            accentR = 0.0,
            accentG = 0.50,
            accentB = 1.0,
            autoLoadProfile = "default"
        },
        -- Aba 1: Aim Master Switches
        aim = {
            globalEnabled = true,     -- Master Switch do Aimbot
            key = 0x02,               -- Mouse 2 (Botão Direito)
            selectedWeaponIndex = 0   -- Índice no dropdown de armas
        },
        silent = {
            globalEnabled = false,    -- Master Switch do Silent Aim
            key = 0x02,               -- Mouse 2
            selectedWeaponIndex = 0
        },
        -- Aba 2: Visuals (ESP)
        visuals = {
            enabled = false,
            boxEsp = true,
            cornerBox = false,
            skeleton = false,
            healthBar = true,
            armorBar = true,
            playerNames = true,
            playerId = true,
            distance = true,
            weaponName = false,
            snaplines = false,
            fovCircle = true,
            crosshair = false,
            maxDistance = 250,
            colorVisR = 0, colorVisG = 200, colorVisB = 255,
            colorInvisR = 255, colorInvisG = 80, colorInvisB = 80
        },
        -- Aba 3: World
        world = {
            customTimeEnabled = false,
            customHour = 12,
            customMinute = 0,
            customWeatherEnabled = false,
            weatherId = 1             -- 1 = Sunny/Clear
        },
        -- Aba 4: Vehicles
        vehicles = {
            speedBoost = false,
            speedBoostKey = 0x10,     -- Shift
            speedMultiplier = 1.35,
            instantRepair = false,
            instantRepairKey = 0x52,  -- R
            flipCar = false,
            flipCarKey = 0x58,        -- X
            godmode = false,
            engineAlwaysOn = false,
            vehicleEsp = false
        },
        -- Aba 5: Player & Movement (C-Slide original preservado)
        player = {
            scriptAtivo = true,
            cSlideAtivo = true,
            duracaoC = 10,            -- Duração do toque no C em ms
            autoSlideAtivo = false,   -- Quick Switch (Soco e volta)
            delayTroca = 0,
            margem_snp = 550,
            margem_desert = 0,
            margem_m4 = 0,
            margem_ak = 0,
            margem_shot = 0,
            infiniteStamina = false,
            noFallDamage = false,
            sprintSpeed = 1.0
        },
        -- Aba 6: Misc & Exploits
        misc = {
            noRecoil = false,
            noSpread = false,
            fastReload = false
        },
        -- HUD / Watermark
        hud = {
            watermark = true,
            fps = true,
            ping = true,
            coords = false,
            serverInfo = true,
            activeFeatures = true
        }
    }

    -- Seções planas de armas suportadas pelo inicfg
    for _, w in ipairs(Config.weaponsList) do
        local def = getDefaultWeaponSettings(w.id)
        settings["aim_w_" .. w.id] = def.aim
        settings["silent_w_" .. w.id] = def.silent
    end

    return settings
end

-- Deep copy de tabelas
local function copyTable(orig)
    local orig_type = type(orig)
    local copy
    if orig_type == 'table' then
        copy = {}
        for orig_key, orig_value in next, orig, nil do
            copy[copyTable(orig_key)] = copyTable(orig_value)
        end
    else
        copy = orig
    end
    return copy
end

-- Mescla recursivamente garantindo tipos válidos
local function mergeTables(target, source)
    for k, v in pairs(source) do
        if type(v) == 'table' then
            if type(target[k]) == 'table' then
                mergeTables(target[k], v)
            else
                target[k] = copyTable(v)
            end
        else
            if target[k] == nil or type(target[k]) ~= type(v) then
                target[k] = v
            end
        end
    end
end

function Config.init()
    Config.data = getDefaultSettings()
    Config.load(Config.currentProfile)
end

function Config.getFilePath(profileName)
    profileName = profileName or Config.currentProfile
    return "somalia_" .. string.lower(profileName) .. ".ini"
end

function Config.save(profileName)
    profileName = profileName or Config.currentProfile
    local file = Config.getFilePath(profileName)
    if okInicfg and Config.data then
        pcall(function()
            inicfg.save(Config.data, file)
        end)
    end
end

function Config.load(profileName)
    profileName = profileName or Config.currentProfile
    local file = Config.getFilePath(profileName)
    local defaults = getDefaultSettings()
    
    if okInicfg then
        local loaded = inicfg.load(defaults, file)
        if loaded and type(loaded) == 'table' then
            mergeTables(loaded, defaults)
            Config.data = loaded
            Config.currentProfile = profileName
            return true
        end
    end
    Config.data = defaults
    Config.currentProfile = profileName
    Config.save(profileName)
    return false
end

function Config.reset()
    Config.data = getDefaultSettings()
    Config.save(Config.currentProfile)
end

function Config.getAimWeapon(wId)
    local key = "aim_w_" .. tostring(wId)
    if type(Config.data[key]) ~= 'table' then
        local def = getDefaultWeaponSettings(wId)
        Config.data[key] = def.aim
    end
    return Config.data[key]
end

function Config.getSilentWeapon(wId)
    local key = "silent_w_" .. tostring(wId)
    if type(Config.data[key]) ~= 'table' then
        local def = getDefaultWeaponSettings(wId)
        Config.data[key] = def.silent
    end
    return Config.data[key]
end

return Config
