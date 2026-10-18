$ErrorActionPreference = 'Stop'

$repo = Split-Path -Parent $PSScriptRoot
$session = 'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\tf_movies\elements\sessions\mtt_soldier\mtt_soldier.dmx'
$manifestPath = Join-Path $repo 'outputs\sfm64-benchmark-mtt-soldier.json'
$manifest = Get-Content -Raw -LiteralPath $manifestPath | ConvertFrom-Json

if (-not (Test-Path -LiteralPath $session -PathType Leaf)) { throw "Benchmark session missing: $session" }
$item = Get-Item -LiteralPath $session
$hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $session).Hash.ToLowerInvariant()
if ($item.Length -ne $manifest.source.bytes) { throw "Benchmark size changed: $($item.Length)" }
if ($hash -ne $manifest.source.sha256) { throw "Benchmark SHA-256 changed: $hash" }

$graphExe = Join-Path $repo 'work\dmx_graph.exe'
$uuidExe = Join-Path $repo 'build\session-core\inspect_session_external_uuids.exe'
$coreExe = Join-Path $repo 'build\session-core\sfm64-session-core.exe'
foreach ($exe in @($graphExe, $uuidExe, $coreExe)) {
    if (-not (Test-Path -LiteralPath $exe -PathType Leaf)) { throw "Required x64 benchmark executable missing: $exe" }
}

$graphOutput = & $graphExe $session
if ($LASTEXITCODE -ne 0) { throw "DMX graph reader exited $LASTEXITCODE" }
$graph = $graphOutput | ConvertFrom-Json
if ($graph.pointer_bits -ne 64 -or $graph.elements -ne 210114 -or $graph.attributes -ne 1165763 -or -not $graph.active_clip_resolved -or $graph.active_clip_index -ne 13145) {
    throw "Raw graph regression: $($graphOutput -join ' ')"
}

$uuidOutput = & $uuidExe $session
if ($LASTEXITCODE -ne 0) { throw "External UUID inspector exited $LASTEXITCODE" }
$uuidText = $uuidOutput -join "`n"
$expectedUuid = $manifest.baselineChecks.externalReferences.uuid
if ($uuidText -notmatch [regex]::Escape($expectedUuid) -or $uuidText -notmatch 'owner_type=DmeParticleChild owner_name=cig_burn attribute=child') {
    throw "External particle reference regression: $uuidText"
}

$coreOutput = & $coreExe inspect $session
if ($LASTEXITCODE -ne 0) { throw "Owned session-core inspect exited $LASTEXITCODE" }
$coreText = $coreOutput -join "`n"
if ($coreText -notmatch 'elements=210114 reference_attributes=304909 reference_values=377508 opaque_external_references_retained=1') {
    throw "Owned runtime import regression: $coreText"
}

$finalHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $session).Hash.ToLowerInvariant()
if ($finalHash -ne $hash) { throw "Installed benchmark changed during verification: $finalHash" }

$renderLogPath = Join-Path $repo 'outputs\sfm64-mtt-soldier-vulkan-lightmap-render.txt'
$renderImagePath = Join-Path $repo 'outputs\mtt-soldier-first-shot-vulkan\mtt-soldier-frame-119-displacements.png'
if (-not (Test-Path -LiteralPath $renderLogPath -PathType Leaf) -or -not (Test-Path -LiteralPath $renderImagePath -PathType Leaf)) {
    throw 'First populated shot Vulkan render receipt or PNG is missing'
}
$renderLog = Get-Content -Raw -LiteralPath $renderLogPath
if ($renderLog -notmatch 'frozen_scene_models=4 vertices=52497 indices=110466' -or $renderLog -notmatch 'multi_model_scene_frame=119 tick=49583 models=4' -or $renderLog -notmatch 'Vulkan indexed model draw passed triangles=36822 model_visible=1 changed_pixels=262144 output=512x512 RGBA frame=119') {
    throw 'First populated shot Vulkan world-plus-session render receipt does not match the verified scene metrics'
}
$renderImage = Get-Item -LiteralPath $renderImagePath
if ($renderImage.Length -lt 4096) { throw 'First populated shot Vulkan PNG is unexpectedly small' }

