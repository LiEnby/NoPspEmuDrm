# NoPspEmuDrm:

A plugin to bypass all PSPEmu DRM Checks,
so you can play digital PSP or PS1 game backups; or of course- games you legitimately own but on another PSN Account

Defintely do not use this for piracy that would be bad mmkay?

Okay, now that message to keep the lawyers happy is out of the way --

Features: 

- Play any PSP, PSX, NeoGeo, PC Engine or TurboGrafix16 contents without a license
-- Directly from the livearea, as if you got it from the PlayStation Store, no Adrenaline.

- Use any PSP DLC Content (EDATs) without a license

- Use PocketStation functionality with PSX games that support it.
  note: requires "PocketStation App" to be installed.

- Start PspEmu content without NpDrm activation

- Force \_\_sce\_ebootpbp signature file validation success (means- psp bubbles wont be thanos snapped out of existance upon downgrading or changing accounts)

- Can use any Multi Disc game on your vita, even if they are not within \_\_sce\_discinfo.

# Comparison to Adrenaline
Adrenaline is an eCFW and basically boots an entire PSP 6.61 Custom Firmware to play games.
it can do almost everything a CFW PSP could do. including play games from ISO's use translation patches.
load plugins. and everything else. *BUT* as a result it kind of doesn't intergrate well with the rest of the vita OS
games in adrenaline are not recognized by Content Manager. leading to the infamous "system use"..
Cannot be background downloaded by pkgj, or anything.
and niche features such as pocketstation intergration with PS1 games, don't work.

if you want to create icons on your livearea you have to do so with sort of 'forwarders'
that just start adrenaline- and then start whatever game you want.

.. 

NoPspEmuDrm on the other hand; works *exactly* as if you downloaded a PlayStation Portable or PlayStation One Classics game from the PlayStation Store
which means that pretty much all of what i just said, works flawlessly, games *can* be background downloaded, *do* intergrate with the rest of the OS ..
DO show up in content manager, etc. 

BUT- that's kind of also its biggest downside.
because this ALSO means it has all the same limitations as offical psn PSP/PSX games

- cannot load any PSP-mode plugins, overclocking, custom ISO's homebrew, anything unsigned.
- same stripped down OFW PSP 6.61 firmware offical games run under, only the signature check on RIF/ACT.dat files is patched out. 
- same limited I/O access as offical PSPEmu, sandboxing and everything is still present.

# Installation:
The plugin consists of two parts; a kernel plugin and a user plugin;
both need to be installed for it to work correctly;

the config.txt entries you need are:

```
*KERNEL
ur0:/tai/NoPspEmuDrm_kern.skprx
*ALL
ur0:/tai/NoPspEmuDrm_user.suprx
```

# Installation of PSP Games:

----

There is now a build of PKGJ that supports background downoload of PSPEmu contents 

https://github.com/blastrock/pkgj/releases/tag/v0.57

Please let us know if you have any issues with this !

----
Manual Install:

Copy digital PSP/GAME folder to ux0:/pspemu/PSP/GAME

however crrently, VitaShell not support "Promoting" PSP games

However i have a fork of VitaShell that adds this in:

https://github.com/KuromeSan/VitaShell/releases

and on the main screen click triangle, press "refresh livearea"

----

( if you use pkgj version v0.55 you need to add `install_psp_as_pbp 1` to the config.txt )

( if you use nps browser with PKG2ZIP v2.3 or older; you need to add `-p` argument to the pkg2zip parameters )

----

# Note:
- this is not an eCFW and will not work with PSX2PSP or Homebrew applications - only official content. ( chovy-sign / sign_np will work though )

- First time startup of a game may take longer than later runs as it has to find the games decryption key, however it is cached and will be faster

- Game manual will only work after first time run in some games as it looks for the cached keys ..

- Aderenaline compatibility: 
PSP EBOOT.PBP games dont work in adrenaline by default because adrenaline does not patch npumdimg drm. (psx games should work fine however)
..
so to be able to use psp EBOOT games with adrenaline youll need to install the [npdrm_free](https://github.com/qwikrazor87/npdrm_free) psp plugin by qwikrazor.
..
this is not needed if you dont use adrenaline

# Credits:

Li         - Main dev; wrote all plugin code (except crypto/), Being transgender

Hykem      - kirk_engine, used to "emulate" alot of the KIRK security co-processor functions from the PSP.

SquallATF  - Wrote alot of the Chovy-Sign2/PspCrypto code that this is heavily based on; Also helped with EDAT. 

TheFlow    - Original NoNpDrm code- NoPspEmuDrm_kern is fork of NoNpDrm.
             and for the original Adrenaline v3.00, which i think i copied like 1 function from
			 which was for reading/writing to PspEmu memory,
 pla
