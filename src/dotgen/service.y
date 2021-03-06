/*
 * Copyright (c) 2009-2015,2017,2019 LAAS/CNRS
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

/*/
 * Service declaration
 * -------------------
 *
 */

/*/
 * === Attribute declaration
 *
 * Attributes are variables that control the behavior of the component. They
 * are stored in the `ids` and they can be changed dynamically by the codels
 * during execution. Genom provides a way to set and get attributes through the
 * chosen middleware.
 *
 * An "attribute" declaration creates a service that can either set or get one
 * or several attibutes.
 *
 * ****
 * [horizontal]
 * <dotgen-rule-attribute.adoc
 * <dotgen-rule-attribute-parameters.adoc
 * <dotgen-rule-attribute-parameter.adoc
 * <dotgen-rule-opt-properties.adoc
 * <dotgen-rule-properties.adoc
 * ****
 *
 * Attributes can have a number of properties:
 * ****
 * [horizontal]
 * <dotgen-rule-service-property.adoc
 * ****
 *
 * +doc+:: A string that describes the functionality of the service.
 *
 * +validate+:: A "validate" codel can be declared to check the validity of
 *   input data and throw an exception in case of a problem. The `validate`
 *   codel is called before updating the value in the `ids`. The codel can
 *   throw an exception if the new value is not acceptable. In this case, the
 *   value of the corresponding `ids` member is not changed. If the codel
 *   returns no error, the value of the `ids` member is updated.
 *
 * === Example
 *
 * This would declare an attribute for changing the speed of a moving thing:
 *
 * [source,C]
 * ----
 * component foo {
 *   ids { double velocity };
 *
 *   exception invalid_speed;
 *
 *   attribute SetSpeed(in velocity = 0.0: "Thing velocity")
 *   {
 *     doc "Change the velocity of a moving thing";
 *     validate check_velocity(local in velocity);
 *     throw invalid_speed;
 *   };
 * };
 * ----
 *
 * In this example `SetSpeed` is a setter for the `velocity` variable (`in`
 * keyword). It is possible to specify a default value ("= 0.0") and a small
 * documentation string that is used by interactive applications to guide the
 * user.
 *
 * This example is of course not functional. The `velocity` value should
 * be used by other codels and services as the reference value for actually
 * controlling the moving thing.
 */
attribute:
  ATTRIBUTE identifier '(' attribute_parameters ')' opt_properties semicolon
  {
    task_p = 0;
    param_setlocals(NULL);
    if (!$2 || !$4) {
      if ($2) parserror(@1, "dropped '%s' attribute", $2);
      if ($4) hash_destroy($4, 1);
      if ($6) hash_destroy($6, 1);
      break;
    }
    if (!service_create(@1, S_ATTRIBUTE, $2, $4, $6)) {
      parserror(@1, "dropped '%s' attribute", $2);
      hash_destroy($4, 1);
      if ($6) hash_destroy($6, 1);
    }
  }
;

/*/
 * === Function and activity declaration
 *
 * ****
 * [horizontal]
 * <dotgen-rule-service.adoc.adoc
 * <dotgen-rule-service-kind.adoc
 * <dotgen-rule-service-parameters.adoc
 * <dotgen-rule-service-parameter.adoc
 * <dotgen-rule-opt-properties.adoc
 * <dotgen-rule-properties.adoc
 * <dotgen-rule-property.adoc
 *
 * <dotgen-rule-service-property.adoc
 * ****
 */

service:
  service_kind identifier '(' service_parameters ')' opt_properties semicolon
  {
    task_p = 0;
    param_setlocals(NULL);
    if (!$2 || !$4) {
      if ($2) parserror(@1, "dropped '%s' %s", $2, service_strkind($1));
      if ($4) hash_destroy($4, 1);
      if ($6) hash_destroy($6, 1);
      break;
    }
    if (!service_create(@1, $1, $2, $4, $6)) {
      parserror(@1, "dropped '%s' %s", $2, service_strkind($1));
      hash_destroy($4, 1);
      if ($6) hash_destroy($6, 1);
    }
  }
;

service_kind:
  FUNCTION	{ $$ = S_FUNCTION; }
  | ACTIVITY	{ $$ = S_ACTIVITY; }
;

service_property:
  TASK_P identifier semicolon
  {
    if (!$2) { parserror(@1, "dropped '%s' property", $1); $$ = NULL; break; }
    $$ = prop_newtask(@1, $2);
  }
  | INTERRUPTS identifier_list semicolon
  {
    if (!$2) { parserror(@1, "dropped '%s' property", $1); $$ = NULL; break; }
    $$ = prop_newhash(@1, PROP_INTERRUPTS, $2);
  }
  | BEFORE identifier_list semicolon
  {
    if (!$2) { parserror(@1, "dropped '%s' property", $1); $$ = NULL; break; }
    $$ = prop_newhash(@1, PROP_BEFORE, $2);
  }
  | AFTER identifier_list semicolon
  {
    if (!$2) { parserror(@1, "dropped '%s' property", $1); $$ = NULL; break; }
    $$ = prop_newhash(@1, PROP_AFTER, $2);
  }
  | VALIDATE codel semicolon
  {
    $$ = $2 ? prop_newcodel(@1, PROP_VALIDATE, $2) : NULL;
  }
  | LOCAL local_variables semicolon
  {
    $$ = NULL; /* parameters were added to param_locals() */
  }
;

