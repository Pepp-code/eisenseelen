<#
Erzeugt Materialien, Toene und die Testkarte neu, ohne Fenster.

    pwsh tools/rebuild_content.ps1
#>
$ErrorActionPreference = 'Stop'
$Engine = 'C:\Games\UE_5.8\Engine'
$Root = Split-Path $PSScriptRoot -Parent
$Project = Join-Path $Root 'Eisenseelen.uproject'
$Log = Join-Path $env:TEMP 'eisenseelen_rebuild.txt'

py -3.12 (Join-Path $PSScriptRoot 'make_sounds.py')
if ($LASTEXITCODE -ne 0) {
	Write-Host 'ROT: Toene konnten nicht erzeugt werden'
	exit 1
}

foreach ($Script in 'make_assets.py', 'make_testarena.py', 'make_runmap.py') {
	$ScriptPath = (Join-Path $PSScriptRoot $Script) -replace '\\', '/'
	$Arguments = @("`"$Project`"", '-run=pythonscript', "-script=$ScriptPath", '-unattended', '-nullrhi', '-nosplash')
	$Editor = Start-Process -FilePath "$Engine\Binaries\Win64\UnrealEditor-Cmd.exe" -ArgumentList $Arguments -PassThru -WindowStyle Hidden -RedirectStandardOutput $Log
	if (-not $Editor.WaitForExit(400000)) {
		Stop-Process -Id $Editor.Id -Force
		Write-Host "ROT: $Script nach 400 Sekunden abgebrochen"
		exit 1
	}
	$Errors = Select-String -Path $Log -Pattern 'LogPython: Error|PythonScriptCommandlet: Error|Failed to compile'
	if ($Editor.ExitCode -ne 0 -or $Errors) {
		$Errors | Select-Object -Last 8 | ForEach-Object { Write-Host $_.Line }
		Write-Host "ROT: $Script fehlgeschlagen"
		exit 1
	}
	Write-Host "$Script fertig"
}
Write-Host 'GRUEN: Inhalte neu erzeugt'
