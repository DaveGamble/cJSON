# Script to patch Appveyor build environment for Visual Studio 2008 64bit

$url = "https://github.com/menpo/condaci/raw/master/vs2008_patch.zip"
$output = "$pwd\build\vs2008_patch.zip"

(New-Object System.Net.WebClient).DownloadFile($url, $output)

7z -e "$pwd\build\vs2008_patch.zip"
cmd.exe /c "$pwd\build\vs2008_patch\setup_x64.bat"


