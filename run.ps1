# ─────────────────────────────────────────────────────────────────────────────
#  run.ps1  —  Interactive wrapper for seam_carver.exe
#  Usage: Right-click the project folder → "Open in Terminal", then: .\run.ps1
# ─────────────────────────────────────────────────────────────────────────────

# Set OpenCV DLLs on PATH (needed every session)
$env:PATH = "C:\opencv\build\x64\vc16\bin;" + $env:PATH

$exe = ".\build\Release\seam_carver.exe"

Write-Host ""
Write-Host "=================================================" -ForegroundColor Cyan
Write-Host "   Seam Carving Engine  —  Interactive Runner    " -ForegroundColor Cyan
Write-Host "=================================================" -ForegroundColor Cyan

while ($true) {
    Write-Host ""

    # ── Input image ───────────────────────────────────────────────────────────
    $input = Read-Host "  Input image path  (e.g. input.jpg or C:\Photos\pic.jpg)"
    if (-not (Test-Path $input)) {
        Write-Host "  [ERROR] File not found: $input" -ForegroundColor Red
        continue
    }

    # ── Output image ──────────────────────────────────────────────────────────
    $output = Read-Host "  Output image path (e.g. output.jpg)"

    # ── Vertical seams ────────────────────────────────────────────────────────
    $vcropStr = Read-Host "  Vertical seams to remove  --vcrop  (shrinks WIDTH,  0 to skip)"
    $vcrop = [int]$vcropStr

    # ── Horizontal seams ──────────────────────────────────────────────────────
    $hcropStr = Read-Host "  Horizontal seams to remove --hcrop (shrinks HEIGHT, 0 to skip)"
    $hcrop = [int]$hcropStr

    # ── Energy function ───────────────────────────────────────────────────────
    Write-Host "  Energy function: [1] dual_gradient (default)  [2] laplacian"
    $efChoice = Read-Host "  Enter 1 or 2"
    $energy = if ($efChoice -eq "2") { "laplacian" } else { "dual_gradient" }

    # ── Visualization ─────────────────────────────────────────────────────────
    $vizPath = Read-Host "  Save seam visualization? Enter path (or press ENTER to skip)"

    # ── Benchmark ─────────────────────────────────────────────────────────────
    $benchChoice = Read-Host "  Show benchmark timing? (y/N)"
    $benchmark = $benchChoice -eq "y" -or $benchChoice -eq "Y"

    # ── Build command ─────────────────────────────────────────────────────────
    $args = @("`"$input`"", "`"$output`"")
    if ($vcrop -gt 0)       { $args += "--vcrop"; $args += $vcrop }
    if ($hcrop -gt 0)       { $args += "--hcrop"; $args += $hcrop }
    $args += "--energy"; $args += $energy
    if ($vizPath -ne "")    { $args += "--visualize"; $args += "`"$vizPath`"" }
    if ($benchmark)         { $args += "--benchmark" }

    Write-Host ""
    Write-Host "  Running: $exe $args" -ForegroundColor Yellow
    Write-Host ""

    & $exe $input $output `
        $(if ($vcrop -gt 0)    { "--vcrop",    $vcrop    }) `
        $(if ($hcrop -gt 0)    { "--hcrop",    $hcrop    }) `
        "--energy" $energy `
        $(if ($vizPath -ne "")  { "--visualize", $vizPath  }) `
        $(if ($benchmark)       { "--benchmark"            })

    # ── Continue? ─────────────────────────────────────────────────────────────
    Write-Host ""
    $again = Read-Host "  Process another image? (y/N)"
    if ($again -ne "y" -and $again -ne "Y") {
        Write-Host ""
        Write-Host "  Goodbye!" -ForegroundColor Green
        break
    }
}
