Set-Location $PSScriptRoot

$shadercross  = "tools/shadercross.exe"
$shaderSrcDir = "assets/shaders/src"
$shaderOutDir = "assets/shaders/out"
$formats = @("spv", "dxil", "msl")

Get-ChildItem $shaderSrcDir -File | ForEach-Object {
    $basename = $_.BaseName
    foreach ($format in $formats) {
        $outFile = "$shaderOutDir/$basename.$format"
        & $shadercross $_.FullName -o $outFile
        if ($LASTEXITCODE -ne 0) {
            Write-Host "> ಠ_ಠ failed: $($_.Name) → $format"
            exit 1
        }
    }
}

cmake --build build/
if ($LASTEXITCODE -ne 0) {
    Write-Host "> ಠ_ಠ build failed"
    exit 1
}

& "$PSScriptRoot/build/bin/app.exe"
