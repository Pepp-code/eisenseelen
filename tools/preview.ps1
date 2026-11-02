<#
Startet das Spiel ohne sichtbares Fenster und nimmt Vorschaubilder auf.

    pwsh tools/preview.ps1

Die Bilder landen in Saved\Screenshots\WindowsEditor.
#>
param(
	[int]$ExpectedShots = 3,
	[int]$TimeoutSeconds = 240
)

$ErrorActionPreference = 'Stop'
$Engine = 'C:\Games\UE_5.8\Engine'
$Root = Split-Path $PSScriptRoot -Parent
$Shots = Join-Path $Root 'Saved\Screenshots\WindowsEditor'
if (Test-Path $Shots) {
	Remove-Item $Shots -Recurse -Force
}

$Script = (Join-Path $PSScriptRoot 'preview_shots.py') -replace '\\', '/'
$Arguments = @(
	"`"$(Join-Path $Root 'Eisenseelen.uproject')`"", '-game', '-RenderOffscreen', '-ResX=1600', '-ResY=900',
	'-nosound', '-nosplash', '-unattended', "-ExecCmds=`"py $Script`""
)
$Game = Start-Process -FilePath "$Engine\Binaries\Win64\UnrealEditor.exe" -ArgumentList $Arguments -PassThru
$Deadline = (Get-Date).AddSeconds($TimeoutSeconds)
$Count = 0
while (-not $Game.HasExited -and (Get-Date) -lt $Deadline) {
	Start-Sleep -Seconds 3
	$Count = @(Get-ChildItem $Shots -Filter '*.png' -ErrorAction SilentlyContinue).Count
	if ($Count -ge $ExpectedShots) {
		Start-Sleep -Seconds 3
		break
	}
}
if (-not $Game.HasExited) {
	Stop-Process -Id $Game.Id -Force
}
Write-Host "$Count von $ExpectedShots Bildern in $Shots"
if ($Count -lt $ExpectedShots) {
	exit 1
}
