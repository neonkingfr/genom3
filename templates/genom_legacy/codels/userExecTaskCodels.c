<?
currentTask = comp.task(currentTaskName) # currentTaskName is set by genom
?>
/* 
 * Copyright (c) 1993-2003 LAAS/CNRS
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

#include "<!comp.name()!>Struct.h"
#include "userCodels.h"

<?
for name, service in servicesDict.iteritems():
    if service.taskName != currentTaskName or service.type == ServiceType.Control:
	continue

    for c in service.codels():
	if c.key() == "control":
	    continue
	codel = c.data()
	?>
/*------------------------------------------------------------------------
 * <!codel.name!>  -  codel <!c.key()!> request <!service.name!>
 *
 * Description:    
<?
	for m in service.errorMessages():
	    print " * ERROR_" + m
	?> *         
 *
 * Returns:    USER_OK or ERROR
 */
<!real_codel_signature(codel)!>
{

}

<?
if currentTask.hasCodel("init"):
    ?>
/*------------------------------------------------------------------------
 *
 * <!currentTask.codel("init").name!>  --  Initialization codel (fIDS, ...)
 *
 * Description: 
 * 
 * Returns:    USER_OK or ERROR
 */

<!real_codel_signature(currentTask.codel("init"))!>
{

}
<?
?>

