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
Copy digital PSP/GAME folder to ux0:/pspemu/PSP/GAME

( if you use pkgj version v0.55 you need to add `install_psp_as_pbp 1` to the config.txt )
( if you use nps browser with PKG2ZIP v2.3 or older; you need to add `-p` argument to the pkg2zip parameters )

-- 
Currently, neither VitaShell or PKGJ support "Promoting" PSP games

However i have a fork of VitaShell that adds this in:

https://github.com/KuromeSan/VitaShell/releases

and on the main screen click triangle, press "refresh livearea"

--
I also got background downloading working; using our original bgdl proof of concept;

https://github.com/KuromeSan/bgdl_nopspemudrm_poc

So PKGJ support for it should be availible soon

# Note:
- this is not an eCFW and will not work with PSX2PSP or Homebrew applications - only official content. ( chovy-sign / sign_np will work though )

- First time startup of a game may take longer than later runs as it has to find the games decryption key, however it is cached after that and will be faster

- PSP EBOOT.PBP games dont work in adrenaline by default because adrenaline do not patch npumd drm.
(can be fixed with npdrm_free psp plugin by qwikrazor however.)

# Credits:

Li         - Main dev; wrote all plugin code (except crypto/), Being transgender

Hykem      - kirk_engine, used to "emulate" alot of the KIRK security co-processor functions from the PSP.

SquallATF  - Wrote alot of the Chovy-Sign2/PspCrypto code that this is heavily based on; Also helped with EDAT. 

TheFlow    - Original NoNpDrm code- NoPspEmuDrm_kern is fork of NoNpDrm.
             and for the original Adrenaline v3.00, which i think i copied like 1 function from
			 which was for reading/writing to PspEmu memory,
