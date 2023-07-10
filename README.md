# Persona 4 Golden HD Patch

[Download binary](https://forum.devchroma.nl/index.php/topic,193.0.html) | [Report bugs](https://github.com/cuevavirus/hdpatch/issues) | [Source code](https://git.shotatoshounenwachigau.moe/vita/p4goldenhd/)

This patch changes the 3D render and framebuffer resolutions of Persona 4 Golden on the Vita and PSTV to 1920x1080 or 1280x720. 1920x1080 can be output to HDMI, 1280x720 can be output to HDMI or USB ([udcd-uvc](https://github.com/xerpi/vita-udcd-uvc)), or Vita users can enjoy a supersampled image directly on the screen.

Preview images (1920x1080): [1](https://git.shotatoshounenwachigau.moe/vita/p4goldenhd/plain/preview1.png?h=assets) [2](https://git.shotatoshounenwachigau.moe/vita/p4goldenhd/plain/preview2.png?h=assets) [3](https://git.shotatoshounenwachigau.moe/vita/p4goldenhd/plain/preview3.png?h=assets) [4](https://git.shotatoshounenwachigau.moe/vita/p4goldenhd/plain/preview4.png?h=assets)

## Installation

1. Install Persona 4 Golden and install the latest update, if available.
2. Install the latest version of [Sharpscale](https://forum.devchroma.nl/index.php/topic,112.0.html) and the configuration app.
3. Turn on "Enable Full HD" in the Sharpscale configuration app.
4. Install `p4goldenhd.suprx` under the appropriate title ID in your taiHEN config.
    - `PCSG00004` Japan
    - `PCSG00563` Japan (reprint)
    - `PCSE00120` North America
    - `PCSB00245` Europe and Australia
    - `PCSH00021` Asia

For example,

```
*PCSG00563
ur0:/tai/p4goldenhd.suprx
```

## Performance

Overclocking is required for good performance. With 1920x1080, framerate ranges between 20-30 FPS, with 25-30 FPS in all but the most graphically intensive areas. With 1280x720, framerate is 30 FPS.

## Building

Dependencies:

- [DolceSDK](https://forum.devchroma.nl/index.php/topic,129.0.html)
- [fnblit and bit2sfn](https://git.shotatoshounenwachigau.moe/vita/fnblit)
- [Terminus font](http://terminus-font.sourceforge.net)
- [taiHEN](https://git.shotatoshounenwachigau.moe/vita/taihen)

To build for 1920x1080 mode, pass `-DFB_FHD=1` when invoking `cmake`.

## Contributing

Use [git-format-patch](https://www.git-scm.com/docs/git-format-patch) or [git-request-pull](https://www.git-scm.com/docs/git-request-pull) and email me at <asakurareiko@protonmail.ch>.

## Credits

- 3D buffer offsets: [VitaGrafix patchlist](https://github.com/Electry/VitaGrafixPatchlist)
- Author: 浅倉麗子

## See more

CBPS ([forum](https://forum.devchroma.nl/index.php), [discord](https://discordapp.com/invite/2ccAkg3))
