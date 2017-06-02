# Mageia-Prime

This tool allows you to easily configure NVidia Prime for using the
discrete graphics card with the NVidia proprietary drivers within the
Mageia GNU/Linux distribution.

Most of nowadays laptops have a graphic subsystem called
"Hybrid-Graphics", i.e they come with two graphics cards: one is
usually "internal" (or "integrated") into the CPU, and is called IGP
(Integrated Graphic Processor), and the other is "dedicated", external
to the CPU, and it is called "discrete" (DGP, Discrete/Dedicated
Graphics Processor).

So a typical laptop configuration could have two cards: an internal Intel
graphics card (e.g.  Intel HD Graphics 530), and a discrete NVidia
graphics card (e.g.  NVidia GeForce GTX 960M), even though other
configurations could be also possible.

The default X11 installation is typically using the internal graphics
card, which is usually slower (though it consumes less energy power) than
the discrete one. Other ways to access to the discrete graphics card with
the proprietary drivers includes to "mix" the output of both (integrated
and discrete) cards into a single one: this is the case of
using "bumblebee" under Linux. Unfortunately for NVidia discrete cards this
will result, sometimes, in getting slower performance than the
internal one.


## Usage:

To configure the NVidia graphics card with Prime, run (as root) the
following command:

	/usr/sbin/mageia-prime-install

this will switch the default configuration in /etc/X11/xorg.conf to
one suitable for NVidia prime. The Mageia RPM packages for NVidia
proprietary drivers should be already installed before issuing this
command, otherwise mageia-prime-install will try to install them
for you. After running such a command you need to restart the X11
server or alternatively reboot the machine. After that, the switch to
the discrete card should be completed. At the next reboot or restart of the
X11 server, you can check that the GL libraries are really coming from
NVidia drivers. The utility "glxinfo" can be used for this purpose. A typical
output of glxinfo with NVidia would contain in this case:

	OpenGL vendor string: NVIDIA Corporation
	OpenGL renderer string: GeForce GTX 960M/PCIe/SSE2
	OpenGL core profile version string: 4.5.0 NVIDIA 375.66
	OpenGL core profile shading language version string: 4.50 NVIDIA

To unconfigure the Nvidia Prime and then switch back to the previous
configuration, just run (as root):

	/usr/sbin/mageia-prime-uninstall

As before, after the next reboot, the glxinfo will show you are back to
the Mesa DRI accelerated drivers; a typical output of glxinfo, in this case,
should contain what follows:

    Vendor: Intel Open Source Technology Center (0x8086)
    Device: Mesa DRI Intel(R) HD Graphics 530 (Skylake GT2)  (0x191b)
    Version: 17.0.5
    Accelerated: yes
    Video memory: 3072MB


## Options
There is a set of command line arguments to add several tasks during
the installation (e.g. restart X11 automatically, etc.).


### Option '-b' (do not blacklist nouveau)
This option avoid the blacklisting of the nouveau driver
and thus avoid to regenerate the initrd kernel images, e.g.:

	/usr/sbin/mageia-prime-install -b

and

	/usr/sbin/mageia-prime-uninstall -b

It's useful when used in conjunction with option -g.


### Option '-g' (add nouveau.modeset=0 to boot command line)
This option adds nouveau.modeset=0 to the kernel booting command line. Works
only with grub2 (doesn't work with grub1). E.g. for installing, use:

	/usr/sbin/mageia-prime-install -g

and

	/usr/sbin/mageia-prime-uninstall -g

for uninstalling.


### Option '-z' (zap):
To quickly restart X11 automatically you can invoke the mageia-prime-install/mageia-prime-uninstall
with the option '-z' (zap). E.g.

	/usr/sbin/mageia-prime-install -z

or

	/usr/sbin/mageia-prime-uninstall -z


### Option '-d' (dnf):
This option allows to use 'dnf' instead of 'urpmi' for installing the nvidia RPM package set.
E.g.

	/usr/sbin/mageia-prime-install -d


### Examples
With

	/usr/sbin/mageia-prime-install -g -b

you can switch to the NVidia drivers and permanently disable the nouveau
driver at the next reboots. With:

	/usr/sbin/mageia-prime-uninstall -b

you can switch back to the Intel drivers, but without having to disable
nouveau.modeset again. Then for subsequents switch you can use just:

	/usr/sbin/mageia-prime-install -b -z

for switching to NVidia and

	/usr/sbin/mageia-prime-uninstall -b -z

for switching back to Intel. In this way, in conjuction with '-z', you can
quickly switch in/out/in/out from Nvidia to Intel drivers and viceversa,
without having to regenerate the grub configuration or the initrd kernel
images.


## Troubleshooting

As already mentioned above, sometimes, it could happen that the nouveau
opensource graphics driver interferes with the NVidia proprietary one,
because its kernel module is automatically preloaded when an NVidia graphics card is
probed by the kernel. This might results in some crashes or causing the
proprietary NVidia kernel module to not loading properly. To avoid
this, you may disable the nouveau driver, by appending
nouveau.modeset=0 to the booting command line, e.g. for grub2, editing
the file /etc/default/grub and appending nouveau.modeset=0 to the
GRUB_CMDLINE_LINUX_DEFAULT, and then regenerating grub.cfg (using
grub2-mkconfig) as well as regenerating the initrd boot image with
dracut. This operation is now automatically executed by the option '-g' of
the installing and uninstalling stages.

Sometimes you can get wrong DPI with NVidia drivers. If this happens,
you can tune manually the DPI, uncommenting the lines:

  #Option "UseEdidDpi" "false"
  #Option "DPI" "96 x 96"

(or your favourite DPI) in /etc/X11/xorg.conf.


## Notes

Once Mageia-Prime is installed, the NVidia card will be accessed as
any other desktop native NVidia graphic card. This means
that you can use all the other NVidia proprietary utilies, such as,
"nvidia-settings", to control the internal card parameters or change the
thermal profiles between Adaptive and Maximum Performance, as well as
checking the internal temperature. Also other utilities, like "The CUDA
Toolkit", or "CUDA-Z" would work. Note that the performance boost
of the NVidia card will be achieved at expense of the battery, which
will probably last shorter, according to the same initial
amount of charge (no problems if you are attached to the PSU).



## Benchmarking

Users can test the performance using the following benchmarking
utilities: glxspheres64 and glmark2, respectively contained in the
packages: virtualgl and glmark2 and both available for Mageia Linux.

A typical usage is this:

* glmark2
* vblank_mode=0 glxsphere64
* glmark2-software --off-screen (only under DRI + Mesa with Intel
configured as graphics card).

While, typical benchmarking values are:

* glmark2 with Intel HD Graphics 530: score 2457

* glmark2 with NVidia GTX 960M: score 6120

* glmark2-software with Gallium on LLVM pipe and Intel 6700HQ: score 740

* glxsphere64 with Intel HD Graphics 530: 290 Mpixels/s

* glxsphere64 with NVidia GTX 960M: 2200 MPixels/s


##### Author: Giuseppe Ghibò
##### Url: [http://www.github.com/ghibo/mageia-prime](http://www.github.com/ghibo/mageia-prime)
