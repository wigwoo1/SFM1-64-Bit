$ErrorActionPreference='Stop'
$projectRoot=Split-Path $PSScriptRoot -Parent
$results=@()
foreach($fixture in @(11,139)) {
 $output=Join-Path $projectRoot ('outputs/key-playback-'+[guid]::NewGuid().ToString('N')+'.dmx')
 $inputPath=Join-Path $projectRoot ('work/roundtrip-'+$fixture+'.dmx')
 $before=(Get-FileHash -LiteralPath $inputPath -Algorithm SHA256).Hash
 $log=& (Join-Path $PSScriptRoot 'test_document_key_playback.exe') $inputPath $output
 if($LASTEXITCODE){throw 'Key playback validation failed'}
 if((Get-FileHash -LiteralPath $inputPath -Algorithm SHA256).Hash -ne $before){throw 'Input session changed'}
 $log
 $results+=@{fixture=$fixture;output=$output;result=$log;inputUnchanged=$true}
}
@{checks=$results;scope='Existing float channel local-time sampling, document edit, undo, redo, save and reload; native scheduler and full application remain unfinished'} | ConvertTo-Json -Depth 5 | Set-Content (Join-Path $projectRoot 'outputs/sfm64-document-key-playback-validation.json')
