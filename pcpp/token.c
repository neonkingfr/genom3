/*	$Id: token.c 2017/05/18 15:48:19 mallet $	*/

/*
 * Copyright (c) 2004,2009 Anders Magnusson. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Tokenizer for the C preprocessor.
 * There are three main routines:
 *	- fastscan() loops over the input stream searching for magic
 *		characters that may require actions.
 *	- sloscan() tokenize the input stream and returns tokens.
 *		It may recurse into itself during expansion.
 *	- yylex() returns something from the input stream that 
 *		is suitable for yacc.
 *
 *	Other functions of common use:
 *	- inpch() returns a raw character from the current input stream.
 *	- inch() is like inpch but \\n and trigraphs are expanded.
 *	- unch() pushes back a character to the input stream.
 */

#include "acgenom.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <fcntl.h>
#include <errno.h>

#include "genom.h"
#include "cpp.h"
#include "genom_pcpp-cpy.h"

static void cvtdig(int rad);
static int charcon(usch *);
static void elsestmt(void);
static void ifdefstmt(void);
static void ifndefstmt(void);
static void endifstmt(void);
static void ifstmt(void);
static void cpperror(void);
static void pragmastmt(void);
static void undefstmt(void);
static void cppwarning(void);
static void elifstmt(void);
static void badop(const char *);
static int chktg(void);
static void ppdir(void);
void  include(void);
void  include_next(void);
void  define(void);
static int inpch(void);

extern int yyget_lineno (void);
extern void yyset_lineno (int);

static int inch(void);

int inif;
extern int dflag;

#define	PUTCH(ch) if (!flslvl) putch(ch)
/* protection against recursion in #include */
#define MAX_INCLEVEL	100
static int inclevel;

/* get next character unaltered */
#define	NXTCH() (ifiles->curptr < ifiles->maxread ? *ifiles->curptr++ : inpch())

usch yytext[CPPBUF];

char spechr[256] = {
	0,	0,	0,	0,	C_SPEC,	C_SPEC,	0,	0,
	0,	C_WSNL,	C_SPEC|C_WSNL,	0,
	0,	C_WSNL,	0,	0,
	0,	0,	0,	0,	0,	0,	0,	0,
	0,	0,	0,	0,	0,	0,	0,	0,

	C_WSNL,	C_2,	C_SPEC,	0,	0,	0,	C_2,	C_SPEC,
	0,	0,	0,	C_2,	0,	C_2,	0,	C_SPEC|C_2,
	C_I,	C_I,	C_I,	C_I,	C_I,	C_I,	C_I,	C_I,
	C_I,	C_I,	0,	0,	C_2,	C_2,	C_2,	C_SPEC,

	0,	C_I,	C_I,	C_I,	C_I,	C_I|C_EP, C_I,	C_I,
	C_I,	C_I,	C_I,	C_I,	C_I,	C_I,	C_I,	C_I,
	C_I|C_EP, C_I,	C_I,	C_I,	C_I,	C_I,	C_I,	C_I,
	C_I,	C_I,	C_I,	0,	C_I,	0,	0,	C_I,

	0,	C_I,	C_I,	C_I,	C_I,	C_I|C_EP, C_I,	C_I,
	C_I,	C_I,	C_I,	C_I,	C_I,	C_I,	C_I,	C_I,
	C_I|C_EP, C_I,	C_I,	C_I,	C_I,	C_I,	C_I,	C_I,
	C_I,	C_I,	C_I,	0,	C_2,	0,	0,	0,

};

/*
 * No-replacement array.  If a macro is found and exists in this array
 * then no replacement shall occur.  This is a stack.
 */
struct symtab *norep[RECMAX];	/* Symbol table index table */
int norepptr = 1;			/* Top of index table */
unsigned short bptr[RECMAX];	/* currently active noexpand macro stack */
int bidx;			/* Top of bptr stack */

