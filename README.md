# Mageia-Prime

This tool allows you to easily configure NVidia Prime for using the
discrete graphics card of a laptop with the NVidia proprietary drivers
for the Mageia GNU/Linux distribution.

Most of nowadays laptops have a graphic subsystem called
"Hybrid-Graphics", in other words they come with two graphics cards: one is
"internal" (or "*integrated*" into the CPU), and is called IGP
(Integrated Graphic Processor), and the other is "dedicated", external
to the CPU, and it is called "*discrete*" (DGP, Discrete/Dedicated
Graphics Processor).

So a typical laptop configuration could have two cards: one internal, usually an
Intel graphics card (e.g. Intel HD Graphics 530), and another discrete, typical
an NVidia graphics card (e.g. NVidia GeForce GTX 960M), though other
configurations are possible.

The default X11 server installation is typically using the internal Intel
graphics card, which is usually slower (but consumes less energy) than the
discrete one. Other softwares to access to the discrete
graphics card with the proprietary drivers use to "mix" the
output of both (integrated and discrete) cards into a single one: this
is the case of using the Linux utility "*bumblebee*". However this latter
method could result, sometimes, in getting slower performance than the native
internal graphics card.


## Usage:

To **configure** the NVidia graphics card with Prime, run (as root) the
following command:

	/usr/sbin/mageia-prime-install

