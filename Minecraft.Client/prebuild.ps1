$ErrorActionPreference = "Stop"
$sha = "unknown"
$ref = "unknown/unknown"
try {
    $sha = (git rev-parse --short=7 HEAD 2>$null)
    if (-not $sha) { $sha = "unknown" }
} catch { $sha = "unknown" }

try {
    if ($env:GITHUB_REPOSITORY) {
        $branch = (git symbolic-ref --short HEAD 2>$null)
        if ($branch) { $ref = "$env:GITHUB_REPOSITORY/$branch" }
    } else {
        $remoteUrl = (git remote get-url origin 2>$null)
        if ($remoteUrl -match '(?:github\.com[:/])([^/:]+/[^/]+?)(?:\.git)?$') {
            $branch = (git symbolic-ref --short HEAD 2>$null)
            $ref = "$($matches[1])/$branch"
        } else {
            $branch = (git symbolic-ref --short HEAD 2>$null)
            $ref = "UNKNOWN/$branch"
        }
    }
} catch { }

$build = 560 # Note: Build/network has to stay static for now, as without it builds wont be able to play together. We can change it later when we have a better versioning scheme in place.
$suffix = ""

# TODO Re-enable
# If we are running in GitHub Actions, use the run number as the build number
# if ($env:GITHUB_RUN_NUMBER) {
#     $build = $env:GITHUB_RUN_NUMBER
# }

# If we have uncommitted changes, add a suffix to the version string
try {
    if (git status --porcelain 2>$null) { $suffix = "-dev" }
} catch { }

@"
#pragma once

#define VER_PRODUCTBUILD $build
#define VER_PRODUCTVERSION_STR_W L"$sha$suffix"
#define VER_FILEVERSION_STR_W VER_PRODUCTVERSION_STR_W
#define VER_BRANCHVERSION_STR_W L"$ref"
#define VER_NETWORK VER_PRODUCTBUILD
"@ | Set-Content "Common/BuildVer.h"
exit 0
