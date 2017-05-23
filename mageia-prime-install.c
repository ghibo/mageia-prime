/*
 *
 * mageia-prime-install.c
 *
 * Copyright 2017 by Giuseppe Ghibò
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>

/* trivial filecopy */
int fcopy(char *name_src, char *name_dest)
{
	char buffer[BUFSIZ];
	FILE *fp_src, *fp_dest;
	size_t nbytes;

	if((fp_src = fopen(name_src, "r")) == NULL)
		return(1);

	if((fp_dest = fopen(name_dest, "w")) == NULL)
	{
		fclose(fp_src);
		return(1);
	}
	
	while ((nbytes = fread(buffer, 1, BUFSIZ, fp_src)) > 0)
	{
		fwrite(buffer, 1, nbytes, fp_dest);
	}
	
	fclose(fp_src);
	fclose(fp_dest);

	return(0);
}

int main(int argc, char **argv)
{
	FILE *fp;
	char buffer[BUFSIZ];
	int ret, clean = 0;
	uid_t uid;
	int i;
	int is_nouveau_loaded = 0;
	int do_not_blacklist_nouveau = 0;
	int is_xorg_free;
	int is_xorg_to_restore = 0;
	int have_to_zap = 0;
	long unsigned pcibus_intel = 0, pcidev_intel = 0, pcifunc_intel = 0;
	long unsigned pcibus_nvidia = 0, pcidev_nvidia = 0, pcifunc_nvidia = 0;

	/* scan arguments */
        for (i = 1; i < argc; i++)
        {
                if (*argv[i] == '-')
                {
                	char opt = argv[i][1];
                	
                	if (opt != '\0')
                	{
                		switch (opt)
                		{
                			case 'z': case 'Z':
                				if (argv[i][2] == '\0')
                				{
                					have_to_zap = 1;
                				}
                				break;
                			
                			case 'b': case 'B':
                				if (argv[i][2] == '\0')
                				{
                					do_not_blacklist_nouveau = 1;
                				}	
					default:
						break;
                		}
                	}
		}
	}

	if ((uid = getuid()) != 0) {
		fprintf(stderr, "Warning: you must run this command as root!\n\n");
		clean++;
	}

	/* TODO: it would be better to use libpciaccess instead of popen()/pclose(), more elegant */
	fp = popen("/bin/lspci | grep -E '3D|VGA'", "r");

	if (fp == NULL) {
		fprintf(stderr, "Can't run /bin/lspci\n");
		exit(1);
	}

	while (fgets(buffer, sizeof(buffer), fp) != NULL)
	{
		if (strcasestr(buffer, "intel")) {
			fprintf(stderr, "Found Intel card: %s", buffer);
			sscanf(buffer, "%02lx:%02lx.%lu", &pcibus_intel, &pcidev_intel, &pcifunc_intel);
			continue;
		}
		
		if (strcasestr(buffer, "nvidia")) {
			fprintf(stderr, "Found Nvidia card: %s", buffer);
			sscanf(buffer, "%02lx:%02lx.%lu", &pcibus_nvidia, &pcidev_nvidia, &pcifunc_nvidia);
			continue;
		}
	}

	pclose(fp);

	fp = popen("/sbin/lsmod | grep nouveau", "r");

	if (fp == NULL) {
		fprintf(stderr, "Can't run /sbin/lsmod: %s (error %d)\n", strerror(errno), errno);
		exit(1);
	}

	while (fgets(buffer, sizeof(buffer), fp) != NULL)
	{
		if (strcasestr(buffer, "nouveau")) {
			if ((strncmp("nouveau", buffer, 7)) == 0) {
				    fprintf(stderr, "Found nouveau kernel module loaded: %s", buffer);
				    is_nouveau_loaded = 1;
			}
		}
	}
	pclose(fp);

	if (is_nouveau_loaded)
	{
	    fprintf(stderr,"Unloading nouveau kernel module...");
	    if ((ret = system("/usr/sbin/rmmod nouveau")) != 0) {
		fprintf(stderr,"Warning: can't unload nouveau kernel module");
		clean++;
	    }
	    else {
		fprintf(stderr,"done.\n");
	    }
	    
	}
	
	if (!do_not_blacklist_nouveau)
	{
	    if ((fp = fopen("/etc/modprobe.d/00_mageia-prime.conf", "w")) == NULL)
	    {
	    	fprintf(stderr,"Warning: can't blacklist nouveau driver in /etc/modprobe.d/00_mageia_prime.conf\n");
	    	clean++;
	    }
	    else
	    {
	    	fprintf(fp, "blacklist nouveau\n");
	    	fprintf(stderr, "Blacklisting nouveau kernel module in /etc/modprobe.d/00_mageia_prime.conf\n");
	    	fprintf(stderr, "(shouldn't be enough, try toadd \'nouveau.modeset=0\' to your booting command line)\n");
	    	fclose(fp);
	    }
	}

	fprintf(stderr, "Ensuring the nonfree repo is enabled...");
	if ((ret = system("/usr/bin/dnf --quiet config-manager --set-enabled mageia-$(rpm -E %distro_arch)-nonfree")) != 0)
	{
		fprintf(stderr, "failed!\n");
		clean++;
	}
	else
	{
		fprintf(stderr, "ok.\n");
	}

	fprintf(stderr, "Checking package dkms-nvidia-current...");
	if ((ret = system("/bin/rpm --quiet -q dkms-nvidia-current")) != 0)
	{
		fprintf(stderr, "installing...");
		
		if ((ret = system("/usr/bin/dnf install dkms-nvidia-current")) != 0)
		{
			fprintf(stderr, "failed!\n");
			clean++;
		}
		else
		{
			fprintf(stderr, "ok.\n");
		}
	}
	else
	{
		fprintf(stderr, "already installed.\n");
	}

	fprintf(stderr, "Checking package nvidia-current-cuda-opencl...");
	if ((ret = system("/bin/rpm --quiet -q nvidia-current-cuda-opencl")) != 0)
	{
		fprintf(stderr, "installing...");
		
		if ((ret = system("/usr/bin/dnf install nvidia-current-cuda-opencl")) != 0)
		{
			fprintf(stderr, "failed!\n");
			clean++;
		}
		else
		{
			fprintf(stderr, "ok.\n");
		}
	}
	else
	{
		fprintf(stderr, "already installed.\n");
	}

	fprintf(stderr, "Checking package x11-driver-video-nvidia-current...");
	if ((ret = system("/bin/rpm --quiet -q x11-driver-video-nvidia-current")) != 0)
	{
		fprintf(stderr, "installing...");
		
		if ((ret = system("/usr/bin/dnf install x11-driver-video-nvidia-current")) != 0)
		{
			fprintf(stderr, "failed!\n");
			clean++;
		}
		else
		{
			fprintf(stderr, "ok.\n");
		}
	}
	else
	{
		fprintf(stderr, "already installed.\n");
	}
	
	if ((fp = fopen("/etc/X11/xorg.conf", "r")) == NULL)
	{
		fprintf(stderr,"You are running an X11 system without any xorg.conf.\n");
		is_xorg_free = 1;
	}
	else
	{
		is_xorg_free = 0;
		fclose(fp);
	}
	
	if ((fp = fopen("/etc/X11/xorg.conf.nvidiaprime.preserve", "r")) != NULL)
	{
		fprintf(stderr, "Found previous mageia-prime configuration.\n");
		is_xorg_to_restore = 1;
		fclose(fp);
	}
	else
		is_xorg_to_restore = 0;
	
	if ((fp = fopen("/etc/X11/xorg.conf.nvidiaprime", "r")) != NULL)
	{
		fprintf(stderr, "\nIt seems Mageia-Prime was already configured, manually delete the file /etc/X11/xorg.conf.nvidiaprime or unconfigure with mageia-prime-uninstall if you want to continue\n");
		fclose(fp);
		exit(1);
	}
	
	if (!is_xorg_to_restore)
	{
		if ((fp = fopen("/etc/X11/xorg.conf.nvidiaprime", "w")) == NULL)
		{
			fprintf(stderr, "Can't write to file /etc/X11/xorg.conf.nvidiaprime: %s (error %d)\n", strerror(errno), errno);
			exit(1);
		}

		fprintf(fp,"#\n"
	           	   "# automatically generated by mageia-prime-install\n"
	           	   "# for running NVidia Prime on Mageia GNU/Linux OS\n"
	           	   "#\n"
	           	   "Section \"ServerLayout\"\n"
	           	   "\tIdentifier \"layout\"\n"
	           	   "\tScreen 0 \"nvidia\"\n"
	           	   "\tInactive \"intel\"\n"
	           	   "EndSection\n\n"
	           	   "Section \"Device\"\n"
	           	   "\tIdentifier \"intel\"\n"
	           	   "\tDriver \"modesetting\"\n");
		fprintf(fp,"#\tBusID \"PCI:%lu:%lu:%lu\"\n", pcibus_intel, pcidev_intel, pcifunc_intel);
		fprintf(fp,"\tOption \"AccelMethod\" \"None\"\n"
		   	   "EndSection\n\n"
		   	   "Section \"Screen\"\n"
		   	   "\tIdentifier \"intel\"\n"
		   	   "\tDevice \"intel\"\n"
		   	   "EndSection\n\n"
		   	   "Section \"Device\"\n"
		   	   "\tIdentifier \"nvidia\"\n"
		   	   "\tDriver \"nvidia\"\n");
		fprintf(fp,"\tBusID \"PCI:%lu:%lu:%lu\"\n", pcibus_nvidia, pcidev_nvidia, pcifunc_nvidia);
		fprintf(fp,"EndSection\n\n"
		   	"Section \"Screen\"\n"
		   	"\tIdentifier \"nvidia\"\n"
		   	"\tDevice \"nvidia\"\n"
		   	"\tOption \"AllowEmptyInitialConfiguration\" \"on\"\n"
		   	"\t#Option \"UseDisplayDevice\" \"None\"\n"
		   	"\t#Option \"IgnoreDisplayDevices\" \"CRT\"\n"
		   	"\t#Option \"UseEDID\" \"off\"\n"
		   	"\t#Option \"UseEdidDpi\" \"false\"\n"
		   	"\t#Option \"DPI\" \"96 x 96\"\n"
		   	"\t#Option \"TripleBuffer\" \"true\"\n"
		   	"EndSection\n");
		fclose(fp);
	}
	else
	{
		fprintf(stderr, "Restoring previous mageia-prime configuration\n");
		if ((fcopy("/etc/X11/xorg.conf.nvidiaprime.preserve", "/etc/X11/xorg.conf.nvidiaprime")) != 0)
		{
			fprintf(stderr, "Can't copy /etc/X11/xorg.conf.nvidiaprime.preserve to /etc/X11/xorg.conf.nvidiaprime: %s (error: %d)\n", strerror(errno), errno);
			exit(1);
		}
	}

	/* mark system is running in an xorg.conf free configuration */
	if (is_xorg_free)
	{
		if ((fp = fopen("/etc/X11/xorg.conf.nvidiaprime.xorgfree", "w")) == NULL)
		{
			fprintf(stderr, "Can't write to file /etc/X11/xorg.conf.nvidiaprime.xorgfree: %s (error %d)\n", strerror(errno), errno);
			exit(1);
		}
	}
	else
	{
		if ((rename("/etc/X11/xorg.conf", "/etc/X11/xorg.conf.bak.beforenvidiaprime")) != 0)
		{
			fprintf(stderr, "Can't backup file /etc/X11/xorg.conf: %s (error %d)\n", strerror(errno), errno);
		}
	}

	if ((fcopy("/etc/X11/xorg.conf.nvidiaprime", "/etc/X11/xorg.conf")) != 0)
	{
		fprintf(stderr, "Can't copy /etc/X11/xorg.conf.nvidiaprime to /etc/X11/xorg.conf: %s (error %d)\n", strerror(errno), errno);
		exit(1);
	}
	
	if ((fp = fopen("/etc/X11/xsetup.d/000nvidiaprime.xsetup", "w")) == NULL)
	{
		fprintf(stderr, "Can't write to file /etc/X11/xsetup.d/000nvidia-prime.xsetup: %s (error %d)\n", strerror(errno), errno);
		exit(1);
	}
	fprintf(fp, "# to be sourced\n"
	            "# automatically generated by mageia-prime-install\n"
	            "/usr/bin/xrandr --setprovideroutputsource modesetting NVIDIA-0\n"
		    "/usr/bin/xrandr --auto\n");
	fclose(fp);

	/* 493 is chmod 755 */
	if ((chmod("/etc/X11/xsetup.d/000nvidiaprime.xsetup", 493)) < 0)
	{
		fprintf(stderr, "Can't chmod 755 file /etc/X11/xsetup.d/000nvidiaprime.xsetup: %s (error %d)\n", strerror(errno), errno);
		exit(1);
	}

	if ((fp = fopen("/etc/modules-load.d/nvidiaprime-drm.conf", "w")) == NULL)
	{
		fprintf(stderr, "Can't create file /etc/modules-load.d/nvidia-prime-drm.conf: %s (error %d)\n", strerror(errno), errno);
		exit(1);
	}
	fprintf(fp, "# automatically generated by mageia-prime-install\n");
	fprintf(fp, "nvidia-drm\n");
	fclose(fp);
	
	fprintf(stderr, "Switching to NVidia GL libraries...");
	if ((ret = system("/usr/sbin/update-alternatives --set gl_conf /etc/nvidia-current/ld.so.conf")) != 0)
	{
		fprintf(stderr, "Warning: failed to run update-alternatives --set gl_conf...\n");
		clean++;
	}
	else
	{
		fprintf(stderr, "ok.\n");
	}
	
	if ((ret = system("/usr/sbin/ldconfig")) != 0)
	{
		fprintf(stderr, "Warning: failed to run ldconfig\n");
		clean++;
	}

	if ((ret = system("/usr/sbin/modprobe nvidia")) != 0)
	{
		fprintf(stderr, "Warning: failed to modprobe nvidia kernel module\n");
		clean++;
	}
			
	if ((ret = system("/usr/sbin/modprobe nvidia-uvm")) != 0)
	{
		fprintf(stderr, "Warning: failed to modprobe nvidia-uvm kernel module\n");
		clean++;
	}
	
	if ((ret = system("/usr/sbin/modprobe nvidia-drm")) != 0)
	{
		fprintf(stderr, "Warning: failed to modprobe nvidia-drm kernel module\n");
		clean++;
	}

	if (clean > 0)
	{
		fprintf(stderr, "Mageia-Prime for NVidia configured (with %d warnings)\n", clean);
	}
	else
	{
		if (!is_xorg_to_restore)
		{
			fprintf(stderr,"Mageia-Prime for NVidia installed.\n");
		}
		else
			fprintf(stderr,"Mageia-Prime for NVidia reinstalled.\n");
		
	}

	if (!have_to_zap)
	{
		fprintf(stderr, "Please restart X11 or reboot the system.\n");
	}
	else /* zap X11 */
	{
		fprintf(stderr,"Zapping X11.\n");
		if ((ret = system("/bin/systemctl restart prefdm.service")) != 0)
		{
			fprintf(stderr, "Warning: Can't restart prefdm.service: %s (error %d)\n", strerror(errno), errno);
		}
	}

	return(0);
}
