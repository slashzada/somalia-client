$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$rootDir = Split-Path -Parent $scriptDir

$candidates = @(
    (Join-Path $rootDir "SomaliaNative\build\SomaliaNative.asi"),
    (Join-Path $rootDir "SomaliaNative.asi"),
    (Join-Path $rootDir "dist\SomaliaNative.asi")
)

$asiPath = $null
foreach ($c in $candidates) {
    if (Test-Path $c) {
        $asiPath = $c
        break
    }
}

if (-not $asiPath) {
    Write-Error "[ERRO] SomaliaNative.asi nao encontrado."
    exit 1
}

$bytes = [System.IO.File]::ReadAllBytes($asiPath)
Write-Host "[INFO] Lendo binario: $asiPath ($($bytes.Length) bytes)"

$payloadDir = Join-Path $rootDir "SomaliaLoader\Payload"
if (-not (Test-Path $payloadDir)) {
    New-Item -ItemType Directory -Path $payloadDir -Force | Out-Null
}

$headerPath = Join-Path $payloadDir "SomaliaPayload.h"
$cppPath = Join-Path $payloadDir "SomaliaPayload.cpp"

$headerContent = @"
#pragma once
#include <cstddef>

// Payload nativo do Somalia embutido diretamente no Loader (.exe standalone)
extern const unsigned char g_SomaliaNativePayload[];
extern const size_t g_SomaliaNativePayloadSize;
"@

[System.IO.File]::WriteAllText($headerPath, $headerContent, [System.Text.Encoding]::UTF8)

Write-Host "[INFO] Gerando $cppPath..."
$sb = [System.Text.StringBuilder]::new($bytes.Length * 6 + 1024)
[void]$sb.AppendLine('#include "SomaliaPayload.h"')
[void]$sb.AppendLine('')
[void]$sb.AppendLine('const unsigned char g_SomaliaNativePayload[] = {')

for ($i = 0; $i -lt $bytes.Length; $i++) {
    if ($i % 16 -eq 0) {
        [void]$sb.Append("    ")
    }
    [void]$sb.Append(('0x{0:X2}, ' -f $bytes[$i]))
    if (($i + 1) % 16 -eq 0) {
        [void]$sb.AppendLine("")
    }
}

if ($bytes.Length % 16 -ne 0) {
    [void]$sb.AppendLine("")
}

[void]$sb.AppendLine('};')
[void]$sb.AppendLine('')
[void]$sb.AppendLine('const size_t g_SomaliaNativePayloadSize = sizeof(g_SomaliaNativePayload);')

[System.IO.File]::WriteAllText($cppPath, $sb.ToString(), [System.Text.Encoding]::UTF8)
Write-Host "[SUCESSO] SomaliaPayload.cpp atualizado com sucesso!"
