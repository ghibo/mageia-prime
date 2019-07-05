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

int nouveau_nomodeset_already_installed = 0;
int grub_add_nouveau_nomodeset()
{
	FILE *fp, *fpnew;
	char buffer[BUFSIZ];
	char bufnew[BUFSIZ+128];
	const char *nouveau_modeset_zero = "nouveau.modeset=0";
	const char *grub_cmdline_linux_default = "GRUB_CMDLINE_LINUX_DEFAULT=\"";
	char *pos;
	size_t l, l1;
	int ret;
	extern int nouveau_nomodeset_already_installed;
	
	l = strlen(grub_cmdline_linux_default);
	l1 = strlen(nouveau_modeset_zero);
	
	/* work on a copy of /etc/default/grub */
	if ((fcopy("/etc/default/grub", "/etc/default/grub.bak.nvidiaprime")) != 0)
	{
		fprintf(stderr, "Can't copy /etc/default/grub to /etc/default/grub.temp.nvidiaprime: %s (error: %d)\n", strerror(errno), errno);
		return(1);
	}
	
	if ((fp = fopen("/etc/default/grub.bak.nvidiaprime", "r")) == NULL)
	{
		fprintf(stderr, "Can't open file /etc/default/grub.bak.nvidiaprime: %s (error %d)\n", strerror(errno), errno);
		return(1);
	}
	
	if ((fpnew = fopen("/etc/default/grub.temp.nvidiaprime", "w")) == NULL)
	{
		fprintf(stderr, "Can't open file /etc/default/grub.temp.nvidiaprime");
		fclose(fp);
		return(1);
	}
	
	while (fgets(buffer, sizeof(buffer), fp) != NULL)
	{
		*(bufnew) = '\0';
		if ((pos = strstr(buffer, grub_cmdline_linux_default)))
		{
			if (strstr(pos + l, nouveau_modeset_zero))
			{
				if ((fputs(buffer, fpnew)) == EOF)
				{
					fprintf(stderr, "Can't write on file: /etc/default/grub.temp.nvidiaprime: %s (error %d)\n", strerror(errno), errno);
					break;
				} 
				
				/* already contains nouveau.modeset=0, skipping */
				nouveau_nomodeset_already_installed = 1;
				fprintf(stderr, "/etc/default/grub already contains nouveau.modeset=0 option, skipping...\n");
			}
			else /* add nouveau.modeset=0 */
			{
				nouveau_nomodeset_already_installed = 0;
				
				if (l <= BUFSIZ)
				{
					strncpy(bufnew, buffer, l);
					*(bufnew + l) = '\0';
				}
				
				if (*(buffer+l) == ' ')
				{
					strncat(bufnew, " ", 1);
					strncat(bufnew, nouveau_modeset_zero, l1);
					strncat(bufnew, buffer + l, strlen(buffer + l));
				}
				else
				{
					strncat(bufnew, nouveau_modeset_zero, l1);
					strncat(bufnew, " ", 1);
					strncat(bufnew, buffer + l, strlen(buffer + l));
				}
				
			}
			if ((fputs(bufnew, fpnew)) == EOF)
			{
				fprintf(stderr, "Can't write on file: /etc/default/grub.temp.nvidiaprime: %s (error %d)\n", strerror(errno), errno);
				break;
			}
		}
		else
		{
			if ((fputs(buffer, fpnew)) == EOF)
			{
				fprintf(stderr, "Can't write on file: /etc/default/grub.temp.nvidiaprime: %s (error %d)\n", strerror(errno), errno);
				break;
			}
		}
	}

	fclose(fp);
	fclose(fpnew);

	if (!nouveau_nomodeset_already_installed)
	{
		if ((rename("/etc/default/grub.temp.nvidiaprime", "/etc/default/grub")) != 0)
		{
			fprintf(stderr, "Can't rename /etc/default/grub.temp.nvidiaprime to /etc/default/grub: %s (error %d)\n", strerror(errno), errno);
			return(1);
		}
	}
	else
	{
		/* discard modified file */
		if ((ret = remove("/etc/default/grub.temp.nvidiaprime")) != 0)
		{
			fprintf(stderr, "Warning: Can't remove file /etc/default/grub.temp.nvidiaprime: %s (%d)\n", strerror(errno), errno);
			return(1);
		}	
	}

	return(0);
}

