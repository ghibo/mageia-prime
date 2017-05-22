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

int main(int argc, char **argv)
{
	FILE *fp;
	int ret, clean = 0;
	uid_t uid;
	int i;
	int is_xorg_free;
	int have_to_zap = 0;
	
	if ((uid = getuid()) != 0) {
		fprintf(stderr, "Warning: you must run this command as root!\n\n");
		clean++;
	}
	
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
                				
					default:
						break;
                		}
                	}
		}
	}

	if ((fp = fopen("/etc/X11/xorg.conf.nvidiaprime", "r")) == NULL)
	{
		fprintf(stderr,"Error: It seems Mageia-Prime was not configured.\nYou have to invoke mageia-prime-install before.\n");
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
			fprintf(stderr,"Error: It seems Mageia-Prime was never configured, can't restore previous configuration!\n");
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
			fprintf(stderr, "Warning: Can't remove file/etc/X11/xorg.conf: %s (%d)\n", strerror(errno), errno);
			clean++;
		}	
	}
	
	
	if ((ret = remove("/etc/X11/xorg.conf.nvidiaprime")) != 0)
	{
		fprintf(stderr, "Warning: Can't remove file /etc/X11/xorg.conf.nvidiaprime: %s (error %d)\n", strerror(errno), errno);
		clean++;
	}
	
	if ((ret = remove("/etc/X11/xsetup.d/000nvidiaprime.xsetup")) != 0)
	{
		fprintf(stderr, "Warning: Can't remove file /etc/X11/xsetup.d/000nvidiaprime.xsetup: %s (error %d)\n", strerror(errno), errno);
		clean++;
	}


	if ((ret = remove("/etc/modules-load.d/nvidiaprime-drm.conf")) != 0)
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
		if ((ret = remove("/etc/modprobe.d/00_mageia-prime.conf")) != 0)
		{
			fprintf(stderr, "Warning: Can't remove file /etc/modprobe.d/nvidia-prime-drm.conf: %s (error %d)\n", strerror(errno), errno);
			clean++;
		}
	}

	if (clean > 0)
	{
		fprintf(stderr,"There were %d warnings or errors. Probably you have to unconfigure manually.\n", clean);
	}
	else
	{
		fprintf(stderr, "Mageia-Prime uninstalled and old configuration restored. Please restart X11 or reboot.\n");
	}

	if (have_to_zap)
	{
		fprintf(stderr,"Zapping X11.\n");
		system("systemctl restart prefdm.service");
	}

	return(0);
}
