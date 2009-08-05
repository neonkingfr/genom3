<?
def parseInput(inputType, name):
  if inputType.kind() == IdlType.Array: 
    a = inputType.asArrayType()
    loopName = counter_name(name)
    ?>
   {
      int <!loopName!>[<!len(a.bounds())!>];
<?
    loopSubscript = ""
    for i in range(len(a.bounds())): 
      loopSubscript += "[" + loopName + "[" + str(i) +"]]"
      ?>
      for(<!loopName!>[<!i!>]=0; <!loopName!>[<!i!>] < <!a.bounds()[i]!>; <!loopName!>[<!i!>]++) {
<?
    flatListArray = flat_struct(a.type(), name + loopSubscript, ".")
    for x in flatListArray:
      parseInput(x[0], x[1])
    ?>
<?
    for i in range(len(a.bounds())): ?>
      }
<?
    ?>
   }
<?
    return

  if inputType.kind() == IdlType.Sequence: 
    a = inputType.asSequenceType()
    ?>
   {
      int loop1[1];
      for(loop1[0]=0; loop1[0] < <!a.bound()!>; loop1[0]++) {
<?
    flatListArray = flat_struct(a.seqType(), name + ".data[loop1[0]]", ".")
    for x in flatListArray:
      parseInput(x[0], x[1])
    ?>
      }
   }
<?
    return

  if inputType.kind() == IdlType.Short: ?>
  {
    int tmpInt;
    ret = Tcl_GetIntFromObj(interp, objv[++curObjc], &tmpInt);
    <!name!> = (short)tmpInt;
  }
<?
  elif inputType.kind() == IdlType.Long: ?>
    ret = Tcl_GetLongFromObj(interp, objv[++curObjc], &<!name!>);
<?
  elif inputType.kind() == IdlType.LongLong: ?>
    ret = Tcl_GetWideIntFromObj(interp, objv[++curObjc], &<!name!>);
<?
  elif inputType.kind() == IdlType.Char: ?>
  {
    int tmpInt;
    ret = Tcl_GetIntFromObj(interp, objv[++curObjc], &tmpInt);
    <!name!> = (char)tmpInt;
  }
<?
  elif inputType.kind() == IdlType.String: ?>
    strncpy(<!name!>, Tcl_GetStringFromObj(objv[++curObjc], NULL), <!inputType.asStringType().bound()!>);
    ret = TCL_OK;
<?
  elif inputType.kind() == IdlType.Float: ?>
  {
    double tmpDouble;
    ret = Tcl_GetDoubleFromObj(interp, objv[++curObjc], &tmpDouble);
    <!name!> = (float)tmpDouble;
  }
<?
  elif inputType.kind() == IdlType.Double: ?>
    ret = Tcl_GetDoubleFromObj(interp, objv[++curObjc], &<!name!>);
<?
  elif inputType.kind() == IdlType.Enum: 
      e = inputType.asEnumType()
      enumList = ""
      i = 0
      for en in e.enumerators():
	enumList += ", \"" + en + "\", " + str(i)
	i += 1
      ?>
    ret = Tcl_GetEnumFromObj(interp, objv[++curObjc], (int *)&<!name!> <!enumList!>, NULL);
<?
  ?>
  if (ret != TCL_OK) return TCL_ERROR;
<?


def processOutput(type, name, allocFlag = False):
  if type.kind() == IdlType.Array: 
    a = type.asArrayType()
    loopName = counter_name(name)
    ?>
   {
      int <!loopName!>[<!len(a.bounds())!>];
<?
    loopSubscript = ""
    for i in range(len(a.bounds())): 
      loopSubscript += "[" + loopName + "[" + str(i) +"]]"
      ?>
      for(<!loopName!>[<!i!>]=0; <!loopName!>[<!i!>] < <!a.bounds()[i]!>; <!loopName!>[<!i!>]++) {
<?
    flatListArray = flat_struct(a.type(), name + loopSubscript, ".")
    for x in flatListArray:
      processOutput(x[0], x[1], allocFlag)
    ?>
<?
    for i in range(len(a.bounds())): ?>
      }
<?
    ?>
   }
<?
    return

  if type.kind() == IdlType.Sequence: 
    a = type.asSequenceType()
    ?>
   {
      int loop1[1];
      for(loop1[0]=0; loop1[0] < <!a.bound()!>; loop1[0]++) {
<?
    flatListArray = flat_struct(a.seqType(), name + ".data[loop1[0]]", ".")
    for x in flatListArray:
      processOutput(x[0], x[1], allocFlag)
    ?>
      }
   }
