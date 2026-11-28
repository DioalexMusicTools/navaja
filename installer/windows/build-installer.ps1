$ErrorActionPreference = "Stop"
$bundle = "build/DC23Plugin_artefacts/Release/VST3/DC-23.vst3"
$binary = Join-Path $bundle "Contents/x86_64-win/DC-23.vst3"
if (-not (Test-Path -LiteralPath $binary -PathType Leaf)) { throw "Missing Windows x64 VST3 binary: $binary" }
if ((Get-Item -LiteralPath $binary).Length -lt 100000) { throw "Windows VST3 binary unexpectedly small" }
$signing = -not [string]::IsNullOrWhiteSpace($env:WINDOWS_SIGNING_PFX_BASE64)
if ($signing) {
  if ([string]::IsNullOrWhiteSpace($env:WINDOWS_SIGNING_PASSWORD)) { throw "Windows signing cert set without password" }
  $pfx = Join-Path $env:RUNNER_TEMP "dc23-windows.pfx"
  [IO.File]::WriteAllBytes($pfx,[Convert]::FromBase64String($env:WINDOWS_SIGNING_PFX_BASE64))
  $signtool = Get-ChildItem "C:\Program Files (x86)\Windows Kits\10\bin" -Filter signtool.exe -Recurse -File | Where-Object { $_.FullName -like "*\x64\signtool.exe" } | Sort-Object FullName -Descending | Select-Object -First 1 -ExpandProperty FullName
  if (-not $signtool) { throw "signtool.exe unavailable" }
  & $signtool sign /f $pfx /p $env:WINDOWS_SIGNING_PASSWORD /fd SHA256 /tr "http://timestamp.digicert.com" /td SHA256 $binary
  if ($LASTEXITCODE -ne 0) { throw "VST3 Authenticode signing failed" }
}
New-Item -ItemType Directory -Force -Path "installer/windows/payload" | Out-Null
Copy-Item -LiteralPath $bundle -Destination "installer/windows/payload" -Recurse -Force
$iscc = "C:\Program Files (x86)\Inno Setup 6\ISCC.exe"
if (-not (Test-Path -LiteralPath $iscc)) {
  choco install innosetup --yes --no-progress
  if ($LASTEXITCODE -ne 0) { throw "Inno Setup installation failed" }
}
if (-not (Test-Path -LiteralPath $iscc)) { throw "Inno Setup 6 unavailable" }
New-Item -ItemType Directory -Force -Path dist | Out-Null
& $iscc "installer\windows\DC23.iss"
if ($LASTEXITCODE -ne 0) { throw "Inno Setup compilation failed" }
$exe = "dist/DC-23-v1.0-Windows-x64-Setup.exe"
if (-not (Test-Path -LiteralPath $exe -PathType Leaf)) { throw "Missing EXE setup installer" }
if ((Get-Item -LiteralPath $exe).Length -lt 100000) { throw "EXE setup installer unexpectedly small" }
if ($signing) {
  & $signtool sign /f $pfx /p $env:WINDOWS_SIGNING_PASSWORD /fd SHA256 /tr "http://timestamp.digicert.com" /td SHA256 $exe
  if ($LASTEXITCODE -ne 0) { throw "Installer signing failed" }
} else {
  Write-Warning "Unsigned Windows installer. Code-sign before public distribution."
}
Get-FileHash -Algorithm SHA256 $exe | Format-List
