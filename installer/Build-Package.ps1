[CmdletBinding()]
param(
    [ValidateSet('Debug', 'RelWithDebInfo', 'Release', 'MinSizeRel')]
    [string] $Configuration = 'Release',
    [string] $BuildDirectory = 'build_x64',
    [string] $IsccPath = ''
)

$ErrorActionPreference = 'Stop'
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$BuildSpec = Get-Content -LiteralPath (Join-Path $ProjectRoot 'buildspec.json') -Raw | ConvertFrom-Json
$OutputRoot = Join-Path $ProjectRoot 'release'
$StageRoot = Join-Path $OutputRoot $Configuration
$PackageRoot = Join-Path $StageRoot $BuildSpec.name
$BuildPath = if ([IO.Path]::IsPathRooted($BuildDirectory)) { $BuildDirectory } else { Join-Path $ProjectRoot $BuildDirectory }

& cmake --install $BuildPath --config $Configuration --prefix $StageRoot
if ($LASTEXITCODE -ne 0) { throw "CMake install failed with exit code $LASTEXITCODE." }

$PluginPath = Join-Path $PackageRoot 'bin\64bit\obs-chzzk-alerts.dll'
$PluginVersion = (Get-Item -LiteralPath $PluginPath).VersionInfo.ProductVersion
if ($PluginVersion -ne $BuildSpec.version) {
    throw "Built DLL version $PluginVersion does not match buildspec version $($BuildSpec.version). Rebuild first."
}

& (Join-Path $PSScriptRoot 'Build-Installer.ps1') -Configuration $Configuration -IsccPath $IsccPath

$ArchivePath = Join-Path $OutputRoot "$($BuildSpec.packageName)-$($BuildSpec.version)-windows-x64.zip"
Compress-Archive -LiteralPath (Join-Path $PackageRoot 'bin'), (Join-Path $PackageRoot 'data') -DestinationPath $ArchivePath -Force
$InstallerPath = Join-Path $OutputRoot "$($BuildSpec.packageName)-$($BuildSpec.version)-windows-x64-setup.exe"
$Checksums = foreach ($ArtifactPath in @($InstallerPath, $ArchivePath)) {
    $Hash = (Get-FileHash -LiteralPath $ArtifactPath -Algorithm SHA256).Hash.ToLowerInvariant()
    '{0}  {1}' -f $Hash, (Split-Path -Leaf $ArtifactPath)
}
$ChecksumPath = Join-Path $OutputRoot 'SHA256SUMS.txt'
Set-Content -LiteralPath $ChecksumPath -Value $Checksums -Encoding ascii
Write-Output $InstallerPath, $ArchivePath, $ChecksumPath
