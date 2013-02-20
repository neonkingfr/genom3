/*
 * Copyright (c) 2009-2012 LAAS/CNRS
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
 *                                           Anthony Mallet on Mon Apr 20 2009
 */

/*/ @node Codel declaration
 * @section Codel declaration
 * @cindex codel, declaration
 * @cindex declaration, codel
 *
 * @ruleinclude codel
 * @ruleinclude fsm_codel
 * @ruleinclude codel_parameters
 * @ruleinclude codel_parameter
 * @ruleinclude opt_async
 * @ruleinclude opt_parameter_src
 * @ruleinclude parameter_dir
 * @ruleinclude parameter_variable
 * @ruleinclude event_list
 */
codel:
  identifier '(' codel_parameters ')'
  {
    $$ = codel_create(@1, $1, CODEL_SYNC, NULL, NULL, $3);
  }
;

fsm_codel:
  event_list identifier '(' codel_parameters ')' YIELD identifier_list
  {
    if (!$1 || !$7) {
      parserror(@1, "dropped codel '%s'", $2); $$ = NULL; break;
    }
    $$ = codel_create(@3, $2, CODEL_SYNC, $1, $7, $4);
  }
  | event_list identifier '(' codel_parameters ')' error
  {
    $$ = NULL;
    parserror(@1, "missing 'yield' values for codel %s", $2);
    if ($1) hash_destroy($1, 1);
    if ($4) hash_destroy($4, 1);
  }
;

opt_async:
  /* empty */	{ $$ = CODEL_SYNC; }
  | ASYNC	{ $$ = CODEL_ASYNC; }
;

event_list:
  '<' identifier_list '>'
  {
    $$ = $2;
  }
;

codel_parameters:
  /* empty */
  {
    $$ = hash_create("parameter list", 0);
  }
  | codel_parameter
  {
    $$ = hash_create("parameter list", 3); if (!$$ || !$1) break;
    if (hash_insert($$, param_name($1), $1, (hrelease_f)param_destroy)) {
      param_destroy($1); break;
    }
  }
  | codel_parameters ',' codel_parameter
  {
    $$ = $1; if (!$3) break;
    switch(hash_insert($$, param_name($3), $3, (hrelease_f)param_destroy)) {
      case 0: break;
      case EEXIST:
	parserror(@3, "duplicate parameter '%s'", param_name($3));
	/*FALLTHROUGH*/
      default: param_destroy($3); break;
    }
  }
;

codel_parameter:
  opt_parameter_src parameter_dir parameter_variable
  {
    $$ = param_newcodel(@3, $1, $2, NULL, $3);
  }
  | opt_parameter_src parameter_dir parameter_variable COLONCOLON identifier
  {
    $$ = param_newcodel(@3, $1, $2, $5, $3);
  }
  | opt_parameter_src parameter_dir COLONCOLON identifier
  {
    $$ = param_newcodel(@3, $1, $2, $4, NULL);
  }
  | error { $$ = NULL; }
;

codel_property:
  opt_async CODEL codel semicolon
  {
    if ($3) codel_setkind($3, $1);
    $$ = $3 ? prop_newcodel(@1, PROP_SIMPLE_CODEL, $3) : NULL;
  }
  | opt_async CODEL fsm_codel semicolon
  {
    if ($3) codel_setkind($3, $1);
    $$ = $3 ? prop_newcodel(@1, PROP_FSM_CODEL, $3) : NULL;
  }
;