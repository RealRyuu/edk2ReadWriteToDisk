what we need:
	-edk2,
	-virtualbox with machine witch attatched fat formated disk,

how to compile:
	-put pkg folder in edk2 directory,
	-configure edk2 properly according to edk2 instructions,
	-with visual studio cmd in admin mode hit:
		- build -p myPkg\myPkg.dsc -a IA32 -m myPkg\Applications\myApp\myApp.inf
	-copy to the disk (you can modify script.bat and use it)