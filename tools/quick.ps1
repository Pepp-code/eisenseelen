<#
Schneller Testlauf fuer die taegliche Arbeit.
Dauert nur ca. 15-30 Sekunden statt 7 Minuten.

Beispiele:
    pwsh tools/quick.ps1                 # Schnelle Logik- und Datentests (Standard)
    pwsh tools/quick.ps1 -Thema kampf    # Schwert, Stab, Zauber
    pwsh tools/quick.ps1 -Thema insel    # Seelenkessel, Insel-Events, Fracht
    pwsh tools/quick.ps1 -Thema daten    # Alle JSON-Dateien
    pwsh tools/quick.ps1 -Thema menue    # Pause-Menue & Einstellungen
    pwsh tools/quick.ps1 -Thema bewegung # Sprint, Sprung, Luftsteuerung
    pwsh tools/quick.ps1 -Filter <Name>  # Beliebiger eigener Testfilter
    pwsh tools/quick.ps1 -NoBuild        # Ohne Kompilieren
#>
param(
	[string]$Thema = 'basis',
	[string]$Filter = '',
	[switch]$NoBuild,
	[int]$TimeoutMinutes = 3
)

$ErrorActionPreference = 'Stop'

# Thema in konkrete Test-Filter aufloesen
if (-not $Filter) {
	switch ($Thema.ToLower()) {
		'basis'    { $Filter = 'Eisenseelen.Run' }
		'daten'    { $Filter = 'Eisenseelen.Data' }
		'insel'    { $Filter = 'Eisenseelen.Run.SoulCargoAndKettleOvercharge' }
		'kampf'    { $Filter = 'Eisenseelen.Play.Sword' }
		'stab'     { $Filter = 'Eisenseelen.Play.Staff' }
		'gegner'   { $Filter = 'Eisenseelen.Play.Enemy' }
		'menue'    { $Filter = 'Eisenseelen.Play.Pause' }
		'bewegung' { $Filter = 'Eisenseelen.Play.Movement' }
		default    { $Filter = $Thema }
	}
}

$CheckScript = Join-Path $PSScriptRoot 'check.ps1'
$Params = @{
	Filter = $Filter
	TimeoutMinutes = $TimeoutMinutes
}
if ($NoBuild) {
	$Params['NoBuild'] = $true
}

& $CheckScript @Params