int grub_remove_nouveau_nomodeset()
{
	FILE *fp, *fpnew;
	char buffer[BUFSIZ];
	char bufnew[BUFSIZ+128];
	const char *nouveau_modeset_zero = "nouveau.modeset=0 ";
	const char *nouveau_modeset_zero_notrail = "nouveau.modeset=0";
	const char *grub_cmdline_linux_default = "GRUB_CMDLINE_LINUX_DEFAULT=\"";
	char *pos, *match;
	size_t l, l1, l2;
	
	l = strlen(grub_cmdline_linux_default);
	l1 = strlen(nouveau_modeset_zero);
	l2 = strlen(nouveau_modeset_zero_notrail);
	
	/* work on a copy of /etc/default/grub */
	if ((fcopy("/etc/default/grub", "/etc/default/grub.bak.nvidiaprime.removed")) != 0)
	{
		fprintf(stderr, "Can't copy /etc/default/grub to /etc/default/grub.temp.nvidiaprime.removed: %s (error: %d)\n", strerror(errno), errno);
		return(1);
	}
	
	if ((fp = fopen("/etc/default/grub.bak.nvidiaprime.removed", "r")) == NULL)
	{
		fprintf(stderr, "Can't open file /etc/default/grub.bak.nvidiaprime.removed: %s (error %d)\n", strerror(errno), errno);
		return(1);
	}
	
	if ((fpnew = fopen("/etc/default/grub.temp.nvidiaprime", "w")) == NULL)
	{
		fprintf(stderr, "Can't open file /etc/default/grub.temp.nvidiaprime");
		fclose(fp);
		return(1);
	}
	
	while (fgets(buffer, sizeof(buffer), fp) != NULL)
	{
		*(bufnew) = '\0';
		if ((pos = strstr(buffer, grub_cmdline_linux_default)))
		{
			strncpy(bufnew, buffer, l);
			*(bufnew + l) = '\0';

			if ((match = strstr(pos + l, nouveau_modeset_zero)))
			{
				strncat(bufnew, pos + l, match - (pos + l)); /* skip nouveau.modeset=0 */
				strncat(bufnew, match + l1, strlen(match + l1));
				
			}
			else if ((match = strstr(pos + l, nouveau_modeset_zero_notrail)))
			{
				strncat(bufnew, pos + l, match - (pos + l)); /* skip nouveau.modeset=0 */
				strncat(bufnew, match + l2, strlen(match + l2));
			}
			else /* copy rest of the string */
			{
				strncat(bufnew, pos + l, strlen(pos + l));
				
			}
			
			if ((fputs(bufnew, fpnew)) == EOF)
			{
				fprintf(stderr, "Can't write on file: /etc/default/grub.temp.nvidiaprime: %s (error %d)\n", strerror(errno), errno);
				break;
			}
		}
		else /* copy buffer as is */
		{
			if ((fputs(buffer, fpnew)) == EOF)
			{
				fprintf(stderr, "Can't write on file: /etc/default/grub.temp.nvidiaprime: %s (error %d)\n", strerror(errno), errno);
				break;
			}
		}
	}

	fclose(fp);
	fclose(fpnew);

	if ((rename("/etc/default/grub.temp.nvidiaprime", "/etc/default/grub")) != 0)
	{
		fprintf(stderr, "Can't rename /etc/default/grub.temp.nvidiaprime to /etc/default/grub: %s (error %d)\n", strerror(errno), errno);
		return(1);
	}
	
	return(0);
}

