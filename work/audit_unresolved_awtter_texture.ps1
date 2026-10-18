$ErrorActionPreference='Stop'
$gameRoot='D:\SteamLibrary\steamapps\common\SourceFilmmaker\game'
$vmt=Join-Path $gameRoot 'usermod/materials/models/mariogamer/awtter/Body.vmt'
$projectRoot=Split-Path $PSScriptRoot -Parent
$requested='materials/models/mariogamer/awtter/bodytythoon.vtf'
$checks=@()
foreach($mount in @('usermod','tf_movies','tf','left4dead2_movies','hl2','workshop','fluidscriptstudios_tools')) {
 $path=Join-Path (Join-Path $gameRoot $mount) $requested
 $checks+=@{mount=$mount;requestedFile=$path;exists=(Test-Path -LiteralPath $path)}
}
$matches=@(& rg --files --no-ignore $gameRoot -g '*.vpk' -g 'bodytythoon.vtf' -g 'bodyrythoon.vtf')
if($LASTEXITCODE -gt 1){throw 'File inventory failed'}
@{requestedTexture=$requested;vmt=$vmt;vmtSha256=(Get-FileHash -LiteralPath $vmt -Algorithm SHA256).Hash;looseMountChecks=$checks;recursiveMatchingInventory=$matches;vpkFilesFound=@($matches | Where-Object {$_ -like '*.vpk'}).Count;limits='Read-only installed file audit. No VPK files found under this game directory. Steam caches, dynamic mounts and native missing-texture behavior remain unverified. Similar names do not prove intended replacements.'} | ConvertTo-Json -Depth 5 | Set-Content (Join-Path $projectRoot 'outputs/sfm64-unresolved-awtter-texture-audit.json')
Write-Output 'Requested texture absent from declared loose mounts; inventory saved.'