# Exercise the rebuilt x64 editor/render path on the benchmark's first populated shot.
# The control-test mode drives several viewport frames and exits without leaving a window open.
$rendererExe = Join-Path $repo 'build\session-core\probe_vulkan_win32_surface_x64.exe'
if (-not (Test-Path -LiteralPath $rendererExe -PathType Leaf)) { throw "Live x64 renderer missing: $rendererExe" }
$gameRoot = (Get-Item -LiteralPath $session).Directory.Parent.Parent.Parent.Parent.FullName
$contentRoot = Join-Path $gameRoot 'usermod'
$runId = [Guid]::NewGuid().ToString('N')
$liveStdoutPath = Join-Path $repo "work\mtt-soldier-live-shell-$runId.stdout.txt"
$liveStderrPath = Join-Path $repo "work\mtt-soldier-live-shell-$runId.stderr.txt"
$renderProcess = Start-Process -FilePath $rendererExe -ArgumentList @(
    '--control-test', '--session', $session, '--content-directory', $contentRoot, '--start-frame', '119'
) -Wait -PassThru -RedirectStandardOutput $liveStdoutPath -RedirectStandardError $liveStderrPath
try {
    if ($renderProcess.ExitCode -ne 0) { throw "Live x64 Vulkan session render exited $($renderProcess.ExitCode): $(Get-Content -Raw $liveStderrPath)" }
    $liveStdout = Get-Content -Raw -LiteralPath $liveStdoutPath
    $liveStderr = Get-Content -Raw -LiteralPath $liveStderrPath
    if (-not [string]::IsNullOrWhiteSpace($liveStderr) -or
        $liveStdout -notmatch 'session_argument_loaded=1 initial_tick=49583 frame_count=2276' -or
        $liveStdout -notmatch 'timeline_frame_state=2 tick=49583' -or
        $liveStdout -notmatch 'multi_model_scene_frame=119 tick=49583 models=4' -or
        $liveStdout -notmatch 'Vulkan indexed model draw passed triangles=36822 model_visible=1 changed_pixels=262144 output=512x512 RGBA frame=119' -or
        $liveStdout -notmatch 'model_gpu_presentation_passed=1') {
        throw "Live first-shot Vulkan session validation failed. stderr=$liveStderr stdout=$liveStdout"
    }
    $rgbaPath = Join-Path $repo 'outputs\vulkan-retained-frame-119.rgba'
    $rgba = [IO.File]::ReadAllBytes($rgbaPath)
    if ($rgba.Length -ne 512 * 512 * 4) { throw "Live Vulkan frame readback has unexpected byte count: $($rgba.Length)" }
    $nonBlackPixels = 0
    $uniqueRgb = [System.Collections.Generic.HashSet[uint32]]::new()
    for ($offset = 0; $offset -lt $rgba.Length; $offset += 4) {
        $red = [uint32]$rgba[$offset]
        $green = [uint32]$rgba[$offset + 1]
        $blue = [uint32]$rgba[$offset + 2]
        if ($red -or $green -or $blue) { $nonBlackPixels++ }
        [void]$uniqueRgb.Add(($red -shl 16) -bor ($green -shl 8) -bor $blue)
    }
    if ($nonBlackPixels -lt 1000 -or $uniqueRgb.Count -lt 32) {
        throw "Live Vulkan frame readback lacks rendered color: nonblack=$nonBlackPixels unique_rgb=$($uniqueRgb.Count)"
    }
} finally {
    if ($renderProcess -and -not $renderProcess.HasExited) { Stop-Process -Id $renderProcess.Id -Force -ErrorAction SilentlyContinue }
}
$finalHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $session).Hash.ToLowerInvariant()
if ($finalHash -ne $hash) { throw "Installed benchmark changed during live render test: $finalHash" }

$report = [ordered]@{
    benchmark = $manifest.benchmark
    status = 'integrity_owned_import_live_first_shot_vulkan_presentation_passed'
    source_sha256 = $finalHash
    bytes = $item.Length
    raw_graph = $graph
    external_reference = $manifest.baselineChecks.externalReferences.uuid
    owned_session_core = '210114 elements, 304909 reference attributes, 377508 reference values; one opaque external reference retained'
    first_populated_shot = 'tick 49583, three session assets plus lightmapped/tessellated BSP world, 52497 vertices, 36822 triangles; Vulkan presented 512x512 world-plus-session frame'
    live_editor_render = 'x64 control-test loaded frame 119 at tick 49583, presented four scene groups including the BSP world, and completed five Vulkan frames'
    live_test_log = $liveStdoutPath
    live_readback = "512x512 RGBA, nonblack_pixels=$nonBlackPixels, unique_rgb_colors=$($uniqueRgb.Count)"
    prototype_png = $renderImagePath
    render_parity = 'live prototype session-to-Vulkan presentation passed; pixel parity against SFM is untested'
    visual_parity_claimed = $false
}
$reportPath = Join-Path $repo 'outputs\sfm64-benchmark-mtt-soldier-latest.json'
$report | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $reportPath -Encoding UTF8
Write-Output "mtt_soldier benchmark passed; owned import and live x64 Vulkan frame-119 presentation verified; SFM pixel parity remains untested; report=$reportPath"


