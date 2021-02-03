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
	int use_xorg_auto = 0; /* set to 1 for an empty/automatic xorg.conf file */
	int use_xorg_prime_offload = 0; /* set to 1 to use prime rendering offloading */
	int use_nvidia_current = 1;
	int use_nvidia_390 = 0;
	int force_nvidia_driver_installing = 0; /* auto-detect which package driver set (nvidia-current or nvidia390) to use */
	int force_xorg_to_overlap = 0; /* force a fresh xorg.conf, don't restore the preserved one */
	int use_intel_driver = 0;
	extern int nouveau_nomodeset_already_installed;
	
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
                			case '3':
                				if (argv[i][2] == '\0')
                				{
                					use_nvidia_390 = 1;
                					use_nvidia_current = 0;
							force_nvidia_driver_installing = 1;
						}
						break;

                			case 'a': case 'A':
                				if (argv[i][2] == '\0')
						{
                					use_xorg_auto = 1;
                					use_xorg_prime_offload = 0;
						}
						break;

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
					
					case 'f': case 'F':
						if (argv[i][2] == '\0')
						{
							force_xorg_to_overlap = 1;
						}
						break;

					case 'g': case 'G':
						if (argv[i][2] == '\0')
						{
							touch_grub = 1;
						}
						break;
					
					case 'i': case 'I':
						if (argv[i][2] == '\0')
						{
							use_intel_driver = 1;
						}
						break;

					case 'k': case 'K':
						if (argv[i][2] == '\0')
						{
							use_nvidia_current = 1;
							use_nvidia_390 = 0;
							force_nvidia_driver_installing = 1;
						}
						break;

					case 'p': case 'P':
						if (argv[i][2] == '\0' )
						{
							use_xorg_prime_offload = 1;
							use_xorg_auto = 0;
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
		fprintf(stderr, "Error: this program requires root privileges to be run!\n");
		exit(1);
	}

	/* TODO: use libpciaccess instead of popen()/pclose() */
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
	    	fprintf(fp, "options nouveau modeset=0\n");
	    	fprintf(fp, "alias nouveau off\n");
	    	fprintf(fp, "options nvidia-drm modeset=1\n");
	    	fprintf(fp, "options nvidia NVreg_DynamicPowerManagement=0x02\n");
	    	fprintf(stderr, "Blacklisted nouveau kernel module in /etc/modprobe.d/00_mageia-prime.conf\n");
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
	
	/* Auto-detect using of nvidia-current or nvidia390 */
	if (!force_nvidia_driver_installing)
	{
		fp = popen("/bin/lspcidrake -v | grep 'Card:NVIDIA'", "r");
		if (fp == NULL) {
			fprintf(stderr, "Can't run /bin/lspcidrake\n");
			exit(1);
		}
		while (fgets(buffer, sizeof(buffer), fp) != NULL)
		{
			if (strcasestr(buffer, "NVIDIA GeForce 635 series and later"))
			{
				fprintf(stderr, "%s", buffer);
				fprintf(stderr, "Card is supported by the nvidia-current driver set.\n");
				use_nvidia_current = 1;
				use_nvidia_390 = 0;
				break;
			}

			if (strcasestr(buffer, "NVIDIA GeForce 420 to GeForce 630"))
			{
				fprintf(stderr, "%s", buffer);
				fprintf(stderr, "Card is supported by the nvidia390 driver set.\n");
				use_nvidia_390 = 1;
				use_nvidia_current = 0;
				break;
			}
		}
		pclose(fp);
	}

	if (use_nvidia_current && !use_nvidia_390)
	{
		fprintf(stderr, "Checking package dkms-nvidia-current...");
		if ((ret = system("/bin/rpm --quiet -q dkms-nvidia-current")) != 0)
		{
			fprintf(stderr, "installing...");

			if (use_dnf)
			{
				if ((ret = system("/usr/bin/dnf install -y dkms-nvidia-current")) != 0)
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
				if ((ret = system("/usr/sbin/urpmi --auto dkms-nvidia-current")) != 0)
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
	}
	else if (use_nvidia_390 && !use_nvidia_current)
	{
		fprintf(stderr, "Checking package dkms-nvidia390...");
		if ((ret = system("/bin/rpm --quiet -q dkms-nvidia390")) != 0)
		{
			fprintf(stderr, "installing...");

			if (use_dnf)
			{
				if ((ret = system("/usr/bin/dnf install -y dkms-nvidia390")) != 0)
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
				if ((ret = system("/usr/sbin/urpmi --auto dkms-nvidia390")) != 0)
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
	}

	if (use_nvidia_current && !use_nvidia_390)
	{
		fprintf(stderr, "Checking package nvidia-current-cuda-opencl...");
		if ((ret = system("/bin/rpm --quiet -q nvidia-current-cuda-opencl")) != 0)
		{
			fprintf(stderr, "installing...");
		
			if (use_dnf)
			{
				if ((ret = system("/usr/bin/dnf install -y nvidia-current-cuda-opencl")) != 0)
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
				if ((ret = system("/usr/sbin/urpmi --auto nvidia-current-cuda-opencl")) != 0)
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
	}
	else if (use_nvidia_390 && !use_nvidia_current)
	{
		fprintf(stderr, "Checking package nvidia390-cuda-opencl...");
		if ((ret = system("/bin/rpm --quiet -q nvidia390-cuda-opencl")) != 0)
		{
			fprintf(stderr, "installing...");

			if (use_dnf)
			{
				if ((ret = system("/usr/bin/dnf install -y nvidia-current-cuda-opencl")) != 0)
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
				if ((ret = system("/usr/sbin/urpmi --auto nvidia390-cuda-opencl")) != 0)
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
	}

	if (use_nvidia_current && !use_nvidia_390)
	{
		fprintf(stderr, "Checking package x11-driver-video-nvidia-current...");
		if ((ret = system("/bin/rpm --quiet -q x11-driver-video-nvidia-current")) != 0)
		{
			fprintf(stderr, "installing...");

			if (use_dnf)
			{
				if ((ret = system("/usr/bin/dnf install -y x11-driver-video-nvidia-current")) != 0)
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
				if ((ret = system("/usr/sbin/urpmi --auto x11-driver-video-nvidia-current")) != 0)
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
	}
	else if (use_nvidia_390 && !use_nvidia_current)
	{
		fprintf(stderr, "Checking package x11-driver-video-nvidia390...");
		if ((ret = system("/bin/rpm --quiet -q x11-driver-video-nvidia390")) != 0)
		{
			fprintf(stderr, "installing...");

			if (use_dnf)
			{
				if ((ret = system("/usr/bin/dnf install -y x11-driver-video-nvidia390")) != 0)
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
				if ((ret = system("/usr/sbin/urpmi --auto x11-driver-video-nvidia390")) != 0)
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
		fprintf(stderr, "Found a previous mageia-prime configuration. Re-using.\n");
		is_xorg_to_restore = 1;
		fclose(fp);
	}
	else
		is_xorg_to_restore = 0;

	if (force_xorg_to_overlap)
		is_xorg_to_restore = 0; /* force a fresh configuration in any case, don't restore the preserved one */
	
	if ((fp = fopen("/etc/X11/xorg.conf.nvidiaprime", "r")) != NULL)
	{
		fprintf(stderr, "\nIt seems mageia-prime was already configured, please unconfigure running mageia-prime-uninstall or manually delete the file /etc/X11/xorg.conf.nvidiaprime\n");
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
	           	   "#\n");
		if ((!use_xorg_auto) && (!use_xorg_prime_offload))
		{
			fprintf(fp,"Section \"ServerLayout\"\n"
	           	   "\tIdentifier \"layout\"\n"
	           	   "\tScreen 0 \"nvidia\"\n"
	           	   "\tInactive \"intel\"\n"
                           "\tOption \"AllowNVIDIAGPUScreens\" \"true\"\n"
                           "\tOption \"AllowExternalGpus\" \"true\"\n"
                           "\tInputDevice \"MyKeyboard\" \"CoreKeyboard\"\n"
	           	   "EndSection\n\n");
	           	fprintf(fp,"Section \"ServerFlags\"\n"
	           	   "\tOption \"AllowMouseOpenFail\" \"true\"\n"
	           	   "EndSection\n\n"
	           	   "Section \"InputDevice\"\n"
	           	   "\tIdentifier \"MyKeyboard\"\n"
			   "\tDriver \"kbd\"\n"
			   "EndSection\n\n"
			   "Section \"Monitor\"\n"
			   "\tIdentifier \"MyMonitor\"\n"
			   "\tVendorname \"Unknown\"\n"
		           "\tModelName  \"Unknown\"\n"
			   "\tOption     \"DPMS\"\n"
			   "EndSection\n\n");
		   	fprintf(fp,"Section \"Device\"\n"
		   	   "\tIdentifier \"nvidia\"\n"
		   	   "\tDriver \"nvidia\"\n");
		   	fprintf(fp,"\tBusID \"PCI:%lu:%lu:%lu\"\n", pcibus_nvidia, pcidev_nvidia, pcifunc_nvidia);
		   	fprintf(fp,"EndSection\n\n"
		   	   "Section \"Screen\"\n"
		   	   "\tIdentifier \"nvidia\"\n"
		   	   "\tDevice \"nvidia\"\n"
		   	   "\tMonitor \"MyMonitor\"\n"
		   	   "\tOption \"AllowEmptyInitialConfiguration\" \"true\"\n"
		   	   "\t#Option \"UseDisplayDevice\" \"None\"\n"
		   	   "\t#Option \"IgnoreDisplayDevices\" \"CRT\"\n"
		   	   "\t#Option \"UseEDID\" \"off\"\n"
		   	   "\t#Option \"UseEdidDpi\" \"false\"\n"
		   	   "\t#Option \"DPI\" \"96 x 96\"\n"
		   	   "\t#Option \"DPI\" \"192 x 192\"\n"
		   	   "\t#Option \"DPI\" \"282 x 282\"\n"
		   	   "\t#Option \"TripleBuffer\" \"true\"\n"
		   	   "EndSection\n\n");
	           	fprintf(fp,"Section \"Device\"\n"
	           	   "\tIdentifier \"intel\"\n");
	           	if (!use_intel_driver)
	           	{
		           	fprintf(fp,"\tDriver \"modesetting\"\n");
				fprintf(fp,"\t#Option \"AccelMethod\" \"None\"\n");
			}
			else
			{
				fprintf(fp,"\tDriver \"intel\"\n");
				fprintf(fp,"\tOption \"DRI\" \"3\"\n");
				fprintf(fp,"\tOption \"Tearfree\" \"false\"\n");
			}
			fprintf(fp,"\tBusID \"PCI:%lu:%lu:%lu\"\n", pcibus_intel, pcidev_intel, pcifunc_intel);
		   	fprintf(fp,"EndSection\n\n"
		   	   "Section \"Screen\"\n"
		   	   "\tIdentifier \"intel\"\n"
		   	   "\tDevice \"intel\"\n"
		   	   "EndSection\n");
		}
		else if (use_xorg_prime_offload)
		{
			fprintf(fp,"Section \"ServerLayout\"\n"
	           	   "\tIdentifier \"layout\"\n"
	           	   "\tScreen 0 \"intel\"\n"
	           	   "\tInactive \"nvidia\"\n"
                           "\tOption \"AllowNVIDIAGPUScreens\" \"true\"\n"
                           "\tOption \"AllowExternalGpus\" \"true\"\n"
                           "\tInputDevice \"MyKeyboard\" \"CoreKeyboard\"\n"
	           	   "EndSection\n\n");
	           	fprintf(fp,"Section \"ServerFlags\"\n"
	           	   "\tOption \"AllowMouseOpenFail\" \"true\"\n"
	           	   "EndSection\n\n"
	           	   "Section \"InputDevice\"\n"
	           	   "\tIdentifier \"MyKeyboard\"\n"
			   "\tDriver \"kbd\"\n"
			   "EndSection\n\n"
			   "Section \"Monitor\"\n"
			   "\tIdentifier \"MyMonitor\"\n"
			   "\tVendorname \"Unknown\"\n"
		           "\tModelName  \"Unknown\"\n"
			   "\tOption     \"DPMS\"\n"
			   "EndSection\n\n");
	           	fprintf(fp,"Section \"Device\"\n"
	           	   "\tIdentifier \"intel\"\n");
	           	if (!use_intel_driver)
	           	{
	           		fprintf(fp,"\tDriver \"modesetting\"\n");
				fprintf(fp,"\t#Option \"AccelMethod\" \"None\"\n");
			}
			else
			{
				fprintf(fp,"\tDriver \"intel\"\n");
				fprintf(fp,"\tOption \"DRI\" \"3\"\n");
				fprintf(fp,"\tOption \"TearFree\" \"false\"\n");
			}
			fprintf(fp,"\tBusID \"PCI:%lu:%lu:%lu\"\n", pcibus_intel, pcidev_intel, pcifunc_intel);
			fprintf(fp,"EndSection\n\n"
		   	   "Section \"Screen\"\n"
		   	   "\tIdentifier \"intel\"\n"
		   	   "\tDevice \"intel\"\n"
		   	   "EndSection\n\n");
		   	fprintf(fp,"Section \"Device\"\n"
		   	   "\tIdentifier \"nvidia\"\n"
		   	   "\tDriver \"nvidia\"\n");
		   	fprintf(fp,"\tBusID \"PCI:%lu:%lu:%lu\"\n", pcibus_nvidia, pcidev_nvidia, pcifunc_nvidia);
		   	fprintf(fp,"EndSection\n\n"
		   	   "Section \"Screen\"\n"
		   	   "\tIdentifier \"nvidia\"\n"
		   	   "\tDevice \"nvidia\"\n"
		   	   "\tMonitor \"MyMonitor\"\n"
		   	   "\tOption \"AllowEmptyInitialConfiguration\" \"true\"\n"
		   	   "\t#Option \"UseDisplayDevice\" \"None\"\n"
		   	   "\t#Option \"IgnoreDisplayDevices\" \"CRT\"\n"
		   	   "\t#Option \"UseEDID\" \"off\"\n"
		   	   "\t#Option \"UseEdidDpi\" \"false\"\n"
		   	   "\t#Option \"DPI\" \"96 x 96\"\n"
		   	   "\t#Option \"DPI\" \"192 x 192\"\n"
		   	   "\t#Option \"DPI\" \"282 x 282\"\n"
		   	   "\t#Option \"TripleBuffer\" \"true\"\n"
		   	   "EndSection\n");
		}
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

	if ((fp = fopen("/etc/X11/xorg.conf.d/20-mageia-prime.conf", "w")) == NULL)
	{
		fprintf(stderr, "Can't write to file /etc/X11/xorg.conf.d/20-mageia-prime.conf: %s (error %d)\n", strerror(errno), errno);
		exit(1);
	}
	fprintf(fp, "# automatically generated by mageia-prime-install\n\n");
	fprintf(fp, "Section \"OutputClass\"\n"
		    "\tIdentifier \"nvidia\"\n"
		    "\tMatchDriver \"nvidia-drm\"\n"
		    "\tDriver \"nvidia\"\n"
		    "\tOption \"AllowEmptyInitialConfiguration\" \"on\"\n");
        if (!use_xorg_prime_offload)
        {
		fprintf(fp,"\tOption \"PrimaryGPU\" \"yes\"\n");
	}
	fprintf(fp,"\tOption \"IgnoreDisplayDevices\" \"CRT\"\n"
	    	   "EndSection\n\n");
	fclose(fp);
	
	if ((fp = fopen("/etc/X11/xinit.d/00mageia-prime.xinit", "w")) == NULL)
	{
		fprintf(stderr, "Can't write to file /etc/X11/xinit.d/00mageia-prime.xinit: %s (error %d)\n", strerror(errno), errno);
		exit(1);
	}
	fprintf(fp, "# to be sourced\n"
	            "# automatically generated by mageia-prime-install\n"
	            "#\n"
	            "/usr/bin/mageia-prime-offload\n"
		    );
	fclose(fp);

	/* 493 is chmod 755 */
	if ((chmod("/etc/X11/xinit.d/00mageia-prime.xinit", 493)) < 0)
	{
		fprintf(stderr, "Can't chmod 755 file /etc/X11/xinit.d/00mageia-prime.xinit: %s (error %d)\n", strerror(errno), errno);
		exit(1);
	}

	if ((fp = fopen("/etc/X11/xsetup.d/00mageia-prime.xsetup", "w")) == NULL)
	{
		fprintf(stderr, "Can't write to file /etc/X11/xsetup.d/00mageia-prime.xsetup: %s (error %d)\n", strerror(errno), errno);
		exit(1);
	}
	fprintf(fp, "# to be sourced\n"
	            "# automatically generated by mageia-prime-install\n"
	            "#\n"
	            "/usr/bin/mageia-prime-offload\n"
		    );
	fclose(fp);

	/* 493 is chmod 755 */
	if ((chmod("/etc/X11/xsetup.d/00mageia-prime.xsetup", 493)) < 0)
	{
		fprintf(stderr, "Can't chmod 755 file /etc/X11/xsetup.d/00mageia-prime.xsetup: %s (error %d)\n", strerror(errno), errno);
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
	
	if (use_nvidia_current && !use_nvidia_390)
	{
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
	}
	else
	{
		fprintf(stderr, "Switching to NVidia GL libraries...");
		if ((ret = system("/usr/sbin/update-alternatives --set gl_conf /etc/nvidia390/ld.so.conf")) != 0)
		{
			fprintf(stderr, "Warning: failed to run update-alternatives --set gl_conf...\n");
			clean++;
		}
		else
		{
			fprintf(stderr, "ok.\n");
		}
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
		fprintf(stderr, "mageia-prime for NVidia graphics card configured (with %d warnings)\n", clean);
	}
	else
	{
		if (!is_xorg_to_restore)
		{
			fprintf(stderr,"mageia-prime for NVidia graphics card installed.\n");
		}
		else
			fprintf(stderr,"mageia-prime for NVidia graphics card reinstalled.\n");
		
	}


	if (!have_to_zap)
	{
		fprintf(stderr, "Please restart X11 or reboot the system.\n");
	}
	else /* zap X11 */
	{
		fprintf(stderr,"Zapping X11.\n");
		if ((ret = system("/bin/systemctl restart display-manager.service")) != 0)
		{
			fprintf(stderr, "Warning: Can't restart display-manager.service: %s (error %d)\n", strerror(errno), errno);
		}
	}

	return(0);
}