<?
    return
  ?>

    ret = Tcl_ListObjAppendElement(interp, my_own_private_unique_result, <?
  if type.kind() == IdlType.Short or type.kind() == IdlType.Long or type.kind() == IdlType.Char: ?>
		Tcl_NewIntObj(<!name!>));
<?
  elif type.kind() == IdlType.UShort or type.kind() == IdlType.ULong or type.kind() == IdlType.Char: ?>
		Tcl_NewIntObj(<!name!>));
<?
  elif type.kind() == IdlType.LongLong or type.kind() == IdlType.ULongLong: ?>
		Tcl_NewWideIntObj(<!name!>));
<?
  elif type.kind() == IdlType.String: ?>
		Tcl_NewStringObj(<!name!>, -1));
<?
  elif type.kind() == IdlType.Double or type.kind() == IdlType.Float: ?>
		Tcl_NewDoubleObj(<!name!>));
<?
  elif type.kind() == IdlType.Enum: 
      e = type.asEnumType()
      enumList = ""
      i = 0
      for en in e.enumerators():
	enumList += ", \"" + en + "\", " + str(i)
	i += 1
      ?>
		Tcl_NewEnumObj(interp, (int)<!name!> <!enumList!>, NULL));
<?
  if allocFlag: ?>
    if (ret != TCL_OK) {free(_posterData); return TCL_ERROR;}
<?
  else: ?>
    if (ret != TCL_OK) return TCL_ERROR;
<?

?>
/* 
 * Copyright (c) 1993-2005 LAAS/CNRS
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

/**
 ** This file was automagically generated by GenoM.
 ** Manual editing not recommended. Changes will be overridden.
 **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tcl.h>

#include <portLib.h>
#include <errnoLib.h>
#include <h2errorLib.h>

#include <genom/tclServ.h>
#include <genom/tcl_cs.h>

#include "<!comp.name()!>MsgLib.h"
#include "<!comp.name()!>PosterLib.h"

int <!comp.name()!>_Init(Tcl_Interp *);

struct ModuleInfo {
   char *name;
   CLIENT_ID cid;
};

static int <!comp.name()!>Install(Tcl_Interp *interp,
			   struct ModuleInfo *, int install);
static int <!comp.name()!>ClientInitCb(ClientData clientData, Tcl_Interp *interp,
				int objc, Tcl_Obj *const objv[]);
static int <!comp.name()!>ClientEndCb(ClientData clientData, Tcl_Interp *interp,
			       int objc, Tcl_Obj *const objv[]);


#define TEST_BAD_USAGE(b)					\
{								\
   if (b) {							\
      Tcl_SetResult(interp, "wrong # args", TCL_STATIC);	\
      return TCL_ERROR;						\
   }								\
}

/*
 * ----------------------------------------------------------------------
 * <!comp.name()!> connection / disconnection
 */

static int
<!comp.name()!>ClientInitCb(ClientData clientData, Tcl_Interp *interp,
		     int objc, Tcl_Obj *const objv[])
{
   char *mboxName;
   struct ModuleInfo *m;
   STATUS status;
   char strerr[64];

   <!comp.name()!>RecordH2errMsgs();

   m = malloc(sizeof(struct ModuleInfo));
   if (m == NULL) {
      Tcl_SetResult(interp, "Not enough memory", TCL_STATIC);
      return TCL_ERROR;
   }

   if (objc > 1) {
      mboxName = Tcl_GetStringFromObj(objv[1], NULL);
      if (mboxName == NULL) return TCL_ERROR;
   } else {
      mboxName = <!upper(comp.name())!>_MBOX_NAME;
   }

   m->name = malloc(strlen(mboxName)+1);
   if (m->name == NULL) {
      free(m);
      Tcl_SetResult(interp, "Not enough memory", TCL_STATIC);
      return TCL_ERROR;
   }
   strcpy(m->name, mboxName);

   status = csClientInit(mboxName, <!upper(comp.name())!>_MAX_RQST_SIZE,
			 <!upper(comp.name())!>_MAX_INTERMED_REPLY_SIZE, 
			 <!upper(comp.name())!>_MAX_REPLY_SIZE, 
			 &m->cid);

   if (status == ERROR) {
      Tcl_SetResult(interp, h2getErrMsg(errnoGet(), strerr, 64), TCL_VOLATILE);
      Tcl_AppendResult(interp, ": \"", mboxName, "\"", NULL);
      return TCL_ERROR;
   }

   Tcl_SetResult(interp, "connected", TCL_STATIC);
   return <!comp.name()!>Install(interp, m, 1);
}