static void
unch(int c)
{
		
	--ifiles->curptr;
	if (ifiles->curptr < ifiles->bbuf)
		error("pushback buffer full");
	*ifiles->curptr = (usch)c;
}

static int
eatcmnt(void)
{
	int ch;

	if (Cflag) { PUTCH('/'); PUTCH('*'); }
	for (;;) {
		ch = inch();
		if (ch == '\n') {
			ifiles->lineno++;
			PUTCH('\n');
		}
		if (ch == -1)
			return -1;
		if (ch == '*') {
			ch = inch();
			if (ch == '/') {
				if (Cflag) {
					PUTCH('*');
					PUTCH('/');
				} else
					PUTCH(' ');
				break;
			}
			unch(ch);
			ch = '*';
		}
		if (Cflag) PUTCH(ch);
	}
	return 0;
}

/*
 * Scan quickly the input file searching for:
 *	- '#' directives
 *	- keywords (if not flslvl)
 *	- comments
 *
 *	Handle strings, numbers and trigraphs with care.
 *	Only data from pp files are scanned here, never any rescans.
 *	TODO: Only print out strings before calling other functions.
 */
static void
fastscan(void)
{
	struct symtab *nl;
	int ch, i, ccnt;
	usch *cp;

	goto run;
	for (;;) {
		ch = NXTCH();
xloop:		if (ch == -1)
			return;
		if (dflag>1)
			printf("fastscan ch %d (%c)\n", ch, ch > 31 ? ch : '@');
		if ((spechr[ch] & C_SPEC) == 0) {
			PUTCH(ch);
			continue;
		}
		switch (ch) {
		case EBLOCK:
		case WARN:
		case CONC:
			error("bad char passed");
			break;

		case '/': /* Comments */
			if ((ch = inch()) == '/') {
cppcmt:				if (Cflag) { PUTCH(ch); } else { PUTCH(' '); }
				do {
					if (Cflag) PUTCH(ch);
					ch = inch();
				} while (ch != -1 && ch != '\n');
				goto xloop;
			} else if (ch == '*') {
				if (eatcmnt())
					return;
			} else {
				PUTCH('/');
				goto xloop;
			}
			break;

		case '?':  /* trigraphs */
			if ((ch = chktg()))
				goto xloop;
			PUTCH('?');
			break;

		case '\\':
			if ((ch = NXTCH()) == '\n') {
				ifiles->lineno++;
				continue;
			} else {
				PUTCH('\\');
			}
			goto xloop;

		case '\n': /* newlines, for pp directives */
run2:			ifiles->lineno++;
			do {
				PUTCH(ch);
run:				ch = NXTCH();
				if (ch == '/') {
					ch = NXTCH();
					if (ch == '/')
						goto cppcmt;
					if (ch == '*') {
						if (eatcmnt())
							return;
						goto run;
					} 
					unch(ch);
					ch = '/';
				}
			} while (ch == ' ' || ch == '\t');
			if (ch == '\\') {
				ch = NXTCH();
				if (ch == '\n')
					goto run2;
				unch(ch);
				ch = '\\';
			}
			if (ch == '#') {
				ppdir();
				continue;
			} else if (ch == '%') {
				ch = NXTCH();
				if (ch == ':') {
					ppdir();
					continue;
				} else {
					unch(ch);
					ch = '%';
				}
			}
			goto xloop;

		case '\"': /* strings */
str:			PUTCH(ch);
			while ((ch = inch()) != '\"') {
					PUTCH(ch);
				if (ch == '\\') {
					ch = inch();
					PUTCH(ch);
				}
				if (ch < 0)
					return;
			}
			PUTCH(ch);
			break;

		case '.':  /* for pp-number */
			PUTCH(ch);
			ch = NXTCH();
			if (ch < '0' || ch > '9')
				goto xloop;
			/* FALLTHROUGH */
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			do {
				PUTCH(ch);
nxt:				ch = NXTCH();
				if (ch == '\\') {
					ch = NXTCH();
					if (ch == '\n') {
						goto nxt;
					} else {
						unch(ch);
						ch = '\\';
					}
				}
				if (spechr[ch] & C_EP) {
					PUTCH(ch);
					ch = NXTCH();
					if (ch == '-' || ch == '+')
						continue;
				}
			} while ((spechr[ch] & C_ID) || (ch == '.'));
			goto xloop;

		case '\'': /* character literal */
con:			PUTCH(ch);
			if (tflag)
				continue; /* character constants ignored */
			while ((ch = NXTCH()) != '\'') {
				PUTCH(ch);
				if (ch == '\\') {
					ch = NXTCH();
					PUTCH(ch);
				} else if (ch < 0)
					return;
				else if (ch == '\n')
					goto xloop;
			}
			PUTCH(ch);
			break;

		case 'L':
			ch = NXTCH();
			if (ch == '\"') {
				PUTCH('L');
				goto str;
			}
			if (ch == '\'') {
				PUTCH('L');
				goto con;
			}
			unch(ch);
			ch = 'L';
			/* FALLTHROUGH */
		default:
			if ((spechr[ch] & C_ID) == 0)
				error("fastscan");
			if (flslvl) {
				while (spechr[ch] & C_ID)
					ch = NXTCH();
				goto xloop;
			}
			i = ccnt = 0;
			do {
				yytext[i++] = (usch)ch;
				ch = NXTCH();
				if (ch == '\\') {
					ch = NXTCH();
					if (ch != '\n') {
						unch('\n');
						ch = '\\';
					} else {
						ifiles->lineno++;
						ch = NXTCH();
					}
				}
				if (ch < 0)
					return;
			} while (spechr[ch] & C_ID);

			yytext[i] = 0;
			unch(ch);

			cp = stringbuf;
			if ((nl = lookup((usch *)yytext, FIND)) && kfind(nl)) {
				putstr(stringbuf);
			} else
				putstr((usch *)yytext);
			stringbuf = cp;

			break;
		}
	}
}

