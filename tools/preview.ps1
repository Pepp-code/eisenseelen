<#
Startet das Spiel ohne sichtbares Fenster und nimmt Vorschaubilder auf.

    pwsh tools/preview.ps1              Waffen und Gegner in der Testarena
    pwsh tools/preview.ps1 -Szene lauf  ein ganzer Lauf auf der Laufkarte, mit dem Gefesselten
    pwsh tools/preview.ps1 -Szene seelenrufer  derselbe Lauf, mit dem Seelenrufer

Die Bilder landen in Saved\Screenshots\WindowsEditor.
#>
param(
	[ValidateSet('waffen', 'lauf', 'seelenrufer')]
	[string]$Szene = 'waffen',
	[int]$TimeoutSeconds = 420
)

$ErrorActionPreference = 'Stop'
$Engine = 'C:\Games\UE_5.8\Engine'
$Root = Split-Path $PSScriptRoot -Parent
$Shots = Join-Path $Root 'Saved\Screenshots\WindowsEditor'
if (Test-Path $Shots) {
	Remove-Item $Shots -Recurse -Force
}

$OnRunMap = $Szene -ne 'waffen'
$ScriptName = if ($OnRunMap) { 'preview_run.py' } else { 'preview_shots.py' }
$Map = if ($OnRunMap) { '/Game/Maps/L_ErsterLauf' } else { '/Game/Maps/L_Testarena' }
$env:EI_VORSCHAU_BOSS = if ($Szene -eq 'seelenrufer') { 'Der Seelenrufer' } else { 'Der Gefesselte' }
$Script = (Join-Path $PSScriptRoot $ScriptName) -replace '\\', '/'
$Arguments = @(
	"`"$(Join-Path $Root 'Eisenseelen.uproject')`"", $Map, '-game', '-RenderOffscreen', '-ResX=1600', '-ResY=900',
	'-nosound', '-nosplash', '-unattended', '-KeinSpielstand', "-ExecCmds=`"py $Script`""
)
$Game = Start-Process -FilePath "$Engine\Binaries\Win64\UnrealEditor.exe" -ArgumentList $Arguments -PassThru
$Finished = $Game.WaitForExit($TimeoutSeconds * 1000)
if (-not $Finished) {
	Stop-Process -Id $Game.Id -Force
}
$Count = @(Get-ChildItem $Shots -Filter '*.png' -ErrorAction SilentlyContinue).Count
Write-Host "$Count Bilder in $Shots"
$Log = Join-Path $Root 'Saved\Logs\Eisenseelen.log'
$Broken = Select-String -Path $Log -Pattern 'Failed to compile Material|LogPython: Error'
if ($Broken) {
	$Broken | Select-Object -Last 5 | ForEach-Object { Write-Host $_.Line }
}
if (-not $Finished -or $Count -eq 0 -or $Broken) {
	Write-Host 'ROT: Die Vorschau ist nicht sauber durchgelaufen'
	exit 1
}
