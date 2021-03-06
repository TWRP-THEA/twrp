/*
	Copyright 2012 to 2016 bigbiff/Dees_Troy TeamWin
	This file is part of TWRP/TeamWin Recovery Project.

	TWRP is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	TWRP is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with TWRP.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <errno.h>
#include <fstream>
#include <libgen.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>

#include "gui/console.h"
#include "set_metadata.h"
#include "twcommon.h"
#include "twrp-functions.hpp"
#include "twrpDigest.hpp"

extern "C" {
	#include "digest/md5.h"
}

using namespace std;

void twrpDigest::setfn(const string& fn) {
	md5fn = fn;
}

int twrpDigest::computeMD5(void) {
	string line;
	struct MD5Context md5c;
	FILE *file;
	int len;
	unsigned char buf[1024];
	MD5Init(&md5c);
	file = fopen(md5fn.c_str(), "rb");
	if (file == NULL)
		return -1;
	while ((len = fread(buf, 1, sizeof(buf), file)) > 0) {
		MD5Update(&md5c, buf, len);
	}
	fclose(file);
	MD5Final(md5sum, &md5c);
	return 0;
}

int twrpDigest::write_md5digest(void) {
	int i;
	string md5string, md5file;
	char hex[3];
	md5file = md5fn + ".md5";

	for (i = 0; i < 16; ++i) {
		snprintf(hex, 3, "%02x", md5sum[i]);
		md5string += hex;
	}
	md5string += "  ";
	md5string += basename((char*) md5fn.c_str());
	md5string +=  + "\n";
	TWFunc::write_file(md5file, md5string);
	tw_set_default_metadata(md5file.c_str());
	LOGINFO("MD5 for %s: %s\n", md5fn.c_str(), md5string.c_str());
	return 0;
}

int twrpDigest::read_md5digest(void) {
	size_t i = 0;
	bool foundMd5File = false;
	string md5file = "";
	vector<string> md5ext;
	md5ext.push_back(".md5");
	md5ext.push_back(".md5sum");

	while (i < md5ext.size()) {
		md5file = md5fn + md5ext[i];
		if (TWFunc::Path_Exists(md5file)) {
			foundMd5File = true;
			break;
		}
		i++;
	}

	if (!foundMd5File) {
		gui_msg("no_md5=Skipping MD5 check: no MD5 file found");
		return -1;
	} else if (TWFunc::read_file(md5file, line) != 0) {
		LOGERR("Skipping MD5 check: MD5 file unreadable %s\n", strerror(errno));
		return 1;
	}

	return 0;
}

/* verify_md5digest return codes:
	-2: md5 did not match
	-1: no md5 file found
	 0: md5 matches
	 1: md5 file unreadable
*/

int twrpDigest::verify_md5digest(void) {
	string buf;
	char hex[3];
	int i, ret;
	string md5string;

	ret = read_md5digest();
	if (ret != 0)
		return ret;
	stringstream ss(line);
	vector<string> tokens;
	while (ss >> buf)
		tokens.push_back(buf);
	computeMD5();
	for (i = 0; i < 16; ++i) {
		snprintf(hex, 3, "%02x", md5sum[i]);
		md5string += hex;
	}
	if (tokens.at(0) != md5string) {
		gui_err("md5_fail=MD5 does not match");
		return -2;
	}

	gui_msg("md5_match=MD5 matched");
	return 0;
}
