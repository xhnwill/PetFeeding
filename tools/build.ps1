param([string]$IdfPath="D:\Espressif\frameworks\esp-idf-v5.5.4",[string]$ToolsPath="D:\Espressif")
$ErrorActionPreference="Stop"
$env:IDF_PATH=$IdfPath
$env:IDF_TOOLS_PATH=$ToolsPath
$taskPython=Join-Path $ToolsPath 'python_env\idf5.5_py3.11_env\Scripts\python.exe'
$taskExport=& $taskPython (Join-Path $IdfPath 'tools\idf_tools.py') export --format key-value
if($LASTEXITCODE){throw "IDF tools export failed"}
foreach($taskLine in $taskExport){if($taskLine -match '^([^=]+)=(.*)$'){
 $taskValue=$Matches[2].Replace('%PATH%',$env:PATH)
 [Environment]::SetEnvironmentVariable($Matches[1],$taskValue,'Process')
}}
$env:IDF_PYTHON_CHECK_CONSTRAINTS="no"
Push-Location (Join-Path $PSScriptRoot '..\firmware')
try {
 & $taskPython (Join-Path $IdfPath 'tools\idf.py') build
 if($LASTEXITCODE){throw "Build failed"}
 & $taskPython (Join-Path $IdfPath 'tools\idf.py') merge-bin -o mypat-v0.2.0-full.bin
 if($LASTEXITCODE){throw "Merge failed"}
} finally {Pop-Location}
