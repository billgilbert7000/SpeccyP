# build_all_boards.ps1
param(
    [string]$Version = ""
)

Write-Output "========================================"
Write-Output "BUILDING ALL BOARDS"
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

# Список всех плат и их чипов (s2p1 УДАЛЕН)
$builds = @(
    @{Board="MURM1"; Chip="p1"},
    @{Board="MURM1"; Chip="p2"},
    @{Board="MURM2"; Chip="p1"},
    @{Board="MURM2"; Chip="p2"},
    @{Board="WS_ZERO1"; Chip="p1"},
    @{Board="WS_ZERO2"; Chip="p2"},
    @{Board="S2"; Chip="p2"}      # <-- ТОЛЬКО p2, p1 удален
)

# Специальные варианты с опциями
$specialBuilds = @(
    @{Board="MURM2"; Chip="p2"; Options="HDMI_HSTX"},
    @{Board="S2"; Chip="p2"; Options="HDMI_HSTX"},
    @{Board="WS_ZERO2"; Chip="p2"; Options="PCM5122"}
)

$totalBuilds = $builds.Count + $specialBuilds.Count
$currentBuild = 0

# Создаем папку build если её нет
if (-not (Test-Path "build")) {
    New-Item -ItemType Directory -Path "build" | Out-Null
}

# Собираем обычные варианты
foreach ($build in $builds) {
    $currentBuild++
    $board = $build.Board
    $chip = $build.Chip
    
    Write-Output "[$currentBuild/$totalBuilds] Building $board ($chip)"
    
    if ($chip -eq "p2") {
        $picoBoard = "pico2"
        $buildType = "Release"
    } else {
        $picoBoard = "pico"
        $buildType = "MinSizeRel"
    }
    
    if ($board -match "WS_ZERO") {
        $config = "SPECCY_WSZERO"
    } else {
        $config = "SPECCY"
    }
    
    $buildDir = "build_${board}_${chip}"
    if (Test-Path $buildDir) {
        Remove-Item -Recurse -Force $buildDir
    }
    New-Item -ItemType Directory -Path $buildDir | Out-Null
    
    Push-Location $buildDir
    
    $cmakeCmd = "cmake -G Ninja -DPICO_SDK_PATH=`"$env:PICO_SDK_PATH`" -DM_BOARD=$board -DSPECCY_CONFIG=$config -DCMAKE_BUILD_TYPE=$buildType -DPICO_BOARD=$picoBoard -DDEBUG=OFF .."
    Invoke-Expression $cmakeCmd
    
    if ($LASTEXITCODE -ne 0) {
        Write-Output "  ERROR: CMake failed for $board ($chip)"
        Pop-Location
        continue
    }
    
    ninja
    
    if ($LASTEXITCODE -ne 0) {
        Write-Output "  ERROR: Build failed for $board ($chip)"
        Pop-Location
        continue
    }
    
    Pop-Location
    Write-Output ""
}

# Собираем специальные варианты
foreach ($build in $specialBuilds) {
    $currentBuild++
    $board = $build.Board
    $chip = $build.Chip
    $options = $build.Options
    
    Write-Output "[$currentBuild/$totalBuilds] Building $board ($chip) with $options"
    
    $picoBoard = "pico2"
    $buildType = "Release"
    
    if ($board -match "WS_ZERO") {
        $config = "SPECCY_WSZERO"
    } else {
        $config = "SPECCY"
    }
    
    $optionFlag = ""
    if ($options -eq "HDMI_HSTX") {
        $optionFlag = "-DHDMI_HSTX=ON"
    } elseif ($options -eq "PCM5122") {
        $optionFlag = "-DPCM5122=ON"
    }
    
    $buildDir = "build_${board}_${chip}_${options}"
    if (Test-Path $buildDir) {
        Remove-Item -Recurse -Force $buildDir
    }
    New-Item -ItemType Directory -Path $buildDir | Out-Null
    
    Push-Location $buildDir
    
    $cmakeCmd = "cmake -G Ninja -DPICO_SDK_PATH=`"$env:PICO_SDK_PATH`" -DM_BOARD=$board -DSPECCY_CONFIG=$config -DCMAKE_BUILD_TYPE=$buildType -DPICO_BOARD=$picoBoard -DDEBUG=OFF $optionFlag .."
    Invoke-Expression $cmakeCmd
    
    if ($LASTEXITCODE -ne 0) {
        Write-Output "  ERROR: CMake failed for $board ($chip) with $options"
        Pop-Location
        continue
    }
    
    ninja
    
    if ($LASTEXITCODE -ne 0) {
        Write-Output "  ERROR: Build failed for $board ($chip) with $options"
        Pop-Location
        continue
    }
    
    Pop-Location
    Write-Output ""
}

# Копируем .uf2 файлы из папки build в release
Write-Output "========================================"
Write-Output "Copying .uf2 files from build folder to release..."
Write-Output "========================================"
Write-Output ""

# Ищем .uf2 файлы в папке build
$buildPath = "build"
if (Test-Path $buildPath) {
    $uf2Files = Get-ChildItem -Path $buildPath -Filter "*.uf2"
    
    if ($uf2Files) {
        foreach ($file in $uf2Files) {
            $destPath = Join-Path $ReleaseDir $file.Name
            Copy-Item $file.FullName $destPath -Force
            $size = $file.Length / 1KB
            Write-Output "  OK: $($file.Name) ($($size.ToString('F1')) KB)"
        }
        Write-Output ""
        Write-Output "Total copied: $($uf2Files.Count) files"
    } else {
        Write-Output "  No .uf2 files found in build folder!"
    }
} else {
    Write-Output "  build folder not found!"
}

# Показываем результаты
Write-Output ""
Write-Output "========================================"
Write-Output "ALL BUILDS COMPLETE!"
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