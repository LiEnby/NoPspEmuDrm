# NoPspEmuDrm

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

# Installation
The plugin consists of two parts; a kernel plugin and a user plugin;
both need to be installed for it to work correctly;

the config.txt entries you need are: 

```
*KERNEL
ur0:/tai/NoPspEmuDrm_kern.skprx
*ALL
ur0:/tai/NoPspEmuDrm_user.suprx
```

# Credits

Li         - Main dev; wrote all plugin code (except crypto/), Being transgender

Hykem      - kirk_engine, used to "emulate" alot of the KIRK security co-processor functions from the PSP.

SquallATF  - Wrote alot of the Chovy-Sign2/PspCrypto code that this is heavily based on; Also helped with EDAT. 

TheFlow    - Original NoNpDrm code- NoPspEmuDrm_kern is fork of NoNpDrm.
             and for the original Adrenaline v3.00, which i think i copied like 1 function from
			 which was for reading/writing to PspEmu memory,