static int
<!comp.name()!>ClientEndCb(ClientData clientData, Tcl_Interp *interp,
		    int objc, Tcl_Obj *const objv[])
{
   struct ModuleInfo *m = (struct ModuleInfo *)clientData;
   char strerr[64];

   if (csClientEnd(m->cid) == ERROR) {
      Tcl_SetResult(interp, h2getErrMsg(errnoGet(), strerr, 64), TCL_VOLATILE);
      return TCL_ERROR;
   }

   <!comp.name()!>Install(interp, m, 0);
   free(m->name);
   free(m);

   Tcl_SetResult(interp, "disconnected", TCL_STATIC);
   return TCL_OK;
}


/*
 * ----------------------------------------------------------------------
 * <!comp.name()!> abort request
 */

static int <!comp.name()!>AbortRqstSendCb(ClientData, Tcl_Interp *, int, Tcl_Obj *const []); 
static int <!comp.name()!>AbortReplyRcvCb(ClientData, Tcl_Interp *, int, Tcl_Obj * const []);

static int
<!comp.name()!>AbortRqstSendCb(ClientData data, Tcl_Interp *interp,
			int objc, Tcl_Obj *const objv[])
{
   struct ModuleInfo *m = (struct ModuleInfo *)data;
   int rqstId, bilan;
   char buf[10];
   int activity;	/* input */
   char strerr[64];

   TEST_BAD_USAGE(objc != 2);
		 
   if (Tcl_GetIntFromObj(interp, objv[1], &activity) != TCL_OK)
      return TCL_ERROR;

   if (tclServRqstSend(interp, Tcl_GetStringFromObj(objv[0], NULL),
		       m->cid, <!upper(comp.name())!>_ABORT_RQST, 
		       &activity, sizeof(int),
		       &rqstId, &bilan) == ERROR) {

      Tcl_SetResult(interp, h2getErrMsg(bilan, strerr, 64), TCL_VOLATILE);
      return TCL_ERROR;
   }
    
   sprintf(buf, " %d", rqstId);
   Tcl_SetResult(interp, "SENT", TCL_STATIC);
   Tcl_AppendResult(interp, buf, NULL);
   return TCL_OK;
}

static int
<!comp.name()!>AbortReplyRcvCb(ClientData clientData, Tcl_Interp *interp,
			int objc, Tcl_Obj * const objv[])
{
   struct ModuleInfo *m = (struct ModuleInfo *)clientData;
   int ret;
   int rqstId, bilan, activity, status;
   Tcl_Obj *result;
   char strerr[64];

   TEST_BAD_USAGE(
      objc != 2 ||
      Tcl_GetIntFromObj(interp, objv[1], &rqstId) != TCL_OK
   );

   status = tclServReplyRcv(interp, m->cid, rqstId, NO_BLOCK,
			    &activity, NULL, 0, &bilan);

   switch (status) {
      case WAITING_INTERMED_REPLY:
	 Tcl_SetResult(interp, "SENT", TCL_STATIC);
	 return TCL_OK;

      case WAITING_FINAL_REPLY: {
	 char tmp[50];
	 sprintf(tmp, "ACK %d", activity);
	 Tcl_SetResult(interp, tmp, TCL_VOLATILE);
	 return TCL_OK;
      }

      case FINAL_REPLY_OK:
	 break;

      default:
	 Tcl_SetResult(interp, h2getErrMsg(bilan, strerr, 64), TCL_VOLATILE);
	 return TCL_ERROR;
   }

   result = Tcl_NewListObj(0, NULL);
   if (result == NULL) return TCL_ERROR;

   ret = Tcl_ListObjAppendElement(interp, result,
				  Tcl_NewStringObj("TERM", -1));
   if (ret != TCL_OK) return TCL_ERROR;

   if (bilan == OK)
      ret = Tcl_ListObjAppendElement(interp, result,
				     Tcl_NewStringObj("OK", -1));
   else
      ret = Tcl_ListObjAppendElement(interp, result,
				     Tcl_NewStringObj(
					h2getErrMsg(bilan, strerr, 64), -1));
   if (ret != TCL_OK) return TCL_ERROR;

   Tcl_SetObjResult(interp, result);
   return TCL_OK;
}


/*
 * ----------------------------------------------------------------------
 * Requests
 */
<?
serviceNum = 0
for name, service in servicesDict.iteritems():
  serviceInfo = services_info_dict[name]
  ?>
/*
 * ----------------------------------------------------------------------
 * <!service.name!> Request
 */

static int <!comp.name()!><!service.name!>RqstSendCb(ClientData, Tcl_Interp *, int, Tcl_Obj *const []); 
static int <!comp.name()!><!service.name!>ReplyRcvCb(ClientData, Tcl_Interp *, int, Tcl_Obj * const []);