int
sloscan()
{
	int ch;
	int yyp;

zagain:
	yyp = 0;
 	ch = inch();
	yytext[yyp++] = (usch)ch;
	switch (ch) {
	case -1:
		return 0;
	case '\n':
		/* sloscan() never passes \n, that's up to fastscan() */
		unch(ch);
		goto yyret;

	case '\r': /* Ignore CR's */
		yyp = 0;
		break;

	case '0': case '1': case '2': case '3': case '4': case '5': 
	case '6': case '7': case '8': case '9':
		/* readin a "pp-number" */
ppnum:		for (;;) {
			ch = inch();
			if (spechr[ch] & C_EP) {
				yytext[yyp++] = (usch)ch;
				ch = inch();
				if (ch == '-' || ch == '+') {
					yytext[yyp++] = (usch)ch;
				} else
					unch(ch);
				continue;
			}
			if ((spechr[ch] & C_ID) || ch == '.') {
				yytext[yyp++] = (usch)ch;
				continue;
			} 
			break;
		}
		unch(ch);
		yytext[yyp] = 0;

		return NUMBER;

	case '\'':
chlit:		
		for (;;) {
			if ((ch = inch()) == '\\') {
				yytext[yyp++] = (usch)ch;
				yytext[yyp++] = (usch)inch();
				continue;
			} else if (ch == '\n') {
				/* not a constant */
				while (yyp > 1)
					unch(yytext[--yyp]);
				ch = '\'';
				goto any;
			} else
				yytext[yyp++] = (usch)ch;
			if (ch == '\'')
				break;
		}
		yytext[yyp] = 0;

		return (NUMBER);

	case ' ':
	case '\t':
		while ((ch = inch()) == ' ' || ch == '\t')
			yytext[yyp++] = (usch)ch;
		unch(ch);
		yytext[yyp] = 0;
		return(WSPACE);

	case '/':
		if ((ch = inch()) == '/') {
			do {
				yytext[yyp++] = (usch)ch;
				ch = inch();
			} while (ch && ch != '\n');
			yytext[yyp] = 0;
			unch(ch);
			goto zagain;
		} else if (ch == '*') {
			int c, wrn;
			extern int readmac;

			if (Cflag && !flslvl && readmac) {
				unch(ch);
				yytext[yyp] = 0;
				return CMNT;
			}

			wrn = 0;
		more:	while ((c = inch()) && c != '*') {
				if (c == '\n')
					putch(c), ifiles->lineno++;
				else if (c == EBLOCK) {
					(void)inch();
					(void)inch();
				} else if (c == 1) /* WARN */
					wrn = 1;
			}
			if (c == 0)
				return 0;
			if ((c = inch()) && c != '/') {
				unch(c);
				goto more;
			}
			if (c == 0)
				return 0;
			if (!tflag && !Cflag && !flslvl)
				unch(' ');
			if (wrn)
				unch(1);
			goto zagain;
		}
		unch(ch);
		ch = '/';
		goto any;

	case '.':
		ch = inch();
		if (isdigit(ch)) {
			yytext[yyp++] = (usch)ch;
			goto ppnum;
		} else {
			unch(ch);
			ch = '.';
		}
		goto any;

	case '\"':
		if (tflag)
			goto any;
	strng:
		for (;;) {
			if ((ch = inch()) == '\\') {
				yytext[yyp++] = (usch)ch;
				yytext[yyp++] = (usch)inch();
				continue;
			} else 
				yytext[yyp++] = (usch)ch;
			if (ch == '\"')
				break;
		}
		yytext[yyp] = 0;
		return(STRING);

	case 'L':
		if ((ch = inch()) == '\"' && !tflag) {
			yytext[yyp++] = (usch)ch;
			goto strng;
		} else if (ch == '\'' && !tflag) {
			yytext[yyp++] = (usch)ch;
			goto chlit;
		}
		unch(ch);
		/* FALLTHROUGH */

	/* Yetch, all identifiers */
	case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': 
	case 'g': case 'h': case 'i': case 'j': case 'k': case 'l': 
	case 'm': case 'n': case 'o': case 'p': case 'q': case 'r': 
	case 's': case 't': case 'u': case 'v': case 'w': case 'x': 
	case 'y': case 'z':
	case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': 
	case 'G': case 'H': case 'I': case 'J': case 'K':
	case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R': 
	case 'S': case 'T': case 'U': case 'V': case 'W': case 'X': 
	case 'Y': case 'Z':
	case '_': /* {L}({L}|{D})* */

		/* Special hacks */
		for (;;) { /* get chars */
			ch = inch();
			if (isalpha(ch) || isdigit(ch) || ch == '_') {
				yytext[yyp++] = (usch)ch;
			} else {
				unch(ch);
				break;
			}
		}
		yytext[yyp] = 0; /* need already string */
		/* end special hacks */

		return IDENT;
	default:
	any:
		yytext[yyp] = 0;
		return yytext[0];

	} /* endcase */
	goto zagain;

yyret:
	yytext[yyp] = 0;
	return ch;
}

