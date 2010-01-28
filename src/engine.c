/*
 * Copyright (c) 2010 LAAS/CNRS
 * All rights reserved.
 *
 * Redistribution  and  use  in  source  and binary  forms,  with  or  without
 * modification, are permitted provided that the following conditions are met:
 *
 *   1. Redistributions of  source  code must retain the  above copyright
 *      notice and this list of conditions.
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice and  this list of  conditions in the  documentation and/or
 *      other materials provided with the distribution.
 *
 * THE SOFTWARE  IS PROVIDED "AS IS"  AND THE AUTHOR  DISCLAIMS ALL WARRANTIES
 * WITH  REGARD   TO  THIS  SOFTWARE  INCLUDING  ALL   IMPLIED  WARRANTIES  OF
 * MERCHANTABILITY AND  FITNESS.  IN NO EVENT  SHALL THE AUTHOR  BE LIABLE FOR
 * ANY  SPECIAL, DIRECT,  INDIRECT, OR  CONSEQUENTIAL DAMAGES  OR  ANY DAMAGES
 * WHATSOEVER  RESULTING FROM  LOSS OF  USE, DATA  OR PROFITS,  WHETHER  IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR  OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 *                                           Anthony Mallet on Thu Jan  7 2010
 */
#include "acgenom.h"

#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <libgen.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <err.h>

#include <sys/ioctl.h>
#include <termios.h>

#include "genom.h"


/* --- local data ---------------------------------------------------------- */

/* engines */
#ifdef WITH_TCL
extern const engdescr eng_tcl;
#endif

/* default engine */
static const engdescr *engine = NULL;

/** template options */
static char **engopts = NULL;

/** number of template options */
static int nengopts;

/** standard file descriptors of genom and template child */
int stdfd[4];


static char *	eng_findtmpl(const char *dir);
static int	eng_printpipe(int fd, FILE *out, const char *bol,
			int *firstline);
int		eng_swapfd(int from, int to);


/* --- eng_listeng --------------------------------------------------------- */

/** list available templates
 */
int
eng_listeng()
{
  char path[PATH_MAX];
  struct dirent de, *r;
  struct stat sb;
  DIR *d;
  char *name;
  int n;

  /* look for template.xxx entries */
  xwarnx("using template directory '%s'", runopt.tmpldir);
  d = opendir(runopt.tmpldir);
  if (!d) {
    warnx("cannot open directory %s", runopt.tmpldir);
    warn(NULL);
    return errno;
  }

  n = 0;
  /* must use readdir_r() here because eng_findtmpl() also readdir() */
  while(!readdir_r(d, &de, &r) && r) {
    if (!(de.d_type & DT_DIR) && de.d_type != DT_UNKNOWN) continue;

    strlcpy(path, runopt.tmpldir, sizeof(path));
    strlcat(path, "/", sizeof(path));
    strlcat(path, de.d_name, sizeof(path));
    if (stat(path, &sb)) continue;
    if (!(sb.st_mode & S_IFDIR)) continue;

    xwarnx("looking for template in '%s'", de.d_name);
    name = eng_findtmpl(path);
    if (name) {
      if (strlen(de.d_name) >= 68) {
	de.d_name[68] = '\0';
	de.d_name[67] = de.d_name[66] = de.d_name[65] = '.';
      }
      name += strlen(TMPL_SPECIAL_FILE);
      printf("%-70s %s\n", de.d_name, name);
      n++;
    }
  }
  closedir(d);

  if (!n) { warnx("no template found!"); return ENOENT; }
  return 0;
}


/* --- eng_seteng ---------------------------------------------------------- */

/** set engine used for component generation
 */
int
eng_seteng(const char *tmpl)
{
  const engdescr *engs[] = {
#ifdef WITH_TCL
    &eng_tcl,
#endif
    NULL
  };
  const engdescr *e;
  char *name;

  /* look for template.xxx entry */
  name = eng_findtmpl(tmpl);
  if (!name) {
    warnx("cannot find template entry '" TMPL_SPECIAL_FILE "<engine>'");
    return ENOENT;
  }
  name += strlen(TMPL_SPECIAL_FILE);

  /* select engine based on file extension */
  for(e = engs[0]; e; e++)
    if (!strncmp(e->name, name, strlen(name))) {
      engine = e;
      break;
    }

  if (!e) {
    if (name[0])
      warnx("unknown generator engine '%s'", name);
    else
      warnx("no generator engine");
    return errno = ENOENT;
  }
  xwarnx("using %s generator engine", name);
  return 0;
}


/* --- eng_optappend ------------------------------------------------------- */

int
eng_optappend(const char *opt, int index)
{
  int s = opt_append(&engopts, &nengopts, opt, index);
  if (s) {
    warnx("failed to add template option `%s'", opt);
    return s;
  }

  xwarnx("added template option `%s'", opt);
  return 0;
}


/* --- eng_optrm ----------------------------------------------------------- */

int
eng_optrm(int index)
{
  return opt_rm(&engopts, &nengopts, index);
}


/* --- eng_invoke ---------------------------------------------------------- */

/** invoke template engine.
 */