static int
<!comp.name()!><!service.name!>RqstSendCb(ClientData data, Tcl_Interp *interp,
				int objc, Tcl_Obj *const objv[])
{
   struct ModuleInfo *tclModuleInfo = (struct ModuleInfo *)data;
   int rqstId, bilan;
   char buf[10];
<?
  if serviceInfo.inputFlag:
    print "   static " + serviceInfo.inputVarDecl + ";"
# #if ($argNumber$ > 0)
  if serviceInfo.inputFlatList:
    ?>
   int ret;
   int curObjc = 0;
<? # endif
  ?>
   char strerr[64];

   TEST_BAD_USAGE(objc != <!len(serviceInfo.inputFlatList)!>+1);
		 
<? #$parseInput$
  if serviceInfo.inputFlag:
    for x in serviceInfo.inputFlatList:
      parseInput(x[0], x[1])
  ?>

   if (tclServRqstSend(interp, Tcl_GetStringFromObj(objv[0], NULL),
		       tclModuleInfo->cid, <!serviceNum!>,
		       <!serviceInfo.inputNamePtr!>, <!serviceInfo.inputSize!>,
		       &rqstId, &bilan) == ERROR) {

     Tcl_SetResult(interp, h2getErrMsg(bilan, strerr, 64), TCL_VOLATILE);
      return TCL_ERROR;
   }
    
   sprintf(buf, " %d", rqstId);
   Tcl_SetResult(interp, "SENT", TCL_STATIC);
   Tcl_AppendResult(interp, buf, NULL);
   return TCL_OK;
}

static int
<!comp.name()!><!service.name!>ReplyRcvCb(ClientData data, Tcl_Interp *interp,
				int objc, Tcl_Obj * const objv[])
{
   struct ModuleInfo *tclModuleInfo = (struct ModuleInfo *)data;
   int ret;
   int rqstId, bilan, activity;
<?
  if serviceInfo.outputFlag:
    print "   static " + serviceInfo.outputVarDecl + ";"
  ?>
   Tcl_Obj *my_own_private_unique_result;
   char strerr[64];

   TEST_BAD_USAGE(
      objc != 2 ||
      Tcl_GetIntFromObj(interp, objv[1], &rqstId) != TCL_OK
   );

   switch (
      /* c'est laid */
      tclServReplyRcv(interp,
		      tclModuleInfo->cid, rqstId, NO_BLOCK,
		      &activity, <!serviceInfo.outputNamePtr!>, <!serviceInfo.outputSize!>,
		      &bilan)
      ) {
      case WAITING_INTERMED_REPLY:
	 Tcl_SetResult(interp, "SENT", TCL_STATIC);
	 return TCL_OK;

      case WAITING_FINAL_REPLY: {
	 char tmp[50];
	 sprintf(tmp, "ACK %d", activity);
	 Tcl_SetResult(interp, tmp, TCL_VOLATILE);
	 return TCL_OK;
      }

      case FINAL_REPLY_OK:
	 break;

      default:
	 Tcl_SetResult(interp, h2getErrMsg(bilan, strerr, 64), TCL_VOLATILE);
	 return TCL_ERROR;
   }

   my_own_private_unique_result = Tcl_NewListObj(0, NULL);
   if (my_own_private_unique_result == NULL) return TCL_ERROR;

   ret = Tcl_ListObjAppendElement(interp, my_own_private_unique_result,
				  Tcl_NewStringObj("TERM", -1));
   if (ret != TCL_OK) return TCL_ERROR;

   if (bilan == OK)
      ret = Tcl_ListObjAppendElement(interp, my_own_private_unique_result,
				     Tcl_NewStringObj("OK", -1));
   else
      ret = Tcl_ListObjAppendElement(interp, my_own_private_unique_result,
				     Tcl_NewStringObj(
					h2getErrMsg(bilan, strerr, 64), -1));
   if (ret != TCL_OK) return TCL_ERROR;
<?
  if serviceInfo.outputFlag:
    for x in serviceInfo.outputFlatList:
      processOutput(x[0], x[1])
  ?>
   Tcl_SetObjResult(interp, my_own_private_unique_result);
   return TCL_OK;
}
<?
?>

<?
for port in outports:
  if is_dynamic_port(port):
      t = dynamic_port_type(port)
  else:
      t = port.idlType
  ?>
/*
 * ----------------------------------------------------------------------
 * <!port.name!> Poster
 */

static int <!comp.name()!><!port.name!>PosterTclRead(ClientData, Tcl_Interp *, int, Tcl_Obj *const []); 