int
yylex()
{
	static int ifdef, noex;
	struct symtab *nl;
	int ch, c2;

	while ((ch = sloscan()) == WSPACE)
		;
	if (ch < 128 && spechr[ch] & C_2)
		c2 = inpch();
	else
		c2 = 0;

#define	C2(a,b,c) case a: if (c2 == b) return c; break
	switch (ch) {
	C2('=', '=', EQ);
	C2('!', '=', NE);
	C2('|', '|', OROR);
	C2('&', '&', ANDAND);
	case '<':
		if (c2 == '<') return LS;
		if (c2 == '=') return LE;
		break;
	case '>':
		if (c2 == '>') return RS;
		if (c2 == '=') return GE;
		break;
	case '+':
	case '-':
		if (ch == c2)
			badop("");
		break;

	case '/':
		if (Cflag == 0 || c2 != '*')
			break;
		/* Found comment that need to be skipped */
		for (;;) {
			ch = inpch();
		c1:	if (ch != '*')
				continue;
			if ((ch = inpch()) == '/')
				break;
			goto c1;
		}
		return yylex();

	case NUMBER:
		if (yytext[0] == '\'') {
			yylval.node.op = NUMBER;
			yylval.node.nd_val = charcon((usch *)yytext);
		} else
			cvtdig(yytext[0] != '0' ? 10 :
			    yytext[1] == 'x' || yytext[1] == 'X' ? 16 : 8);
		return NUMBER;

	case IDENT:
		if (strcmp((char *)yytext, "defined") == 0) {
			ifdef = 1;
			return DEFINED;
		}
		nl = lookup((usch *)yytext, FIND);
		if (ifdef) {
			yylval.node.nd_val = nl != NULL;
			ifdef = 0;
		} else if (nl && noex == 0) {
			usch *och = stringbuf;
			int i;

			i = kfind(nl);
			unch(WARN);
			if (i)
				unpstr(stringbuf);
			else
				unpstr(nl->namep);
			stringbuf = och;
			noex = 1;
			return yylex();
		} else {
			yylval.node.nd_val = 0;
		}
		yylval.node.op = NUMBER;
		return NUMBER;
	case WARN:
		noex = 0;
		return yylex();
	default:
		return ch;
	}
	unch(c2);
	return ch;
}