this will switch the default configuration in /etc/X11/xorg.conf to
one suitable for NVidia prime. The Mageia RPM packages for NVidia
proprietary drivers should be already installed before issuing this
command, otherwise mageia-prime-install will install the required
packages for you. After running such command you need to restart the
X11 server or alternatively reboot the machine. At this point the
switch to the discrete card it is completed. At the next reboot or
the subsequent restart of the X11 server, you can check that the GL
libraries are really coming from NVidia drivers. The command "`glxinfo
-B`" can be used for this purpose. A typical output of glxinfo with
NVidia drivers would contain the following entries:

    OpenGL vendor string: NVIDIA Corporation
    OpenGL renderer string: GeForce GTX 960M/PCIe/SSE2
    OpenGL core profile version string: 4.6.0 NVIDIA 460.39
    OpenGL core profile shading language version string: 4.60 NVIDIA

To "unconfigure" the NVidia graphics card and then switch back to the
previous configuration, just run (as root):

	/usr/sbin/mageia-prime-uninstall

At the next reboot, the `glxinfo` command will tell you that are you are back to
the Mesa DRI accelerated drivers; a typical output of `glxinfo`, in this case,
should contain what follows:

    Vendor: Intel Open Source Technology Center (0x8086)
    Device: Mesa DRI Intel(R) HD Graphics 530 (Skylake GT2)  (0x191b)
    Version: 20.3.4
    Accelerated: yes
    Video memory: 3072MB

Note that mageia-prime can also work without any prepared `xorg.conf` file
(basically it will be used an automatic configuration). This particular configuration can be
achieved using the option '`-a`', e.g.:

    mageia-prime-install -a

This can be used for instance when none of the standard configurations would work.

## Options
There are some command line options to execute further tasks during
the installation (e.g. restart X11 automatically, etc.). Let's see them:

### Option '-a' (automatic xorg.conf)
Use this option to run Xorg with an empty `/etc/X11/xorg.conf` file. In
this way an automatic configuration would be used (with the priority
to the NVidia card).

### Option '-b' (do not blacklist nouveau)
This option avoid the blacklisting of the nouveau driver
and thus avoid to regenerate the initrd kernel images, e.g.:

    /usr/sbin/mageia-prime-install -b

and

    /usr/sbin/mageia-prime-uninstall -b

It's useful when used in conjunction with option `-g`.


### Option '-g' (add nouveau.modeset=0 to boot command line)
This option adds `nouveau.modeset=0` to the kernel booting command line. It works
only with grub2 (doesn't work with grub1, if you still have  it installed). E.g. use:

	/usr/sbin/mageia-prime-install -g

for configuring the NVidia prime and for adding `nouveau.modeset=0` to `/etc/default/grub`. And use:

	/usr/sbin/mageia-prime-uninstall -g

for removing such argument.

### Option '-p' (prime-offload)
Use this option for configuring the X11 server with the Intel
*integrated* graphics card, but to allow the *NVidia prime rendering offload*,
i.e. to use the NVidia GPU for doing the screen rendering. For this purpose
you have to set the environment variables `__NV_PRIME_RENDER_OFFLOAD=1` and `__GLX_VENDOR_LIBRARY_NAME=nvidia` before invoking the corresponding application.
As alternative you might use the provided wrapper script `mageia-prime-offload-run`; e.g.:

    mageia-prime-offload-run glmark2

    mageia-prime-offload-run <some cuda program>

This is similar to use *bumblebee*'s `optirun`.

### Option '-z' (zap):
To quickly restart the X11 server automatically after the card switching, you can invoke the `mageia-prime-install` and `mageia-prime-uninstall` commands with the option '`-z`' (*zap*). E.g.:

	/usr/sbin/mageia-prime-install -z

to configure NVidia Prime and then automatically restart Xorg; for unconfiguring (i.e. to switch back to the Intel drivers), just use:

	/usr/sbin/mageia-prime-uninstall -z

Note that mageia-prima-uninstall just switches back to the Intel graphics adapter, and won't take care of removing the installed proprietary NVidia RPM packages. The same commands can be run in with the '`-a`' option for an Xorg automatic configuration, e.g. to quickly switch to the NVidia card with the automatic configuration, use:

       /usr/sbin/mageia-prime-install -a -z

and then

       /usr/bin/mageia-prime-uninstall -a -z

to switch back to the Intel one.

### Option '-d' (dnf):
This option allows to use '`dnf`' instead of '`urpmi`' for installing the
required NVidia proprietary RPM package set. E.g.:

	/usr/sbin/mageia-prime-install -d


### Examples
With:

	/usr/sbin/mageia-prime-install -g -b

you can switch to the NVidia drivers and permanently disable the nouveau
driver at the next reboots. With:

	/usr/sbin/mageia-prime-uninstall -b

you can switch back to the Intel drivers, but without having to disable
nouveau.modeset again. So,for subsequents switches you can just use:

	/usr/sbin/mageia-prime-install -b -z

for switching to NVidia and

	/usr/sbin/mageia-prime-uninstall -b -z

for switching back to Intel. In this way, in conjuction with '-z', you can
quickly switch back and forth from NVidia to Intel drivers and viceversa,
without having to regenerate the grub configuration files or the initrd kernel
images.


## Troubleshooting

As already mentioned above, sometimes, it could happen that the
nouveau opensource graphics driver interferes with the NVidia
proprietary one, because its kernel module is automatically preloaded
when an NVidia graphics card is probed by the kernel. Sometimes this might happen
at the very early stage of kernel boot, long before the blacklisting commands of
`/etc/modules.d` can intervene. This might results in some crashes or causing the proprietary NVidia kernel modules to not being properly loaded. To avoid this situation, you may disable the
nouveau driver at kernel boot, using the following procedure:

* Edit (for grub2) the file `/etc/default/grub` and append `nouveau.modeset=0` to the GRUB_CMDLINE_LINUX_DEFAULT
* Regenerate the `grub.cfg` using `update-grub2`
* Regenerate the initrd boot image with `dracut -f`

All these steps are now automatically executed by the option '`-g`' at the mageia-prime installing and uninstalling stages.

Sometimes you can get wrong DPI with NVidia drivers. If this happens,
you can manually tune the DPI, uncommenting the lines:

    #Option "UseEdidDpi" "false"
    #Option "DPI" "96 x 96"

(or replace with the true "*dots per inch*" value of your display) in `/etc/X11/xorg.conf`.


## Notes

Once mageia-prime is installed, the NVidia card of your laptop will be accessed in a way similar any other desktop native NVidia graphic card. This means that you can use all the other NVidia proprietary utilities, such as, `nvidia-settings`, to control the NVidia card parameters or to change the thermal profiles between *Adaptive* and *Maximum Performance*, as well as checking the internal
temperatures. Also other utilities, like *The CUDA Toolkit*, or `cuda-z` would work.


## Benchmarking

Users can test the performances using the following benchmarking
utilities: `gxspheres64`, `glxgears` and `glmark2`, respectively contained in the
packages: `virtualgl`, `mesa-demos` and `glmark2`, all availables in Mageia Linux.

A typical usage is this:

* `$ glmark2`
* `$ vblank_mode=0 __GL_SYNC_TO_VBLANK=0 glxspheres64`
* `$ vblank_mode=0 __GL_SYNC_TO_VBLANK=0 glxgears`
* `$ glmark2-software --off-screen` (only under DRI + Mesa with Intel configured as graphics card).

Typical results for these benchmarks are:

* glmark2 with Intel HD Graphics 530: score 2457

* glmark2 with NVidia GTX 960M: score 6339

* glmark2-software with Gallium on LLVM pipe and Intel 6700HQ: score 740

* glxspheres64 with Intel HD Graphics 530: 290 Mpixels/s

* glxspheres64 with NVidia GTX 960M: 2392 MPixels/s


##### Author: Giuseppe Ghibò
##### Url: [http://www.github.com/ghibo/mageia-prime](http://www.github.com/ghibo/mageia-prime)