static int
<!comp.name()!><!port.name!>PosterTclRead(ClientData data, Tcl_Interp *interp,
			 int objc, Tcl_Obj *const objv[])
{
   int ret;
   static <!MapTypeToC(t, True)!> *_posterData;	/* data */
   Tcl_Obj *my_own_private_unique_result;
   char strerr[64];

   TEST_BAD_USAGE(objc != 1);
		 
  if ((_posterData = malloc(sizeof(<!MapTypeToC(t, True)!>))) == NULL) {
      Tcl_SetResult(interp, h2getErrMsg(errnoGet(), strerr, 64), TCL_VOLATILE);
      return TCL_ERROR;
  }

   if (<!comp.name()!><!port.name!>PosterRead(_posterData) != OK) {
      Tcl_SetResult(interp, h2getErrMsg(errnoGet(), strerr, 64), TCL_VOLATILE);
      free(_posterData);
      return TCL_ERROR;
   }

   my_own_private_unique_result = Tcl_NewListObj(0, NULL);
   if (my_own_private_unique_result == NULL) {
     free(_posterData);
     return TCL_ERROR;
   }

<?
  portFlatList = flat_struct(t, "(*_posterData)", ".") 
  for x in portFlatList:
    processOutput(x[0], x[1], True)
  ?>

   Tcl_SetObjResult(interp, my_own_private_unique_result);
   free(_posterData);
   return TCL_OK;
}

<?
?>



int
<!comp.name()!>_Init(Tcl_Interp *interp)
{
   char buf[20];
   int code;

   code = Tcl_PkgProvide(interp, "<!comp.name()!>", "1.0");
   if (code != TCL_OK) return code;

   Tcl_CreateObjCommand(interp, "::<!comp.name()!>::connect",
			<!comp.name()!>ClientInitCb, NULL, NULL);

   sprintf(buf, "%d", (int)<!upper(comp.name())!>_CLIENT_MBOX_REPLY_SIZE);
   Tcl_SetVar(interp, "::<!comp.name()!>::mboxSize", buf, TCL_GLOBAL_ONLY);

   Tcl_SetVar(interp, "::<!comp.name()!>::datadir", DATADIR, TCL_GLOBAL_ONLY);

   return TCL_OK;
}

static int
<!comp.name()!>Install(Tcl_Interp *interp, struct ModuleInfo *m, int install)
{
   int offset;
   char name[<!max_service_name_length() + 50!>];

   strcpy(name, "::");
   strcat(name, m->name);
   strcat(name, "::");
   offset = strlen(name);

   if (install) {
      strcpy(name+offset, "disconnect");
      Tcl_CreateObjCommand(interp, name, <!comp.name()!>ClientEndCb, m, NULL);

      strcpy(name+offset, "AbortSend");
      Tcl_CreateObjCommand(interp, name, <!comp.name()!>AbortRqstSendCb, m, NULL);
      strcpy(name+offset, "AbortRcv");
      Tcl_CreateObjCommand(interp, name, <!comp.name()!>AbortReplyRcvCb, m, NULL);
<? # $createCommands$
for name, service in servicesDict.iteritems():
  ?>
      strcpy(name+offset, "<!service.name!>Send");
      Tcl_CreateObjCommand(interp, name, <!comp.name()!><!service.name!>RqstSendCb, m, NULL);
      strcpy(name+offset, "<!service.name!>Rcv");
      Tcl_CreateObjCommand(interp, name, <!comp.name()!><!service.name!>ReplyRcvCb, m, NULL);
<?
for port in outports:
  ?>
      strcpy(name+offset, "<!port.name!>PosterRead");
      Tcl_CreateObjCommand(interp, name, <!comp.name()!><!port.name!>PosterTclRead, m, NULL);
<?
?>
   } else {
      strcpy(name+offset, "disconnect");
      Tcl_DeleteCommand(interp, name);

      strcpy(name+offset, "AbortSend");
      Tcl_DeleteCommand(interp, name);
      strcpy(name+offset, "AbortRcv");
      Tcl_DeleteCommand(interp, name);
<? # $deleteCommands$
for name, service in servicesDict.iteritems():
  ?>
      strcpy(name+offset, "<!service.name!>Send");
      Tcl_DeleteCommand(interp, name);
      strcpy(name+offset, "<!service.name!>Rcv");
      Tcl_DeleteCommand(interp, name);
<?
for p in outports:
  ?>
      strcpy(name+offset, "<!port.name!>PosterRead");
      Tcl_DeleteCommand(interp, name);
<?
?>
   }

   return TCL_OK;
}




