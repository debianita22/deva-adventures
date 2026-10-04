# Deva's Awesome Adventures - the Windows packages of release\ tried on Windows (1.2). The CI runs it
# on a Windows runner after make dist; by hand, from the folder of the sources:
#   pwsh tools/release/try_windows.ps1
#
# The program of the zip: --version, --check, a short game with SDL's own video and sound and no
# screen (600 frames, then out in order), the save whole and in LF. Then the installer, silent: the
# files, the Start menu, "App installate", the installed program (--check and a short game), and the
# silent uninstall. A failure also becomes an error annotation ("::error"), which the checks API of
# GitHub shows without the raw log.
$ErrorActionPreference = 'Stop'
$env:DEVA_NO_DIALOG = '1'
$W = Join-Path ([IO.Path]::GetTempPath()) 'deva-try'
Remove-Item -Recurse -Force $W -ErrorAction SilentlyContinue
New-Item -ItemType Directory $W | Out-Null

function Run([string]$exe, [string[]]$argv, [string]$what) {
    $out = Join-Path $W 'out.txt'
    $err = Join-Path $W 'err.txt'
    $p = Start-Process $exe -ArgumentList $argv -Wait -PassThru -NoNewWindow `
        -RedirectStandardOutput $out -RedirectStandardError $err
    $text = "$(Get-Content $out -Raw)$(Get-Content $err -Raw)".Trim()
    Write-Host $text # (shown in the log; the function gives back only the text)
    if ($p.ExitCode -ne 0) { throw "${what}: exit $($p.ExitCode)`n$text" }
    return $text
}

function Game([string]$exe, [string]$saves, [string]$what) {
    # SDL's own video and sound, no screen: the game runs 600 frames and closes in order
    $env:SDL_VIDEODRIVER = 'dummy'; $env:SDL_AUDIODRIVER = 'dummy'; $env:DEVA_TEST_FRAMES = '600'
    try {
        Run $exe @('--verbose', '--saves', $saves) $what | Out-Null
    } finally {
        Remove-Item Env:SDL_VIDEODRIVER, Env:SDL_AUDIODRIVER, Env:DEVA_TEST_FRAMES
    }
    $log = Get-Content (Join-Path $saves 'deva-adventures.log')
    Write-Host (($log | Select-Object -Last 2) -join "`n")
    if (-not ($log -match '\d+ frames in')) { throw "${what}: no frames in the log" }
    $sav = [IO.File]::ReadAllText((Join-Path $saves 'deva_adventures.sav'))
    if (-not $sav.EndsWith("# fine`n")) { throw "${what}: the save is not whole" }
    if ($sav.Contains("`r")) { throw "${what}: the save has CR-LF, not the bytes of the other systems" }
    Write-Host "== ${what}: ok"
}

try {
    Write-Output '== the program of the zip'
    $zip = (Get-Item release/deva-adventures-*-windows-x64.zip).FullName
    Expand-Archive $zip -DestinationPath (Join-Path $W 'zip')
    $dir = (Get-ChildItem (Join-Path $W 'zip') -Directory)[0].FullName
    foreach ($f in 'deva-adventures.exe', 'SDL2.dll', 'LEGGIMI.txt', 'LICENSE-SDL2.txt', 'LICENSE-mingw-w64.txt') {
        if (-not (Test-Path (Join-Path $dir $f))) { throw "the zip has no $f" }
    }
    $exe = Join-Path $dir 'deva-adventures.exe'
    Run $exe @('--version') 'version' | Out-Null
    $check = Run $exe @('--check') 'check'
    if ($check -notmatch 'tutto pronto') { throw "check: not ready`n$check" }
    Game $exe (Join-Path $W 'saves') 'a game from the zip'

    Write-Output '== the installer'
    $setup = (Get-Item release/deva-adventures-*-windows-x64-setup.exe).FullName
    $p = Start-Process $setup -ArgumentList '/S' -Wait -PassThru
    if ($p.ExitCode -ne 0) { throw "setup /S: exit $($p.ExitCode)" }
    $i = Join-Path $env:LOCALAPPDATA "Programs\Deva's Awesome Adventures"
    foreach ($f in 'deva-adventures.exe', 'SDL2.dll', 'deva_adventures\gfx\atlas.txt', 'docs\manuale.pdf', 'uninstall.exe') {
        if (-not (Test-Path (Join-Path $i $f))) { throw "not installed: $f" }
    }
    $lnk = Join-Path ([Environment]::GetFolderPath('Programs')) "Deva's Awesome Adventures.lnk"
    if (-not (Test-Path $lnk)) { throw 'no shortcut in the Start menu' }
    $key = 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\DevaAdventures'
    $entry = Get-ItemProperty $key
    Write-Output "App installate: $($entry.DisplayName) $($entry.DisplayVersion) in $($entry.InstallLocation)"
    $iexe = Join-Path $i 'deva-adventures.exe'
    $check = Run $iexe @('--check') 'installed check'
    if ($check -notmatch 'tutto pronto') { throw "installed check: not ready`n$check" }
    Game $iexe (Join-Path $W 'saves2') 'a game from the installed program'

    Write-Output '== the uninstaller'
    # (it copies itself to %TEMP% and goes on from there: wait for it)
    Start-Process (Join-Path $i 'uninstall.exe') -ArgumentList '/S' -Wait
    for ($k = 0; $k -lt 60 -and ((Test-Path $iexe) -or (Test-Path $key)); $k++) { Start-Sleep 1 }
    if (Test-Path $iexe) { throw 'still installed after uninstall /S' }
    if (Test-Path $key) { throw 'still in App installate after uninstall /S' }
    if (Test-Path $lnk) { throw 'the Start menu shortcut is still there' }
    Write-Output '== installed, played and removed: ok'
    $os = (Get-CimInstance Win32_OperatingSystem).Caption
    Write-Output "::notice title=Windows packages::${os}: the zip, the installer, the installed game and the uninstall ok"
} catch {
    $msg = ($_ | Out-String).Trim()
    $log = Join-Path $W 'saves\deva-adventures.log'
    if (Test-Path $log) { $msg += "`n-- deva-adventures.log:`n" + ((Get-Content $log | Select-Object -Last 15) -join "`n") }
    Write-Output $msg
    $esc = $msg -replace '%', '%25' -replace "`r", '' -replace "`n", '%0A'
    Write-Output "::error title=Windows packages::$esc"
    exit 1
}