usch *yyp, yybuf[CPPBUF];

int yywrap(void);

static int
inpch(void)
{
	int len;

	if (ifiles->curptr < ifiles->maxread)
		return *ifiles->curptr++;

	if (ifiles->infil == -1)
		return -1;
	if ((len = read(ifiles->infil, ifiles->buffer, CPPBUF)) < 0)
		error("read error on file %s", ifiles->orgfn);
	if (len == 0)
		return -1;
	ifiles->curptr = ifiles->buffer;
	ifiles->maxread = ifiles->buffer + len;
	return inpch();
}

static int
inch(void)
{
	int c;

again:	switch (c = inpch()) {
	case '\\': /* continued lines */
msdos:		if ((c = inpch()) == '\n') {
			ifiles->lineno++;
			goto again;
		} else if (c == '\r')
			goto msdos;
		unch(c);
		return '\\';
	case '?': /* trigraphs */
		if ((c = chktg())) {
			unch(c);
			goto again;
		}
		return '?';
	default:
		return c;
	}
}

/*
 * Let the command-line args be faked defines at beginning of file.
 */
static void
prinit(struct initar *it, struct includ *ic)
{
	const char *pre, *post;
	char *a;

	if (it->next)
		prinit(it->next, ic);
	pre = post = NULL; /* XXX gcc */
	switch (it->type) {
	case 'D':
		pre = "#define ";
		if ((a = strchr(it->str, '=')) != NULL) {
			*a = ' ';
			post = "\n";
		} else
			post = " 1\n";
		break;
	case 'U':
		pre = "#undef ";
		post = "\n";
		break;
	case 'i':
		pre = "#include \"";
		post = "\"\n";
		break;
	default:
		error("prinit");
	}
	strlcat((char *)ic->buffer, pre, CPPBUF+1);
	strlcat((char *)ic->buffer, it->str, CPPBUF+1);
	if (strlcat((char *)ic->buffer, post, CPPBUF+1) >= CPPBUF+1)
		error("line exceeds buffer size");

	ic->lineno--;
	while (*ic->maxread)
		ic->maxread++;
}

/*
 * A new file included.
 * If ifiles == NULL, this is the first file and already opened (stdin).
 * Return 0 on success, -1 if file to be included is not found.
 */
