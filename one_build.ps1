# build_one.ps1
param(
    [string]$Board = "MURM2",
    [string]$Chip = "p1"
)

Write-Output "========================================"
Write-Output "START BUILD"
Write-Output "========================================"
Write-Output ""
Write-Output "Board: $Board"
Write-Output "Chip: $Chip"
Write-Output "PICO_SDK_PATH: $env:PICO_SDK_PATH"
Write-Output ""

# Определяем параметры
if ($Chip -eq "p2") {
    $PicoBoard = "pico2"
    $BuildType = "Release"
    $ChipName = "RP2350"
} else {
    $PicoBoard = "pico"
    $BuildType = "MinSizeRel"
    $ChipName = "RP2040"
}

Write-Output "PICO_BOARD: $PicoBoard"
Write-Output "Build Type: $BuildType"
Write-Output ""

# Определяем конфигурацию
if ($Board -match "WS_ZERO") {
    $Config = "SPECCY_WSZERO"
} else {
    $Config = "SPECCY"
}

# Суффикс для HDMI
$Suffix = ""
$HdmiFlag = ""
if ($Board -in @("MURM2", "S2") -and $Chip -eq "p2") {
    $Suffix = "_hdmi_audio"
    $HdmiFlag = "-DHDMI_HSTX=ON"
}

$BuildDir = "build_${Board}_${Chip}"
Write-Output "Build dir: $BuildDir"

# Удаляем старую папку
if (Test-Path $BuildDir) {
    Remove-Item -Recurse -Force $BuildDir
}
New-Item -ItemType Directory -Path $BuildDir | Out-Null

# Переходим в папку сборки
Push-Location $BuildDir

# Формируем и запускаем CMake
$CmakeCmd = "cmake -G Ninja -DPICO_SDK_PATH=`"$env:PICO_SDK_PATH`" -DM_BOARD=$Board -DSPECCY_CONFIG=$Config -DCMAKE_BUILD_TYPE=$BuildType -DPICO_BOARD=$PicoBoard -DDEBUG=OFF $HdmiFlag .."
Write-Output ""
Write-Output "Running CMake..."
Write-Output "Command: $CmakeCmd"
Write-Output ""
Invoke-Expression $CmakeCmd

if ($LASTEXITCODE -ne 0) {
    Write-Output "ERROR: CMake failed!"
    Pop-Location
    exit 1
}

Write-Output ""
Write-Output "Running Ninja..."
Write-Output ""
ninja

if ($LASTEXITCODE -ne 0) {
    Write-Output "ERROR: Build failed!"
    Pop-Location
    exit 1
}

# Копируем результат - ищем любой .uf2 файл
$ReleaseDir = "..\release"
New-Item -ItemType Directory -Path $ReleaseDir -Force | Out-Null

# Ищем .uf2 файлы в текущей папке
$Uf2Files = Get-ChildItem -Filter "*.uf2"

if ($Uf2Files) {
    foreach ($File in $Uf2Files) {
        $DestFile = Join-Path $ReleaseDir $File.Name
        Copy-Item $File.FullName $DestFile -Force
        $Size = $File.Length / 1KB
        Write-Output "OK: Copied $($File.Name) ($($Size.ToString('F1')) KB)"
    }
} else {
    # Если .uf2 нет, ищем .elf и пытаемся создать .uf2 через picotool
    $ElfFiles = Get-ChildItem -Filter "*.elf"
    if ($ElfFiles) {
        Write-Output "WARNING: .uf2 not found, but found .elf files:"
        $ElfFiles | ForEach-Object {
            Write-Output "  $($_.Name)"
        }
        
        # Пробуем создать .uf2 с помощью picotool
        $Picotool = "$env:USERPROFILE/.pico-sdk/picotool/2.3.0/picotool/picotool.exe"
        if (Test-Path $Picotool) {
            Write-Output "Attempting to create .uf2 with picotool..."
            foreach ($Elf in $ElfFiles) {
                $Uf2Name = $Elf.BaseName + ".uf2"
                & $Picotool uf2 convert $Elf.FullName $Uf2Name
                if (Test-Path $Uf2Name) {
                    Copy-Item $Uf2Name $ReleaseDir -Force
                    $Size = (Get-Item $Uf2Name).Length / 1KB
                    Write-Output "OK: Created $Uf2Name ($($Size.ToString('F1')) KB)"
                }
            }
        } else {
            Write-Output "WARNING: picotool not found at $Picotool"
        }
    } else {
        Write-Output "WARNING: No .uf2 or .elf files found!"
        Write-Output "Files in build directory:"
        Get-ChildItem | ForEach-Object {
            Write-Output "  $($_.Name) ($($_.Length / 1KB) KB)"
        }
    }
}

Pop-Location
Write-Output ""
Write-Output "========================================"
Write-Output "BUILD COMPLETE!"
Write-Output "========================================"