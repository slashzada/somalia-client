--[[
    SOMALIA - ETAPA 1: SCRIPT DE DIAGNÓSTICO MÍNIMO
    Isolado de qualquer dependência externa (Sem ImGui, Sem KeyAuth, Sem Módulos)
    Objetivo: Provar se o MoonLoader está executando scripts Lua no GTA/SA-MP
]]

print("[SOMALIA-TEST] ========================================")
print("[SOMALIA-TEST] 1. Lua script carregado no MoonLoader!")
print("[SOMALIA-TEST] ========================================")

function main()
    print("[SOMALIA-TEST] 2. Entrou na thread main(). Aguardando SA-MP...")

    -- Aguarda o SA-MP estar disponível
    while not isSampAvailable() do
        wait(100)
    end

    print("[SOMALIA-TEST] 3. SA-MP disponivel e detectado com sucesso!")
    
    -- Envia mensagem nítida em verde no chat do SA-MP
    sampAddChatMessage("{00FF00}[SOMALIA-TEST]{FFFFFF} Runtime MoonLoader funcionando com sucesso!", -1)
    sampAddChatMessage("{00FF00}[SOMALIA-TEST]{FFFFFF} O ambiente Lua basico esta 100% operacional.", -1)

    print("[SOMALIA-TEST] 4. Mensagem enviada ao chat. Loop basico iniciado.")

    while true do
        wait(1000)
    end
end