void choose_nvidia_version(char *result)
{
	FILE *fp;
	char buffer[BUFSIZ];

	result[0] = '\0';

	fp = popen("/bin/lspcidrake -v | grep 'Card:NVIDIA'", "r");

	if (fp == NULL) {
		fprintf(stderr, "Can't run /bin/lspcidrake\n");
		return;
	}

	while (fgets(buffer, sizeof(buffer), fp) != NULL)
	{
		if (strstr(buffer, "NVIDIA GeForce 420 to GeForce 630")) {
			strcpy(result, "390");
			break;
		}

		if (strstr(buffer, "NVIDIA GeForce 635 series and later")) {
			strcpy(result, "-current");
			break;
		}
	}

	if (result[0] != '\0')
	{
		fprintf(stderr, "Driver that have to be used: nvidia%s", result);
	}

	pclose(fp);
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
	int use_dnf = 0;
	int have_to_zap = 0;
	int touch_grub = 0;
	extern int nouveau_nomodeset_already_installed;
	
	long unsigned pcibus_intel = 0, pcidev_intel = 0, pcifunc_intel = 0;
	long unsigned pcibus_nvidia = 0, pcidev_nvidia = 0, pcifunc_nvidia = 0;

	char nv_driver_ver[9];
	char command[BUFSIZ];

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
                			case 'b': case 'B':
                				if (argv[i][2] == '\0')
                				{
                					do_not_blacklist_nouveau = 1;
                				}
                				break;
                				
                			case 'd': case 'D':
                				if (argv[i][2] == '\0')
                				{
                					use_dnf = 1;
                				}
                				break;
					
					case 'g': case 'G':
						if (argv[i][2] == '\0')
						{
							touch_grub = 1;
						}
						break;

                			case 'z': case 'Z':
                				if (argv[i][2] == '\0')
                				{
                					have_to_zap = 1;
                				}
                				break;

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
	    	fprintf(stderr,"Warning: can't blacklist nouveau driver in /etc/modprobe.d/00_mageia-prime.conf\n");
	    	clean++;
	    }
	    else
	    {
	    	fprintf(fp, "blacklist nouveau\n");
	    	fprintf(stderr, "Blacklisting nouveau kernel module in /etc/modprobe.d/00_mageia-prime.conf\n");
	    	fprintf(stderr, "(shouldn't be enough, try toadd \'nouveau.modeset=0\', e.g. with option -g here, to your booting command line)\n");
	    	fclose(fp);
	    	
	    	fprintf(stderr, "Regenerating kernel initrd images...");
	    	if ((ret = system("/sbin/bootloader-config --action rebuild-initrds")) != 0)
	    	{
	    		fprintf(stderr, "failed!\n");
	    		clean++;
	    	}
	    	else
	    	{
	    		fprintf(stderr, "done.\n");
		}
	    }
	}

	if (use_dnf)
	{
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
	}

	choose_nvidia_version(nv_driver_ver);

	if (nv_driver_ver[0] == '\0')
	{
		exit(1);
	}

	fprintf(stderr, "Checking package dkms-nvidia%s...", nv_driver_ver);
	sprintf(command, "/bin/rpm --quiet -q dkms-nvidia%s", nv_driver_ver);
	if ((ret = system(command)) != 0)
	{
		fprintf(stderr, "installing...");
		
		if (use_dnf)
		{
			sprintf(command, "/usr/bin/dnf install dkms-nvidia%s", nv_driver_ver);
			if ((ret = system(command)) != 0)
			{
				fprintf(stderr, "failed!\n");
				clean++;
			}
			else
			{
				fprintf(stderr, "ok.\n");
			}
		}
		else /* urpmi */
		{
			sprintf(command, "/usr/sbin/urpmi dkms-nvidia%s", nv_driver_ver);
			if ((ret = system(command)) != 0)
			{
				fprintf(stderr, "failed!\n");
				clean++;
			}
			else
			{
				fprintf(stderr, "ok.\n");
			}
		
		}
	}
	else
	{
		fprintf(stderr, "already installed.\n");
	}

	fprintf(stderr, "Checking package nvidia%s-cuda-opencl...", nv_driver_ver);
	sprintf(command, "/bin/rpm --quiet -q nvidia%s-cuda-opencl", nv_driver_ver);
	if ((ret = system(command)) != 0)
	{
		fprintf(stderr, "installing...");
		
		if (use_dnf)
		{
			sprintf(command, "/usr/bin/dnf install nvidia%s-cuda-opencl", nv_driver_ver);
			if ((ret = system(command)) != 0)
			{
				fprintf(stderr, "failed!\n");
				clean++;
			}
			else
			{
				fprintf(stderr, "ok.\n");
			}
		}
		else /* urpmi */
		{
			sprintf(command, "/usr/sbin/urpmi nvidia%s-cuda-opencl", nv_driver_ver);
			if ((ret = system(command)) != 0)
			{
				fprintf(stderr, "failed!\n");
				clean++;
			}
			else
			{
				fprintf(stderr, "ok.\n");
			}
		}
	}
	else
	{
		fprintf(stderr, "already installed.\n");
	}

	fprintf(stderr, "Checking package x11-driver-video-nvidia%s...", nv_driver_ver);
	sprintf(command, "/bin/rpm --quiet -q x11-driver-video-nvidia%s", nv_driver_ver);
	if ((ret = system(command)) != 0)
	{
		fprintf(stderr, "installing...");
		
		if (use_dnf)
		{
			sprintf(command, "/usr/bin/dnf install x11-driver-video-nvidia%s", nv_driver_ver);
			if ((ret = system(command)) != 0)
			{
				fprintf(stderr, "failed!\n");
				clean++;
			}
			else
			{
				fprintf(stderr, "ok.\n");
			}
		}
		else /* urpmi */
		{
			sprintf(command, "/usr/sbin/urpmi x11-driver-video-nvidia%s", nv_driver_ver);
			if ((ret = system(command)) != 0)
			{
				fprintf(stderr, "failed!\n");
				clean++;
			}
			else
			{
				fprintf(stderr, "ok.\n");
			}
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
	
	if ((fp = fopen("/etc/X11/xsetup.d/000mageia-prime.xsetup", "w")) == NULL)
	{
		fprintf(stderr, "Can't write to file /etc/X11/xsetup.d/000mageia-prime.xsetup: %s (error %d)\n", strerror(errno), errno);
		exit(1);
	}
	fprintf(fp, "# to be sourced\n"
	            "# automatically generated by mageia-prime-install\n"
	            "/usr/bin/xrandr --setprovideroutputsource modesetting NVIDIA-0\n"
		    "/usr/bin/xrandr --auto\n");
	fclose(fp);

	/* 493 is chmod 755 */
	if ((chmod("/etc/X11/xsetup.d/000mageia-prime.xsetup", 493)) < 0)
	{
		fprintf(stderr, "Can't chmod 755 file /etc/X11/xsetup.d/000mageia-prime.xsetup: %s (error %d)\n", strerror(errno), errno);
		exit(1);
	}

	if ((fp = fopen("/etc/modules-load.d/nvidia-prime-drm.conf", "w")) == NULL)
	{
		fprintf(stderr, "Can't create file /etc/modules-load.d/nvidia-prime-drm.conf: %s (error %d)\n", strerror(errno), errno);
		exit(1);
	}
	fprintf(fp, "# automatically generated by mageia-prime-install\n");
	fprintf(fp, "nvidia-drm\n");
	fclose(fp);
	
	fprintf(stderr, "Switching to NVidia GL libraries...");
	sprintf(command, "/usr/sbin/update-alternatives --set gl_conf /etc/nvidia%s/ld.so.conf", nv_driver_ver);
	if ((ret = system(command)) != 0)
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

	if (touch_grub)
	{
		fprintf(stderr, "Adding \"nouveau.modeset=0\" to grub2...");
 		if (grub_add_nouveau_nomodeset() != 0)
 		{
 			fprintf(stderr, "failed!\n");
 			clean++;
		}
		else
			fprintf(stderr, "done.\n");
		
		
		if (!nouveau_nomodeset_already_installed)
		{
			fprintf(stderr, "Updating grub.cfg...");
			if ((ret = system("/usr/bin/update-grub")) != 0)
			{
				fprintf(stderr, "failed!\n");
				clean++;
			}
			else
			{
				fprintf(stderr, "done.\n");
			}
		}
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
