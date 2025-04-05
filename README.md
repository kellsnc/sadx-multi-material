# SADX Multi Material
This mod adds multiple material support to the basic model format. For example, one can apply an additive environment texture on top of a surface to simulate reflection.

It supports up to 8 stages with a variety of blending options. Models using this feature still work without the mod; only the first texture will render.

> [!WARNING]  
> This mod is not compatible and will have no effect with the Lantern Engine mod.

## 📋 How to use
The basic model format provides 7 bits of user flags for materials. This mod reserves the first bit as a "multi material" flag. When this flag is set, the next material in the list is used as a secondary material.

For additional materials, alpha blending options are replaced with stage blending options. "Source Alpha" and "Destination Alpha" become respectively "Color Operation" and "Alpha Operation". The values become, in order:
* Current: Use the new texture, disregard the previous one
* Previous: Use the previous texture, disregard the new one
* Add: Add the new texture to the previous (additive blending)
* Subtract: Subtract the new texture from the previous
* Multiply: Multiply the new texture with the previous
* Linear interpolation using current texture alpha
* Linear interpolation using previous alpha
* Linear interpolation using current specular alpha

For example, to add an additive environment map to a model's material:
1. Set the multi material flag on the material
    * In SAMDL, type "20" as a user flag
2. Add a material below, enable texture and environment map.
    * In SAMDL, check "Use Texture" and "Environment Map"
3. Set the color operation to add and alpha operation to previous
    * In SAMDL, use "Other" (Add) as Source Alpha
    * Use "One" (Previous) as Destination Alpha and/or uncheck "Use Alpha"

## 📥 How to install
You need the following prerequisites:
* [SA Mod Manager](https://github.com/X-Hax/SA-Mod-Manager)
* [Visual C++ Redistributable Runtimes (32 Bits)](https://aka.ms/vs/17/release/vc_redist.x86.exe)

Download the mod archive [here](https://github.com/kellsnc/sadx-multi-material/releases/latest) and extract it into your "mods" folder. Then, enable the mod in the Mod Manager.

## 🤝 Contributions
If you encounter issues, please report them as an [issue](https://github.com/kellsnc/sadx-multi-material/issues) or on the [X-Hax discord](https://discord.gg/gqJCF47).

## 🛠 How to build:
You need the following prerequisites on Windows:
* Visual Studio 2019/2022 with the v141 toolkit with XP support.
* The [DirectX 8.1 SDK](https://archive.org/details/dx81sdk_full)

Simply pull the repository, then open and build the solution.

## ©️ Disclaimer
This mod is a fan-made project and is not affiliated with SEGA or Sonic Team. All trademarks and intellectual properties belong to their respective owners. A copy of the game is needed.