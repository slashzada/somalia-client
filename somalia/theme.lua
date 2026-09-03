--[[
    Somalia - Theme & Styling Module
    Preserva a identidade visual SML Neon Blue (#0A0B10, #0F1117, #0080FF)
]]

local okImgui, imgui = pcall(require, 'imgui')

local Theme = {
    Colors = {
        Background      = okImgui and imgui.ImVec4(0.04, 0.04, 0.06, 1.00) or {0.04, 0.04, 0.06, 1.00}, -- #0A0B10
        SidebarBg       = okImgui and imgui.ImVec4(0.04, 0.04, 0.06, 1.00) or {0.04, 0.04, 0.06, 1.00},
        CardBg          = okImgui and imgui.ImVec4(0.06, 0.07, 0.09, 1.00) or {0.06, 0.07, 0.09, 1.00}, -- #0F1117
        CardBorder      = okImgui and imgui.ImVec4(0.12, 0.14, 0.18, 0.85) or {0.12, 0.14, 0.18, 0.85}, -- #1E232E
        AccentPrimary   = okImgui and imgui.ImVec4(0.00, 0.50, 1.00, 1.00) or {0.00, 0.50, 1.00, 1.00}, -- #0080FF
        AccentHover     = okImgui and imgui.ImVec4(0.12, 0.60, 1.00, 1.00) or {0.12, 0.60, 1.00, 1.00}, -- #1E96FF
        AccentActive    = okImgui and imgui.ImVec4(0.00, 0.40, 0.85, 1.00) or {0.00, 0.40, 0.85, 1.00}, -- #0066D9
        AccentDark      = okImgui and imgui.ImVec4(0.10, 0.15, 0.25, 1.00) or {0.10, 0.15, 0.25, 1.00}, -- #182640
        TextPrimary     = okImgui and imgui.ImVec4(0.96, 0.97, 1.00, 1.00) or {0.96, 0.97, 1.00, 1.00}, -- #F5F7FF
        TextSecondary   = okImgui and imgui.ImVec4(0.50, 0.54, 0.62, 1.00) or {0.50, 0.54, 0.62, 1.00}, -- #808A9E
        TextMuted       = okImgui and imgui.ImVec4(0.35, 0.38, 0.45, 1.00) or {0.35, 0.38, 0.45, 1.00},
        Success         = okImgui and imgui.ImVec4(0.10, 0.80, 0.45, 1.00) or {0.10, 0.80, 0.45, 1.00},
        Danger          = okImgui and imgui.ImVec4(0.95, 0.25, 0.25, 1.00) or {0.95, 0.25, 0.25, 1.00},
        Warning         = okImgui and imgui.ImVec4(0.98, 0.70, 0.15, 1.00) or {0.98, 0.70, 0.15, 1.00}
    },
    Sizes = {
        Window          = okImgui and imgui.ImVec2(780, 540) or {780, 540},
        SidebarWidth    = 150,
        CardHeight      = 460
    },
    Rounding = {
        Window          = 10.0,
        Card            = 8.0,
        Button          = 6.0,
        Slider          = 8.0,
        Checkbox        = 4.0
    },
    Fonts = {
        Main            = nil,
        Bold            = nil,
        Title           = nil,
        Logo            = nil,
        Small           = nil
    }
}

function Theme.initFonts()
    if not okImgui then return end
    pcall(function()
        local imguiIO = imgui.GetIO()
        local winDir = os.getenv("WINDIR") or "C:\\Windows"
        local segoeB = winDir .. "\\Fonts\\segoeuib.ttf"
        local segoe  = winDir .. "\\Fonts\\segoeui.ttf"
        local arialB = winDir .. "\\Fonts\\arialbd.ttf"
        local arial  = winDir .. "\\Fonts\\arial.ttf"
        
        local fontPath = nil
        local f = _G.io.open(segoeB, "r")
        if f then f:close(); fontPath = segoeB end
        
        if not fontPath then
            f = _G.io.open(arialB, "r")
            if f then f:close(); fontPath = arialB end
        end
        if not fontPath then
            f = _G.io.open(segoe, "r")
            if f then f:close(); fontPath = segoe end
        end
        if not fontPath then
            f = _G.io.open(arial, "r")
            if f then f:close(); fontPath = arial end
        end
        
        if fontPath then
            Theme.Fonts.Main  = imguiIO.Fonts:AddFontFromFileTTF(fontPath, 16.0)
            Theme.Fonts.Bold  = imguiIO.Fonts:AddFontFromFileTTF(fontPath, 17.5)
            Theme.Fonts.Title = imguiIO.Fonts:AddFontFromFileTTF(fontPath, 20.0)
            Theme.Fonts.Logo  = imguiIO.Fonts:AddFontFromFileTTF(fontPath, 26.0)
            Theme.Fonts.Small = imguiIO.Fonts:AddFontFromFileTTF(fontPath, 13.5)
        end
    end)
end

function Theme.applyStyle()
    if not okImgui then return end
    pcall(function()
        local style = imgui.GetStyle()
        local colors = style.Colors
        local clr = imgui.Col

        style.WindowRounding = Theme.Rounding.Window
        style.FrameRounding = Theme.Rounding.Button
        style.GrabRounding = Theme.Rounding.Slider
        style.ScrollbarRounding = 4.0
        style.WindowPadding = imgui.ImVec2(14, 14)
        style.ItemSpacing = imgui.ImVec2(8, 8)
        style.ItemInnerSpacing = imgui.ImVec2(6, 6)

        pcall(function() style.ChildWindowRounding = Theme.Rounding.Card end)
        pcall(function() style.ChildRounding = Theme.Rounding.Card end)

        if clr.WindowBg then colors[clr.WindowBg] = Theme.Colors.Background end
        if clr.ChildWindowBg then colors[clr.ChildWindowBg] = Theme.Colors.CardBg end
        if clr.ChildBg then colors[clr.ChildBg] = Theme.Colors.CardBg end
        if clr.Border then colors[clr.Border] = Theme.Colors.CardBorder end
        if clr.BorderShadow then colors[clr.BorderShadow] = imgui.ImVec4(0, 0, 0, 0) end
        
        if clr.TitleBg then colors[clr.TitleBg] = Theme.Colors.Background end
        if clr.TitleBgActive then colors[clr.TitleBgActive] = Theme.Colors.Background end
        if clr.Button then colors[clr.Button] = Theme.Colors.AccentPrimary end
        if clr.ButtonHovered then colors[clr.ButtonHovered] = Theme.Colors.AccentHover end
        if clr.ButtonActive then colors[clr.ButtonActive] = Theme.Colors.AccentActive end
        
        if clr.Text then colors[clr.Text] = Theme.Colors.TextPrimary end
        if clr.TextDisabled then colors[clr.TextDisabled] = Theme.Colors.TextMuted end
    end)
end

return Theme
