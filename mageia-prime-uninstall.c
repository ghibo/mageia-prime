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

int main()
{
	FILE *fp;
	int ret, clean = 0;
	uid_t uid;

	if ((uid = getuid()) != 0) {
		fprintf(stderr, "Warning: you must run this command as root!\n\n");
		clean++;
	}
	
	if ((fp = fopen("/etc/X11/xorg.conf.bak.beforenvidiaprime", "r")) == NULL)
	{
		fprintf(stderr,"It seems Mageia NVidia Prime was never configured, can't restore previous configuration back\n");
		exit(1);
	}
	else
	{
		fclose(fp);
	}

	if ((rename("/etc/X11/xorg.conf.bak.beforenvidiaprime", "/etc/X11/xorg.conf")) != 0)
	{
		fprintf(stderr, "Warning: Can't restore file /etc/X11/xorg.conf: %d %s\n", errno, strerror(errno));
		fprintf(stderr, "Warning: You have to manually reconfigure X11 using XFdrake\n");
		clean++;
	}

	if ((ret = remove("/etc/X11/xorg.conf.nvidiaprime")) != 0)
	{
		fprintf(stderr, "Warning: Can't remove file /etc/X11/xorg.conf.nvidiaprime: %d %s\n", errno, strerror(errno));
		clean++;
	}
	
	if ((ret = remove("/etc/X11/xsetup.d/000nvidiaprime.xsetup")) != 0)
	{
		fprintf(stderr, "Warning: Can't remove file /etc/X11/xsetup.d/000nvidiaprime.xsetup: %d %s\n", errno, strerror(errno));
		clean++;
	}


	if ((ret = remove("/etc/modules-load.d/nvidiaprime-drm.conf")) != 0)
	{
		fprintf(stderr, "Warning: Can't remove file /etc/modules-load.d/nvidia-prime-drm.conf: %d %s\n", errno, strerror(errno));
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

	if (clean > 0)
	{
		fprintf(stderr,"There were %d warnings or errors. Probably you have to unconfigure manually.\n", clean);
	}
	else
	{
		fprintf(stderr, "Old configuration restored. Please restart X11 or reboot.\n");
	}

	return(0);
}
