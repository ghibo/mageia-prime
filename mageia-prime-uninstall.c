/*
 *
 * mageia-prime-uninstall.c
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
	int ret, clean = 0;
	uid_t uid;
	int i;
	int is_xorg_free;
	int have_to_zap = 0;
	int do_not_blacklist_nouveau = 0;
	int touch_grub = 0;
	int use_quick_install = 0;
	
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

                			case 'z': case 'Z':
                				if (argv[i][2] == '\0')
                				{
                					have_to_zap = 1;
                				}
                				break;

					case 'h': case 'H':
						if (argv[i][2] == '\0')
						{
							fprintf(stderr, "Usage: mageia-prime-uninstall [options]\n\n"
								"where [options] is one or more of:\n"
								"   -h   show this messsage\n"
								"   -b   avoid cleaning of blacklisting nouveau and regenerating initrd images\n"
								"   -g   regenerate grub config\n"
								"   -y   quick-uninstallation, skip regenerating initrd images\n"
								"   -z   \"zap\" X11 after de-configuration\n");
							exit(1);
						}

					case 'g': case 'G':
						if (argv[i][2] == '\0')
						{
							touch_grub = 1;
						}
						break;

					case 'y': case 'Y':
						if (argv[i][2] == '\0')
						{
							use_quick_install = 1;
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

	if ((fp = fopen("/etc/X11/xorg.conf.nvidiaprime", "r")) == NULL)
	{
		fprintf(stderr,"Error: It seems mageia-prime was not configured.\nYou have to invoke mageia-prime-install before.\n");
		exit(1);
	}
	
	if ((fp = fopen("/etc/X11/xorg.conf.nvidiaprime.xorgfree", "r")) != NULL)
	{
		fprintf(stderr, "You were running an X11 system without a xorg.conf.\n");
		is_xorg_free = 1;
		fclose(fp);
	}
	else
	{
		is_xorg_free = 0;
	}

	/* preserve current mageia-prime configuration, just in the case the user has customized it manually */
	if ((fcopy("/etc/X11/xorg.conf", "/etc/X11/xorg.conf.nvidiaprime.preserve")) != 0)
	{
        	fprintf(stderr, "Can't copy /etc/X11/xorg.conf to /etc/X11/xorg.conf.nvidiaprime.preserve: %s (error %d)\n", strerror(errno), errno);
                exit(1);
        }
	
	/* restore previous (intel) xorg.conf configuration */
	if (!is_xorg_free)
	{	
		if ((fp = fopen("/etc/X11/xorg.conf.bak.beforenvidiaprime", "r")) == NULL)
		{
			fprintf(stderr,"Error: It seems mageia-prime was never configured, can't restore previous configuration!\n");
			fprintf(stderr,"You have to invoke mageia-prime-install before.\n");
			exit(1);
		}
		else
		{
			fclose(fp);
		}

		if ((rename("/etc/X11/xorg.conf.bak.beforenvidiaprime", "/etc/X11/xorg.conf")) != 0)
		{
			fprintf(stderr, "Warning: Can't restore file /etc/X11/xorg.conf: %s (%d)\n", strerror(errno), errno);
			fprintf(stderr, "Warning: You have to manually reconfigure X11 using XFdrake\n");
			clean++;
		}
	}

	if (is_xorg_free)
	{
		/* system was xorg.conf free so we remove nvidia-prime config */
		if ((ret = remove("/etc/X11/xorg.conf")) != 0)
		{
			fprintf(stderr, "Warning: Can't remove file /etc/X11/xorg.conf: %s (%d)\n", strerror(errno), errno);
			clean++;
		}	
	}
	
	
	if ((ret = remove("/etc/X11/xorg.conf.nvidiaprime")) != 0)
	{
		fprintf(stderr, "Warning: Can't remove file /etc/X11/xorg.conf.nvidiaprime: %s (error %d)\n", strerror(errno), errno);
		clean++;
	}
	
	if ((ret = remove("/etc/X11/xorg.conf.d/20-mageia-prime.conf")) != 0)
	{
		fprintf(stderr, "Warning: Can't remove file /etc/X11/xorg.conf.d/20-mageia-prime.conf: %s (error %d)\n", strerror(errno), errno);
		clean++;
	}

	if ((ret = remove("/etc/X11/xinit.d/00mageia-prime.xinit")) != 0)
	{
		fprintf(stderr, "Warning: Can't remove file /etc/X11/xinit.d/00mageia-prime.xinit: %s (error %d)\n", strerror(errno), errno);
		clean++;
	}

	if ((ret = remove("/etc/X11/xsetup.d/00mageia-prime.xsetup")) != 0)
	{
		fprintf(stderr, "Warning: Can't remove file /etc/X11/xsetup.d/00mageia-prime.xsetup: %s (error %d)\n", strerror(errno), errno);
		clean++;
	}

	if ((ret = remove("/etc/modules-load.d/nvidia-prime-drm.conf")) != 0)
	{
		fprintf(stderr, "Warning: Can't remove file /etc/modules-load.d/nvidia-prime-drm.conf: %s (error %d)\n", strerror(errno), errno);
		clean++;
	}
	

	fprintf(stderr, "Switching to Intel GL libraries...");
	if ((ret = system("/usr/sbin/update-alternatives --set gl_conf /etc/ld.so.conf.d/GL/standard.conf")) != 0)
	{
		fprintf(stderr, "Warning: failed to run update-alternatives --set gl_conf...\n");
		clean++;
	}
	else
	{
		fprintf(stderr,"ok.\n");
	}
	
	if ((ret = system("/usr/sbin/ldconfig")) != 0)
	{
		fprintf(stderr, "Warning: failed to run ldconfig\n");
		clean++;
	}

	if ((fp = fopen("/etc/modprobe.d/00_mageia-prime.conf", "r")) != NULL)
	{
		fclose(fp);
		
		if (!do_not_blacklist_nouveau)
		{
			if ((ret = remove("/etc/modprobe.d/00_mageia-prime.conf")) != 0)
			{
				fprintf(stderr, "Warning: Can't remove file /etc/modprobe.d/00_mageia-prime.conf: %s (error %d)\n", strerror(errno), errno);
				clean++;
			}

			if (!use_quick_install)
			{
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
	}

	if (touch_grub)
	{
		fprintf(stderr, "Removing \"nouveau.modeset=0\" to grub2...");
 		if (grub_remove_nouveau_nomodeset() != 0)
 		{
 			fprintf(stderr, "failed!\n");
 			clean++;
		}
		else
			fprintf(stderr, "done.\n");
		
		fprintf(stderr, "Updating grub.cfg...");
		if ((ret = system("/usr/bin/update-grub")) != 0)
		{
			fprintf(stderr, "failed!\n");
			clean++;
		}
		else
			fprintf(stderr, "done.\n");

	}

	if (clean > 0)
	{
		fprintf(stderr,"There were %d warnings or errors. Probably you have to unconfigure manually.\n", clean);
	}
	else
	{
		fprintf(stderr, "mageia-prime uninstalled and old configuration restored. Please restart X11 or reboot.\n");
	}

	if (have_to_zap)
	{
		fprintf(stderr,"Zapping X11.\n");
		if ((ret = system("/bin/systemctl restart display-manager.service")) != 0)
		{
			fprintf(stderr, "Warning: Can't restart display-manager.service: %s (error %d)\n", strerror(errno), errno);
		}
	}

	return(0);
}
