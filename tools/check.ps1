<#
Kompiliert das Projekt und startet alle Tests ohne Fenster.

    pwsh tools/check.ps1            kompilieren und testen
    pwsh tools/check.ps1 -NoBuild   nur testen

Exit-Code 0 heisst: alles gruen.
#>
param(
	[switch]$NoBuild,
	[string]$Filter = 'Eisenseelen',
	[int]$TimeoutMinutes = 15
)

$ErrorActionPreference = 'Stop'
$Engine = 'C:\Games\UE_5.8\Engine'
$Root = Split-Path $PSScriptRoot -Parent
$Project = Join-Path $Root 'Eisenseelen.uproject'
$Report = Join-Path $Root 'Saved\TestReport'

if (-not $NoBuild) {
	& "$Engine\Build\BatchFiles\Build.bat" EisenseelenEditor Win64 Development "-Project=$Project" -WaitMutex -NoHotReload
	if ($LASTEXITCODE -ne 0) {
		Write-Host 'ROT: Kompilieren fehlgeschlagen'
		exit 1
	}
}

if (Test-Path $Report) {
	Remove-Item $Report -Recurse -Force
}
$Arguments = @(
	"`"$Project`"", '-nullrhi', '-unattended', '-nopause', '-nosplash', '-nosound',
	"-ReportExportPath=`"$Report`"",
	"-ExecCmds=`"Automation RunTests $Filter; Quit`"",
	'-TestExit="Automation Test Queue Empty"'
)
$Editor = Start-Process -FilePath "$Engine\Binaries\Win64\UnrealEditor-Cmd.exe" -ArgumentList $Arguments -PassThru -WindowStyle Hidden
if (-not $Editor.WaitForExit($TimeoutMinutes * 60000)) {
	Stop-Process -Id $Editor.Id -Force
	Write-Host "ROT: Tests nach $TimeoutMinutes Minuten abgebrochen"
	exit 1
}

$Index = Join-Path $Report 'index.json'
if (-not (Test-Path $Index)) {
	Write-Host 'ROT: Kein Testbericht entstanden, siehe Saved\Logs\Eisenseelen.log'
	exit 1
}

$Result = Get-Content $Index -Raw | ConvertFrom-Json
foreach ($Test in $Result.tests) {
	Write-Host ("{0,-8} {1}" -f $Test.state, $Test.fullTestPath)
	foreach ($Entry in $Test.entries) {
		if ($Entry.event.type -eq 'Error') {
			Write-Host "         $($Entry.event.message)"
		} elseif ($Entry.event.message -match ' cm') {
			Write-Host "         $($Entry.event.message)"
		}
	}
}
$Total = @($Result.tests).Count
$Green = @($Result.tests | Where-Object { $_.state -eq 'Success' }).Count
if ($Total -eq 0 -or $Green -ne $Total) {
	Write-Host "ROT: $Green von $Total Tests gruen"
	exit 1
}
Write-Host "GRUEN: $Green von $Total Tests"
