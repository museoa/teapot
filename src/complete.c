#define _XOPEN_SOURCE 500

#include <sys/types.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>

#include "complete.h"

void completefile(char *file, char *pos, size_t size)
{
	struct dirent *direntp;
	char *dir, *slash, *complete;
	char next;
	int required, atmost;
	DIR *dirp;

	next = *pos;
	*pos = 0;

	if (!(complete=strrchr(file,'/'))) {
		dir = strdup(".");
		complete = file;

	} else {
		if (complete == file) {
			dir = strdup("/");
		} else {
			dir = strdup(file);
			dir[complete-file] = 0;
		}
		complete++;
	}
	required = strlen(complete);

	if (!(dirp = opendir(dir))) {
		free(dir);
		return;
	}

	while ((direntp = readdir(dirp))) {
		if ((direntp->d_name[0] != '.' || complete[0] == '.') && !strncmp(complete, direntp->d_name, required)) {
			if (!next && strlen(direntp->d_name) > required) {
				strncpy(pos, direntp->d_name+required, size-(pos-file));
				file[size-1] = 0;
				break;
			} else if (direntp->d_name[required] == next && !strcmp(pos+1, direntp->d_name+required+1)) {
				next = 0;
			}
		}
	}

	closedir(dirp);
	free(dir);
}
