$ErrorActionPreference = "Stop"

if (-not $env:DOCKER_BUILDKIT) {
    $env:DOCKER_BUILDKIT = "1"
}

if (-not (Get-Command docker -ErrorAction SilentlyContinue)) {
    throw "docker is required"
}

$image = if ($env:CATHOOK_DOCKER_IMAGE) { $env:CATHOOK_DOCKER_IMAGE } else { "cathook-builder:ubuntu24.04" }
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot ".")).Path
$dockerfilePath = Join-Path $repoRoot "docker\\builder.Dockerfile"

if (-not $env:CATHOOK_DOCKER_IMAGE) {
    $needsRebuild = $env:CATHOOK_DOCKER_REBUILD -eq "1"
    if (-not $needsRebuild) {
        $previousErrorActionPreference = $ErrorActionPreference
        $ErrorActionPreference = "Continue"
        & docker image inspect $image 2>$null 1>$null
        $ErrorActionPreference = $previousErrorActionPreference
        $needsRebuild = $LASTEXITCODE -ne 0
    }

    if ($needsRebuild) {
        docker build -t $image -f $dockerfilePath $repoRoot
    }
}

$containerScript = @'
set -euo pipefail
chmod +x build.sh
if [ "${CATHOOK_DOCKER_INSTALL_PACKAGES:-0}" = "1" ]; then
    chmod +x packages/packages.sh
    ./packages/packages.sh
fi
if [ "${CATHOOK_TEXTMODE:-0}" = "1" ]; then
    CATHOOK_TEXTMODE=1 ./build.sh
else
    ./build.sh
fi
'@
$containerScript = $containerScript -replace "`r`n", "`n"

docker run --rm `
    -e CATHOOK_TEXTMODE="${env:CATHOOK_TEXTMODE}" `
    -e CATHOOK_DOCKER_INSTALL_PACKAGES="${env:CATHOOK_DOCKER_INSTALL_PACKAGES}" `
    -v "${repoRoot}:/workspace" `
    -w /workspace `
    $image `
    bash -lc $containerScript
