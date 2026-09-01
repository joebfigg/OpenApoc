# Dump every frame of a battle unit image pack to numbered PNGs, as pose
# references for restyling. Output is DERIVED FROM PROPRIETARY GAME DATA:
# keep it outside the repo (default output is the local scratch dir).
#
# Usage: .\dump_pack_frames.ps1 -Pack xcom1a [-OutDir <dir>]
param(
    [Parameter(Mandatory = $true)][string]$Pack,
    [string]$OutDir = "C:\Users\joebf\Developer\scratch\openapoc-setup\pack_frames",
    [string]$RepoRoot = "C:\Users\joebf\Developer\projects\openapoc",
    [string]$Palette = "xcom3/tacdata/tactical.pal"
)
# ImageDump logs routine warnings to stderr; don't let those become terminating
$ErrorActionPreference = 'Continue'
$dump = Join-Path $RepoRoot 'build\bin\OpenApoc_ImageDump.exe'
$packFile = Join-Path $RepoRoot "data\imagepacks\$Pack"
$dst = Join-Path $OutDir $Pack
New-Item -ItemType Directory -Force -Path $dst | Out-Null

# The pack is a zip holding imagepack.xml with one <entry> per frame
$tmp = Join-Path $env:TEMP ("pack_" + $Pack)
if (Test-Path $tmp) { Remove-Item $tmp -Recurse -Force }
Add-Type -AssemblyName System.IO.Compression.FileSystem
[System.IO.Compression.ZipFile]::ExtractToDirectory($packFile, $tmp)
$xml = [xml](Get-Content (Join-Path $tmp 'imagepack.xml') -Raw)

Set-Location $RepoRoot
$i = 0
foreach ($entry in $xml.imagepack.images.entry) {
    $s = "$entry"
    if ($s -ne '') {
        $out = Join-Path $dst ("{0:D3}.png" -f $i)
        & $dump ($s + ':' + $Palette) $out 2>$null | Out-Null
        if (-not (Test-Path $out)) { & $dump $s $out 2>$null | Out-Null }
    }
    $i++
}
$done = (Get-ChildItem $dst -Filter *.png).Count
Write-Output "dumped $done frames of $i entries -> $dst"
