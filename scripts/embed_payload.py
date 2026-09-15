import os
import sys

def main():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    root_dir = os.path.dirname(script_dir)

    candidates = [
        os.path.join(root_dir, "SomaliaNative", "build", "SomaliaNative.asi"),
        os.path.join(root_dir, "SomaliaNative.asi"),
        os.path.join(root_dir, "dist", "SomaliaNative.asi"),
        os.path.join(root_dir, "SomaliaNative", "build", "SomaliaNative.dll"),
        os.path.join(root_dir, "SomaliaNative.dll"),
    ]

    asi_path = None
    for c in candidates:
        if os.path.isfile(c) and os.path.getsize(c) > 0:
            asi_path = c
            break

    if not asi_path:
        print(f"[ERRO] Binario SomaliaNative.asi nao encontrado em nenhum dos locais esperados.")
        return 1

    file_size = os.path.getsize(asi_path)
    print(f"[INFO] Lendo binario: {asi_path} ({file_size} bytes)")

    with open(asi_path, "rb") as f:
        data = f.read()

    payload_dir = os.path.join(root_dir, "SomaliaLoader", "Payload")
    os.makedirs(payload_dir, exist_ok=True)

    header_path = os.path.join(payload_dir, "SomaliaPayload.h")
    cpp_path = os.path.join(payload_dir, "SomaliaPayload.cpp")

    # Header
    with open(header_path, "w", encoding="utf-8") as f:
        f.write("#pragma once\n")
        f.write("#include <cstddef>\n\n")
        f.write("// Payload nativo do Somalia embutido diretamente no Loader (.exe standalone)\n")
        f.write("extern const unsigned char g_SomaliaNativePayload[];\n")
        f.write("extern const size_t g_SomaliaNativePayloadSize;\n")

    # Cpp
    print(f"[INFO] Gerando {cpp_path}...")
    with open(cpp_path, "w", encoding="utf-8") as f:
        f.write('#include "SomaliaPayload.h"\n\n')
        f.write("const unsigned char g_SomaliaNativePayload[] = {\n")
        
        # Write bytes in chunks of 16 for readable and efficient C++ compilation
        chunk_size = 16
        for i in range(0, len(data), chunk_size):
            chunk = data[i:i + chunk_size]
            hex_str = ", ".join(f"0x{b:02X}" for b in chunk)
            f.write(f"    {hex_str},\n")

        f.write("};\n\n")
        f.write("const size_t g_SomaliaNativePayloadSize = sizeof(g_SomaliaNativePayload);\n")

    print(f"[SUCESSO] Payload C++ gerado com sucesso em {cpp_path} ({len(data)} bytes embutidos).")
    return 0

if __name__ == "__main__":
    sys.exit(main())