int
pushfile(const usch *file, const usch *fn, int idx, void *incs)
{
	extern struct initar *initar;
	struct includ ibuf;
	struct includ *ic;
	int otrulvl;

	ic = &ibuf;
	ic->next = ifiles;

	if (file != NULL) {
		if ((ic->infil = open((const char *)file, O_RDONLY)) < 0)
			return -1;
		ic->orgfn = ic->fname = file;
		if (++inclevel > MAX_INCLEVEL)
			error("Limit for nested includes exceeded");
	} else {
		ic->infil = 0;
		ic->orgfn = ic->fname = (const usch *)"<stdin>";
	}
#ifndef BUF_STACK
	ic->bbuf = malloc(BBUFSZ);
#endif
	ic->buffer = ic->bbuf+NAMEMAX;
	ic->curptr = ic->buffer;
	ifiles = ic;
	ic->lineno = 1;
	ic->maxread = ic->curptr;
	ic->idx = idx;
	ic->incs = incs;
	ic->fn = fn;
	prtline();
	if (initar) {
		int oin = ic->infil;
		ic->infil = -1;
		*ic->maxread = 0;
		prinit(initar, ic);
		initar = NULL;
		if (dMflag)
			if (write(ofd, ic->buffer, strlen((char *)ic->buffer))) {}
		fastscan();
		prtline();
		ic->infil = oin;
	}

	otrulvl = trulvl;

	fastscan();

	if (otrulvl != trulvl || flslvl)
		error("unterminated conditional");

#ifndef BUF_STACK
	free(ic->bbuf);
#endif
	ifiles = ic->next;
	close(ic->infil);
	inclevel--;
	return 0;
}

/*
 * Print current position to output file.
 */
void
prtline()
{
	usch *s, *os = stringbuf;

	if (Mflag) {
		if (dMflag)
			return; /* no output */
		if (ifiles->lineno == 1) {
			s = sheap("%s: %s\n", Mfile, ifiles->fname);
			if (write(ofd, s, strlen((char *)s))) {}
		}
	} else if (!Pflag)
		putstr(sheap("\n# %d \"%s\"\n", ifiles->lineno, ifiles->fname));
	stringbuf = os;
}

void
cunput(int c)
{
#ifdef CPP_DEBUG
//	extern int dflag;
//	if (dflag)printf(": '%c'(%d)\n", c > 31 ? c : ' ', c);
#endif
#if 0
if (c == 10) {
	printf("c == 10!!!\n");
}
#endif
	unch(c);
}

int yywrap(void) { return 1; }

static int
dig2num(int c)
{
	if (c >= 'a')
		c = c - 'a' + 10;
	else if (c >= 'A')
		c = c - 'A' + 10;
	else
		c = c - '0';
	return c;
}

/*
 * Convert string numbers to unsigned long long and check overflow.
 */
static void
cvtdig(int rad)
{
	unsigned long long rv = 0;
	unsigned long long rv2 = 0;
	usch *y = yytext;
	int c;

	c = *y++;
	if (rad == 16)
		y++;
	while (isxdigit(c)) {
		rv = rv * rad + dig2num(c);
		/* check overflow */
		if (rv / rad < rv2)
			error("Constant \"%s\" is out of range", yytext);
		rv2 = rv;
		c = *y++;
	}
	y--;
	while (*y == 'l' || *y == 'L')
		y++;
	yylval.node.op = *y == 'u' || *y == 'U' ? UNUMBER : NUMBER;
	yylval.node.nd_uval = rv;
	if ((rad == 8 || rad == 16) && yylval.node.nd_val < 0)
		yylval.node.op = UNUMBER;
	if (yylval.node.op == NUMBER && yylval.node.nd_val < 0)
		/* too large for signed, see 6.4.4.1 */
		error("Constant \"%s\" is out of range", yytext);
}

