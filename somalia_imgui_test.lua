--[[
    SOMALIA - ETAPA 5: TESTE MÍNIMO DE IMGUI & HOTKEY F5
    Isolado de módulos complexos. Testa apenas se a lib ImGui renderiza janelas e se o F5 responde.
]]

local okImgui, imgui = pcall(require, 'imgui')
local showTestWindow = okImgui and imgui.ImBool(false) or { v = false }

function main()
    print("[SOMALIA-IMGUI-TEST] ========================================")
    print("[SOMALIA-IMGUI-TEST] 1. Script de teste ImGui carregado!")
    print("[SOMALIA-IMGUI-TEST] 2. Status lib ImGui encontrada: " .. tostring(okImgui))

    while not isSampAvailable() do
        wait(100)
    end

    if okImgui then
        sampAddChatMessage("{0080FF}[SOMALIA-IMGUI-TEST]{FFFFFF} ImGui detectado! Pressione {0080FF}F5{FFFFFF} para testar janela.", -1)
    else
        sampAddChatMessage("{FF0000}[SOMALIA-IMGUI-TEST]{FFFFFF} ERRO: Biblioteca ImGui ausente em moonloader/lib!", -1)
    end

    while true do
        wait(0)

        if isKeyJustPressed(0x74) then -- 0x74 = VK_F5
            if okImgui then
                showTestWindow.v = not showTestWindow.v
                print("[SOMALIA-IMGUI-TEST] F5 Pressionado! Janela aberta: " .. tostring(showTestWindow.v))
            end
        end

        if okImgui then
            imgui.Process = showTestWindow.v
            imgui.ShowCursor = showTestWindow.v
        end
    end
end

if okImgui then
    function imgui.OnDrawFrame()
        if showTestWindow.v then
            local sw, sh = getScreenResolution()
            imgui.SetNextWindowPos(imgui.ImVec2(sw / 2, sh / 2), imgui.Cond.FirstUseEver, imgui.ImVec2(0.5, 0.5))
            imgui.SetNextWindowSize(imgui.ImVec2(350, 200), imgui.Cond.FirstUseEver)
            imgui.Begin("SOMALIA - TESTE DE IMGUI", showTestWindow)
            imgui.Text("Se voce esta vendo este texto, o ImGui e o F5 estao funcionando!")
            imgui.Spacing()
            if imgui.Button("Fechar Janela", imgui.ImVec2(-1, 30)) then
                showTestWindow.v = false
            end
            imgui.End()
        end
    end
end

function isKeyJustPressed(key)
    if isKeyDown(key) then
        if not keyWasDown then
            keyWasDown = true
            return true
        end
    else
        keyWasDown = false
    end
    return false
end
