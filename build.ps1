Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$project_root = Split-Path -Parent $MyInvocation.MyCommand.Definition
$shaders_dir = "$project_root/shaders"
$build_dir = "$project_root/build"
$exe_path = "$build_dir/bin/app.exe"

if (-not (Test-Path $build_dir)) {
    Write-Error "❌ No build directory in project root."
}

function Confirm-Tasks {
    param (
        [string]$Message,
        [string[]]$Tasks
    )

    $input = Read-Host "$Message (Enter to continue, anything else to cancel)"
    if ([string]::IsNullOrWhiteSpace($input)) {
        foreach ($task in $Tasks) {
            Invoke-Expression $task
        }
    } else {
        Write-Host "⚠️ Cancelled."
    }
}

Set-Location $project_root

Confirm-Tasks "Compile shaders?" @(
    "glslc $shaders_dir\shader.glsl.frag -o $shaders_dir\compiled\shader.frag.spv",
    "glslc $shaders_dir\shader.glsl.vert -o $shaders_dir\compiled\shader.vert.spv"
)

Write-Host "🔧 Compiling project..."
cmake --build $build_dir

Confirm-Tasks "Run application?" @(
    {
        & $exe_path
    }
)