/*/
 * === Function and activity parameters
 *
 * ****
 * [horizontal]
 * <dotgen-rule-attribute-parameter.adoc
 * <dotgen-rule-service-parameter.adoc
 *
 * <dotgen-rule-parameter-dir.adoc
 * <dotgen-rule-parameter-variable.adoc
 * <dotgen-rule-opt-initializer.adoc
 * <dotgen-rule-initializers.adoc
 * <dotgen-rule-initializer.adoc
 * <dotgen-rule-initializer-value.adoc
 * ****
 */

attribute_parameters:
  /* empty */
  {
    task_p = 1; $$ = hash_create("parameter list", 0); param_setlocals($$);
  }
  | attribute_parameter
  {
    task_p = 1; $$ = param_locals(); if (!$1) break;
    if (!$$) {
      $$ = hash_create("parameter list", 3); param_setlocals($$);
      if (!$$) break;
    }
    if (hash_insert($$, param_name($1), $1, (hrelease_f)param_destroy)) {
      param_destroy($1); break;
    }
  }
  | attribute_parameters ',' attribute_parameter
  {
    $$ = param_locals(); if (!$3) break;
    if (!$$) {
      $$ = hash_create("parameter list", 3); param_setlocals($$);
      if (!$$) break;
    }
    switch(hash_insert($$, param_name($3), $3, (hrelease_f)param_destroy)) {
      case 0: break;
      case EEXIST:
	parserror(@3, "duplicate parameter '%s'", param_name($3));
	/*FALLTHROUGH*/
      default: param_destroy($3); break;
    }
  }
;

attribute_parameter:
  parameter_dir parameter_variable opt_initializer
  {
    if (!$2) { $$ = NULL; break; }
    $$ = param_newids(@2, $1, NULL, $2, $3);
  }
  | parameter_dir parameter_variable COLONCOLON identifier opt_initializer
  {
    if (!$2) { $$ = NULL; break; }
    $$ = param_newids(@2, $1, $4, $2, $5);
  }
  | parameter_dir COLONCOLON identifier opt_initializer
  {
    if (!$2) { $$ = NULL; break; }
    $$ = param_newids(@2, $1, $3, NULL, $4);
  }
  | error { $$ = NULL; }
;

service_parameters:
  /* empty */
  {
    task_p = 1; $$ = hash_create("parameter list", 0); param_setlocals($$);
  }
  | service_parameter
  {
    task_p = 1; $$ = param_locals(); if (!$1) break;
    if (!$$) {
      $$ = hash_create("parameter list", 3); param_setlocals($$);
      if (!$$) break;
    }
    if (hash_insert($$, param_name($1), $1, (hrelease_f)param_destroy)) {
      param_destroy($1); break;
    }
  }
  | service_parameters ',' service_parameter
  {
    $$ = param_locals(); if (!$3) break;
    if (!$$) {
      $$ = hash_create("parameter list", 3); param_setlocals($$);
      if (!$$) break;
    }
    switch(hash_insert($$, param_name($3), $3, (hrelease_f)param_destroy)) {
      case 0: break;
      case EEXIST:
	parserror(@3, "duplicate parameter '%s'", param_name($3));
	/*FALLTHROUGH*/
      default: param_destroy($3); break;
    }
  }
;

service_parameter:
  parameter_dir type_spec declarator opt_initializer
  {
    clist_s m;
    cval n;
    if (!$2 || !$3) { $$ = NULL; break; }
    if (!dcl_settype($3, $2)) { $$ = NULL; break; }
    n.k = CST_STRING;
    n.s = dcl_name($3);
    m = clist_append(NULL, n, 0);
    $$ = param_newlocal(@3, $1, NULL, m, dcl_type($3), $4);
  }
  | error { $$ = NULL; }
;

local_variables:
  type_spec declarator
  {
    param_s p;
    clist_s m;
    hash_s h;
    cval n;

    if (!task_p) {
      parserror(@1, "local variables may not be defined outside services");
      $$ = NULL;
      break;
    }
    if (!$1 || !$2) { $$ = NULL; break; }
    if (!dcl_settype($2, $1)) { $$ = NULL; break; }
    $$ = $1;

    n.k = CST_STRING;
    n.s = dcl_name($2);
    m = clist_append(NULL, n, 0);
    p = param_newlocal(@2, P_NODIR, NULL, m, dcl_type($2), NULL);
    if (!p) break;

    h = param_locals();
    if (!h) {
      h = hash_create("parameter list", 3); param_setlocals(h);
      if (!h) { param_destroy(p); break; }
    }
    switch(hash_insert(h, param_name(p), p, (hrelease_f)param_destroy)) {
      case 0: break;
      case EEXIST:
	parserror(@2, "duplicate parameter '%s'", param_name(p));
	/*FALLTHROUGH*/
      default: param_destroy(p); break;
    }
  }
  | local_variables ',' declarator
  {
    $$ = $1; if (!$1 || !$3) break;
    if (dcl_settype($3, $1)) {
      param_s p;
      clist_s m;
      hash_s h;
      cval n;

      n.k = CST_STRING;
      n.s = dcl_name($3);
      m = clist_append(NULL, n, 0);
      p = param_newlocal(@3, P_NODIR, NULL, m, dcl_type($3), NULL);
      if (!p) break;

      h = param_locals();
      if (!h) { param_destroy(p); break; }
      switch(hash_insert(h, param_name(p), p, (hrelease_f)param_destroy)) {
        case 0: break;
        case EEXIST:
          parserror(@2, "duplicate parameter '%s'", param_name(p));
          /*FALLTHROUGH*/
        default: param_destroy(p); break;
      }
    }
  }
;