int
eng_invoke()
{
  char tmpl[PATH_MAX];
  int bol[2], m;
  pid_t pid;
  fd_set fdset;
  int s;

  if (!engine) { warnx("no template engine"); return EINVAL; }

  /* setup engine's standard output and error descriptors */
  if (pipe(&stdfd[0]) < 0 || pipe(&stdfd[2]) < 0) {
    warn("cannot create a pipe to template engine:");
    return errno;
  }

  /* invoke engine - in a fork()ed process so that it is easy to redefine
   * stdout and stderr or implement a totally different engine strategy (than
   * default Tcl). fork() might also provide more  isolation in case the
   * template messes up everything.
   */
  strlcpy(tmpl, runopt.tmpl, sizeof(tmpl));
  strlcpy(tmpl, basename(tmpl), sizeof(tmpl));
  s = eng_optappend(tmpl, 0);
  if (s) return s;
  if (runopt.verbose) {
    char **o;
    xwarnx("invoking template engine with the following arguments:");
    for(o = engopts; *o; o++) xwarnx("  + %s", *o);
  }

  switch((pid = fork())) {
    case -1:
      warnx("unable to fork"); warn(NULL);
      return errno = EAGAIN;

    case 0: /* child */
      close(stdfd[0]); close(stdfd[2]);

      if (eng_swapfd(stdfd[1], fileno(stdout)) < 0 ||
	  eng_swapfd(stdfd[3], fileno(stderr)) < 0) {
	warnx("unable to set template engine stdout/stderr"); warn(NULL);
	_exit(2);
      }

      /* make stdout/stderr unbuffered */
      setvbuf(stdout, NULL, _IONBF, 0);
      setvbuf(stderr, NULL, _IONBF, 0);

      s = engine->invoke(runopt.tmpl, nengopts, engopts);
      _exit(s);
      break;

    default: /* parent */
      close(stdfd[1]); close(stdfd[3]);
      eng_optrm(0);
      break;
  }

  /* wait for engine and print output */
  bol[0] = bol[1] = 0;
  m = stdfd[0]>stdfd[2] ? stdfd[0] : stdfd[2];
  FD_ZERO(&fdset);
  FD_SET(stdfd[0], &fdset);
  FD_SET(stdfd[2], &fdset);
  do {
    s = select(1 + m, &fdset, NULL, NULL, NULL);
    if (s > 0) {
      if (FD_ISSET(stdfd[0], &fdset)) {
	if (eng_printpipe(stdfd[0], runopt.verbose?stdout:NULL, tmpl, &bol[0]))
	  bol[0] = -1;
      } else FD_SET(stdfd[0], &fdset);

      if (FD_ISSET(stdfd[2], &fdset)) {
	if (eng_printpipe(stdfd[2], stderr, tmpl, &bol[1]))
	  bol[1] = -1;
      } else FD_SET(stdfd[2], &fdset);

      if (bol[0] == -1 && bol[1] == -1) break;
    }
  } while(s >= 0 || errno == EINTR);
  if (s < 0) warnx("unable to read template engine output");

  close(stdfd[0]); close(stdfd[2]);

  waitpid(pid, &s, 0);
  if ((!WIFEXITED(s) || WEXITSTATUS(s))) {
    if (WIFSIGNALED(s)) {
      warnx("generator engine exited with signal %d", WTERMSIG(s));
      return 127;
    }
    warnx("generator engine exited with code %d", WEXITSTATUS(s));
    return WEXITSTATUS(s);
  }

  xwarnx("generator engine exited with code %d", WEXITSTATUS(s));
  return 0;
}


/* --- eng_findtmpl -------------------------------------------------------- */

/** return the name of the first template.xxx file in directory
 */
static char *
eng_findtmpl(const char *dir)
{
  struct dirent *de;
  char *name;
  DIR *d;

  /* look for template.xxx entry */
  d = opendir(dir);
  if (!d) {
    warnx("cannot open directory %s", dir);
    warn(NULL);
    return NULL;
  }

  name = NULL;
  while((de = readdir(d))) {
    if (strncmp(de->d_name, TMPL_SPECIAL_FILE, strlen(TMPL_SPECIAL_FILE)))
      continue;
    if (de->d_type & DT_DIR) {
      warnx("template entry '%s' should not be a directory", de->d_name);
      continue;
    }

    xwarnx("found template entry '%s'", de->d_name);
    name = de->d_name;
    break;
  }
  closedir(d);

  return name;
}


/* --- eng_printpipe ------------------------------------------------------- */

/** Read from fd as much data as possible and print data to FILE *, prefixing
 * every beginning of line with 'bol'.
 */
static int
eng_printpipe(int fd, FILE *out, const char *bol, int *firstline)
{
  char buffer[1024];
  ssize_t s;
  char *l, *n;

  s = read(fd, buffer, sizeof(buffer)-1);
  if (s == 0) return 1/*eof*/;
  if (s < 0 && errno != EINTR) {
    warn("cannot read generator output");
    return 0;
  }
  buffer[s] = '\0';

  if (!out) return 0;
  if (!bol) {
    fputs(buffer, out);
    return 0;
  }

  /* handle the prefix for the first line */
  if (!*firstline) { fprintf(out, "%s: ", bol); *firstline = 1; }

  /* handle subsequent lines */
  n = buffer;
  for(l = strsep(&n, "\n"); l; l = strsep(&n, "\n")) {
    if (n) {
      if (*n) {
	fprintf(out, "%s\n%s: ", l, bol);
	fflush(out);
      } else {
	fprintf(out, "%s\n", l);
	*firstline = 0;
      }
    } else {
      fprintf(out, "%s", l);
      fflush(out);
    }
  }

  return 0;
}


/* --- eng_swapfd ---------------------------------------------------------- */

/** Exchange fd 'from' and fd 'to'.
 */
int
eng_swapfd(int from, int to)
{
  int t;

  t = dup(from); if (t < 0) return errno;
  if (dup2(to, from) < 0) return errno;
  if (dup2(t, to) < 0) return errno;
  close(t);

  return 0;
}