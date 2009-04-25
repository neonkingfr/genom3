/*
 * Copyright (c) 2009 LAAS/CNRS
 * All rights reserved.
 *
 * Redistribution and use  in source  and binary  forms,  with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *   1. Redistributions of  source  code must retain the  above copyright
 *      notice, this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice,  this list of  conditions and the following disclaimer in
 *      the  documentation  and/or  other   materials provided  with  the
 *      distribution.
 *
 * THIS  SOFTWARE IS PROVIDED BY  THE  COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND  ANY  EXPRESS OR IMPLIED  WARRANTIES,  INCLUDING,  BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES  OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR  PURPOSE ARE DISCLAIMED. IN  NO EVENT SHALL THE COPYRIGHT
 * HOLDERS OR      CONTRIBUTORS  BE LIABLE FOR   ANY    DIRECT, INDIRECT,
 * INCIDENTAL,  SPECIAL,  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF  SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE   OF THIS SOFTWARE, EVEN   IF ADVISED OF   THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

#include <string>
#include <iostream>

#include "templateinterpreter.h"
#include "parsers/genom/driver.h"

using namespace std;
using namespace G3nom;

int main(int argc, char* argv[])
{
	string s;
	TemplateInterpreter ti;
	Driver d;
// 	d.setDebug(true);

	if (!d.parseFile("/home/ccpasteur/work/git/g3nom/parsers/genom/test/test.gnm"))
		cout << "Error parsing gen file " << endl;

	ti.setComponent(&(d.component()));

	if (argc > 1) {
		string arg(argv[1]);
		if (arg == "testFile") {
			if (!ti.parseInfoFile("/home/ccpasteur/work/git/g3nom/parsers/template_info/test/template.info"))
				cout << "Error parsing info file " << endl;
			return 0;
		} else {
			cout << "unknown test name: " << arg << endl;
			return 1;
		}
	}
}
// kate: indent-mode cstyle; replace-tabs off; tab-width 4; 