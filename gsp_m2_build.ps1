# build_gsp_m2.ps1
param(
    [string]$Version = ""
)

Write-Output "========================================"
Write-Output "BUILDING GSP_M2 (GSP_FULL)"
Write-Output "========================================"
Write-Output ""

# Если версия не указана, читаем из CMakeLists.txt
if (-not $Version) {
    Write-Output "Reading version from CMakeLists.txt..."
    
    $cmakeContent = Get-Content "CMakeLists.txt" -Raw
    
    if ($cmakeContent -match 'set\(VERSION_MAJOR\s+(\d+)\)') {
        $major = $Matches[1]
    } else {
        Write-Output "ERROR: VERSION_MAJOR not found!"
        exit 1
    }
    
    if ($cmakeContent -match 'set\(VERSION_MINOR\s+(\d+)\)') {
        $minor = $Matches[1]
    } else {
        Write-Output "ERROR: VERSION_MINOR not found!"
        exit 1
    }
    
    if ($cmakeContent -match 'set\(VERSION_PATCH\s+(\d+)\)') {
        $patch = $Matches[1]
    } else {
        Write-Output "ERROR: VERSION_PATCH not found!"
        exit 1
    }
    
    $Version = "${major}.${minor}.${patch}"
    Write-Output "Version detected: $Version"
    Write-Output ""
}

# Создаем папку release/SpeccyP vX.X.X
$ReleaseDir = "release\SpeccyP v${Version}"
if (Test-Path $ReleaseDir) {
    Remove-Item -Recurse -Force $ReleaseDir
}
New-Item -ItemType Directory -Path $ReleaseDir -Force | Out-Null
Write-Output "Release folder: $ReleaseDir"
Write-Output ""

# Плата GSP_M2
$board = "GSP_M2"
$config = "GSP_FULL"

# Варианты: p1 и p2
$builds = @(
    @{Board=$board; Chip="p1"; TargetName="SpeccyP_${Version}GS_m2p1.uf2"},
    @{Board=$board; Chip="p2"; TargetName="SpeccyP_${Version}GS_m2p2.uf2"}
)

$totalBuilds = $builds.Count
$currentBuild = 0

# Создаем папку build если её нет
if (-not (Test-Path "build")) {
    New-Item -ItemType Directory -Path "build" | Out-Null
}

# Собираем варианты
foreach ($build in $builds) {
    $currentBuild++
    $chip = $build.Chip
    $targetName = $build.TargetName
    
    Write-Output "[$currentBuild/$totalBuilds] Building $board ($chip) with $config -> $targetName"
    
    if ($chip -eq "p2") {
        $picoBoard = "pico2"
        $buildType = "Release"
        $chipName = "RP2350"
    } else {
        $picoBoard = "pico"
        $buildType = "MinSizeRel"
        $chipName = "RP2040"
    }
    
    $buildDir = "build_${board}_${chip}_GSP_FULL"
    if (Test-Path $buildDir) {
        Remove-Item -Recurse -Force $buildDir
    }
    New-Item -ItemType Directory -Path $buildDir | Out-Null
    
    Push-Location $buildDir
    
    $cmakeCmd = "cmake -G Ninja -DPICO_SDK_PATH=`"$env:PICO_SDK_PATH`" -DM_BOARD=$board -DSPECCY_CONFIG=$config -DCMAKE_BUILD_TYPE=$buildType -DPICO_BOARD=$picoBoard -DDEBUG=OFF .."
    Write-Output "  Running CMake..."
    Invoke-Expression $cmakeCmd
    
    if ($LASTEXITCODE -ne 0) {
        Write-Output "  ERROR: CMake failed for $board ($chip)"
        Pop-Location
        continue
    }
    
    Write-Output "  Running Ninja..."
    ninja
    
    if ($LASTEXITCODE -ne 0) {
        Write-Output "  ERROR: Build failed for $board ($chip)"
        Pop-Location
        continue
    }
    
    # Копируем .uf2 файл из папки build в release с правильным именем
    $sourceFile = "..\build\SpeccyP.uf2"
    if (Test-Path $sourceFile) {
        $destFile = Join-Path "..\$ReleaseDir" $targetName
        Copy-Item $sourceFile $destFile -Force
        $size = (Get-Item $sourceFile).Length / 1KB
        Write-Output "  OK: Copied as $targetName ($($size.ToString('F1')) KB)"
    } else {
        Write-Output "  WARNING: $sourceFile not found!"
    }
    
    Pop-Location
    Write-Output "  OK: Build complete for $board ($chipName)"
    Write-Output ""
}

# Показываем результаты
Write-Output ""
Write-Output "========================================"
Write-Output "BUILD COMPLETE!"
Write-Output "========================================"
Write-Output ""

$outputMessage = "Files in " + $ReleaseDir + ":"
Write-Output $outputMessage

$files = Get-ChildItem -Path $ReleaseDir -Filter "*.uf2" | Sort-Object Name
if ($files) {
    $files | ForEach-Object {
        $size = $_.Length / 1KB
        Write-Output "  $($_.Name) ($($size.ToString('F1')) KB)"
    }
    Write-Output ""
    $totalMessage = "Total: " + $files.Count + " files"
    Write-Output $totalMessage
} else {
    Write-Output "  No .uf2 files found!"
}

# Удаляем временные папки сборки
Write-Output ""
Write-Output "Cleaning up build folders..."

# Удаляем папки build_*
$buildFolders = Get-ChildItem -Directory -Filter "build_*"
if ($buildFolders) {
    $buildFolders | ForEach-Object {
        Remove-Item -Recurse -Force $_.FullName
        Write-Output "  Removed: $($_.Name)"
    }
} else {
    Write-Output "  No build_* folders to clean"
}

# Удаляем папку build целиком
if (Test-Path "build") {
    Remove-Item -Recurse -Force "build"
    Write-Output "  Removed: build folder"
}

Write-Output ""
Write-Output "========================================"
Write-Output "DONE!"
Write-Output "========================================"
Write-Output ""

$finalMessage = "Release folder: " + $ReleaseDir
Write-Output $finalMessage