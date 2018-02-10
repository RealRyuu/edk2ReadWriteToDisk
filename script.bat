diskpart /s C:\edk2\diskpartScript.txt
del F:\myApp.efi
copy C:\edk2\Build\myPkg\DEBUG_VS2015x86\IA32\myApp.efi F:\myApp.efi
timeout 1
diskpart /s C:\edk2\diskpartScript2.txt
VBoxManage startvm "tempo"