static int
charcon(usch *p)
{
	int val, c;

	p++; /* skip first ' */
	val = 0;
	if (*p++ == '\\') {
		switch (*p++) {
		case 'a': val = '\a'; break;
		case 'b': val = '\b'; break;
		case 'f': val = '\f'; break;
		case 'n': val = '\n'; break;
		case 'r': val = '\r'; break;
		case 't': val = '\t'; break;
		case 'v': val = '\v'; break;
		case '\"': val = '\"'; break;
		case '\'': val = '\''; break;
		case '\\': val = '\\'; break;
		case 'x':
			while (isxdigit(c = *p)) {
				val = val * 16 + dig2num(c);
				p++;
			}
			break;
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7':
			p--;
			while (isdigit(c = *p)) {
				val = val * 8 + (c - '0');
				p++;
			}
			break;
		default: val = p[-1];
		}

	} else
		val = p[-1];
	return val;
}

static void
chknl(int ignore)
{
	int t;

	while ((t = sloscan()) == WSPACE)
		;
	if (t != '\n') {
		if (t && t != (usch)-1) {
			if (ignore) {
				warning("newline expected, got \"%s\"", yytext);
				/* ignore rest of line */
				while ((t = sloscan()) && t != '\n')
					;
			}
			else
				error("newline expected, got \"%s\"", yytext);
		} else {
			if (ignore)
				warning("no newline at end of file");
			else
				error("no newline at end of file");
		}
	}
}

static void
elsestmt(void)
{
	if (flslvl) {
		if (elflvl > trulvl)
			;
		else if (--flslvl!=0) {
			flslvl++;
		} else {
			trulvl++;
			prtline();
		}
	} else if (trulvl) {
		flslvl++;
		trulvl--;
	} else
		error("If-less else");
	if (elslvl==trulvl+flslvl)
		error("Too many else");
	elslvl=trulvl+flslvl;
	chknl(1);
}

static void
skpln(void)
{
	/* just ignore the rest of the line */
	while (inch() != '\n')
		;
	unch('\n');
	flslvl++;
}

static void
ifdefstmt(void)		 
{ 
	int t;

	if (flslvl) {
		skpln();
		return;
	}
	do
		t = sloscan();
	while (t == WSPACE);
	if (t != IDENT)
		error("bad ifdef");
	if (lookup((usch *)yytext, FIND) == 0) {
		putch('\n');
		flslvl++;
	} else
		trulvl++;
	chknl(0);
}

static void
ifndefstmt(void)	  
{ 
	int t;

	if (flslvl) {
		skpln();
		return;
	}
	do
		t = sloscan();
	while (t == WSPACE);
	if (t != IDENT)
		error("bad ifndef");
	if (lookup((usch *)yytext, FIND) != 0) {
		putch('\n');
		flslvl++;
	} else
		trulvl++;
	chknl(0);
}

static void
endifstmt(void)		 
{
	if (flslvl) {
		flslvl--;
		if (flslvl == 0) {
			putch('\n');
			prtline();
		}
	} else if (trulvl)
		trulvl--;
	else
		error("If-less endif");
	if (flslvl == 0)
		elflvl = 0;
	elslvl = 0;
	chknl(1);
}

static void
ifstmt(void)
{
	if (flslvl == 0) {
		if (yyparse() == 0) {
			putch('\n');
			++flslvl;
		} else
			++trulvl;
	} else
		++flslvl;
}

static void
elifstmt(void)
{
	if (flslvl == 0)
		elflvl = trulvl;
	if (flslvl) {
		if (elflvl > trulvl)
			;
		else if (--flslvl!=0)
			++flslvl;
		else {
			if (yyparse()) {
				++trulvl;
				prtline();
			} else {
				putch('\n');
				++flslvl;
			}
		}
	} else if (trulvl) {
		++flslvl;
		--trulvl;
	} else
		error("If-less elif");
}

static usch *
svinp(void)
{
	int c;
	usch *cp = stringbuf;

	while ((c = inch()) && c != '\n')
		savch(c);
	savch('\n');
	savch(0);
	return cp;
}

static void
cpperror(void)
{
	usch *cp;
	int c;

	if (flslvl)
		return;
	c = sloscan();
	if (c != WSPACE && c != '\n')
		error("bad error");
	cp = svinp();
	if (flslvl)
		stringbuf = cp;
	else
		error("%s", cp);
}

