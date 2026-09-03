import os

modules = [
    'somalia/theme.lua', 'somalia/notifications.lua', 'somalia/keybinds.lua',
    'somalia/config.lua', 'somalia/hud.lua', 'somalia/aim.lua',
    'somalia/visuals.lua', 'somalia/player.lua', 'somalia/vehicles.lua',
    'somalia/world.lua', 'somalia/misc.lua', 'main.lua'
]

embedded = {}
for m in modules:
    if os.path.exists(m):
        with open(m, 'r', encoding='utf-8') as f:
            embedded[m] = f.read()

with open('somalia_test.lua', 'r', encoding='utf-8') as f:
    s_test = f.read()

with open('somalia_imgui_test.lua', 'r', encoding='utf-8') as f:
    s_imgui = f.read()

content = '"""Embedded Lua Assets"""\n\n'
content += 'SOMALIA_TEST_LUA = ' + repr(s_test) + '\n\n'
content += 'SOMALIA_IMGUI_TEST_LUA = ' + repr(s_imgui) + '\n\n'
content += 'EMBEDDED_MODULES = ' + repr(embedded) + '\n'

with open('client/embedded_assets.py', 'w', encoding='utf-8') as f:
    f.write(content)

print('Embedded assets successfully synced!')