static void
cppwarning(void)
{
	usch *cp;
	int c;

	if (flslvl)
		return;
	c = sloscan();
	if (c != WSPACE && c != '\n')
		error("bad warning");

	/* svinp() add an unwanted \n */
	cp = stringbuf;
	while ((c = inch()) && c != '\n')
		savch(c);
	savch(0);

	if (flslvl)
		stringbuf = cp;
	else
		warning("#warning %s", cp);

	unch('\n');
}

static void
undefstmt(void)
{
	struct symtab *np;

	if (sloscan() != WSPACE || sloscan() != IDENT)
		error("bad undef");
	if (flslvl == 0 && (np = lookup((usch *)yytext, FIND)))
		np->value = 0;
	chknl(0);
}

static void
pragmastmt(void)
{
	int c;
	usch *cp;

	if (sloscan() != WSPACE)
		error("bad pragma");
	if (!flslvl)
		putstr((const usch *)"\n#pragma ");

	cp = stringbuf;
	do {
		c = inch();
		if (!flslvl)
			putch(c);	/* Do arg expansion instead? */
		savch(c);
	} while (c && c != '\n');
	savch(0);
	if (c == '\n') {
		unch(c);
		--stringbuf;
		stringbuf[-1] = 0;
	}
	if (!flslvl)
		pragmaimpl(cp);
	prtline();
}

static void
badop(const char *op)
{
	error("invalid operator in preprocessor expression: %s", op);
}

int
cinput()
{
	return inch();
}

/*
 * Check for (and convert) trigraphs.
 */
int
chktg()
{
	int c;

	if ((c = inpch()) != '?') {
		unch(c);
		return 0;
	}
	switch (c = inpch()) {
	case '=': c = '#'; break;
	case '(': c = '['; break;
	case ')': c = ']'; break;
	case '<': c = '{'; break;
	case '>': c = '}'; break;
	case '/': c = '\\'; break;
	case '\'': c = '^'; break;
	case '!': c = '|'; break;
	case '-': c = '~'; break;
	default:
		unch(c);
		unch('?');
		c = 0;
	}
	return c;
}

static struct {
	const char *name;
	void (*fun)(void);
} ppd[] = {
	{ "ifndef", ifndefstmt },
	{ "ifdef", ifdefstmt },
	{ "if", ifstmt },
	{ "include", include },
	{ "else", elsestmt },
	{ "endif", endifstmt },
	{ "error", cpperror },
	{ "warning", cppwarning },
	{ "define", define },
	{ "undef", undefstmt },
	{ "line", line },
	{ "pragma", pragmastmt },
	{ "elif", elifstmt },
#ifdef GCC_COMPAT
	{ "include_next", include_next },
#endif
};

/*
 * Handle a preprocessor directive.
 */
void
ppdir(void)
{
	char bp[20];
	int ch, i;

	while ((ch = inch()) == ' ' || ch == '\t')
		;
	if (ch == '\n') { /* empty directive */
		unch(ch);
		return;
	}
	if (ch < 'a' || ch > 'z')
		goto out; /* something else, ignore */
	i = 0;
	do {
		bp[i++] = (usch)ch;
		if (i == sizeof(bp)-1)
			goto out; /* too long */
		ch = inch();
	} while ((ch >= 'a' && ch <= 'z') || (ch == '_'));
	unch(ch);
	bp[i++] = 0;

	/* got keyword */
#define	SZ (int)(sizeof(ppd)/sizeof(ppd[0]))
	for (i = 0; i < SZ; i++)
		if (bp[0] == ppd[i].name[0] && strcmp(bp, ppd[i].name) == 0)
			break;
	if (i == SZ)
		goto out;

	/* Found matching keyword */
	(*ppd[i].fun)();
	return;

out:	while ((ch = inch()) != '\n' && ch != -1)
		;
	unch('\n');
}
