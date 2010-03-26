#
# Copyright (c) 2010 LAAS/CNRS
# All rights reserved.
#
# Redistribution  and  use  in  source  and binary  forms,  with  or  without
# modification, are permitted provided that the following conditions are met:
#
#   1. Redistributions of  source  code must retain the  above copyright
#      notice and this list of conditions.
#   2. Redistributions in binary form must reproduce the above copyright
#      notice and  this list of  conditions in the  documentation and/or
#      other materials provided with the distribution.
#
# THE SOFTWARE  IS PROVIDED "AS IS"  AND THE AUTHOR  DISCLAIMS ALL WARRANTIES
# WITH  REGARD   TO  THIS  SOFTWARE  INCLUDING  ALL   IMPLIED  WARRANTIES  OF
# MERCHANTABILITY AND  FITNESS.  IN NO EVENT  SHALL THE AUTHOR  BE LIABLE FOR
# ANY  SPECIAL, DIRECT,  INDIRECT, OR  CONSEQUENTIAL DAMAGES  OR  ANY DAMAGES
# WHATSOEVER  RESULTING FROM  LOSS OF  USE, DATA  OR PROFITS,  WHETHER  IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR  OTHER TORTIOUS ACTION, ARISING OUT OF OR
# IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#
#                                           Anthony Mallet on Fri Jan  8 2010
#
package require Tcl 8.5

namespace eval engine {

    # make stdout unfiltered
    variable verbose		off

    # debug mode
    variable debug		[dotgen genom debug]

    # overwrite existing files
    variable overwrite		off

    # move-if-change files
    variable move-if-change	on

    # available engine modes
    variable modes {verbose overwrite move-if-change debug}

    # default output directory
    variable outdir	{}

    # markups
    variable markup
    array set markup {
	open	{<(["'])}	close	{(['"])>}
	full	{<(["'])(.*?)(['"])>}
    }

    # interpreter evaluating templates substitutions. Objects from the .gen as
    # well as the commands 'dotgen', 'mapping' and 'language' are available,
    # (but not the template command that are reserved to the toplevel
    # template.tcl file) and puts is redefined to catch template output within
    # <' '> markups.
    interp create -safe slave
    slave hide puts
    slave alias puts slave invokehidden puts
    slave alias dotgen dotgen
    slave alias language language
    slave alias mapping mapping
    foreach o [info commands $::dotgen::ns(object)::*] {
	slave alias $o $o
    }


    # --- mode -------------------------------------------------------------

    # Set operation mode for files.
    #
    proc mode { args } {
	variable modes

	if {[llength $args] == 0} {
	    foreach m $modes {
		variable $m
		if {[set $m]} { lappend r $m } else { lappend r -$m }
	    }
	    return $r
	}

	foreach m $args {
	    switch -- [string index $m 0] {
		+	{ set v on;  set m [string replace $m 0 0] }
		-	{ set v off; set m [string replace $m 0 0] }
		default { set v on; }
	    }

	    if {[lsearch $modes $m] < 0} {
		template fatal "unknown mode $m: should be [join $modes {, }]"
	    }

	    variable $m
	    if {([set $m] && !$v) || (![set $m] && $v)} {
		set $m $v
		puts "$m mode $v"
	    }
	    switch -- $m {
		verbose { dotgen genom stdout [set $m] }
	    }
	}
    }
    namespace export mode


    # --- chdir ------------------------------------------------------------

    # Change default output directory.
    #
    proc chdir { d } {
	variable outdir
	if {$outdir != $d} {
	    set outdir $d
	    template message "set output directory to '$d'"
	}
    }
    namespace export chdir


    # --- pwd --------------------------------------------------------------

    # Return the current output directory.
    #
    proc pwd { } {
	variable outdir
	return $outdir
    }
    namespace export pwd


    # --- args -------------------------------------------------------------

    # Set argc/argv in slave interpreter and return previous value of argv.
    #
    proc args { l } {
	if {[catch {slave eval [list set argv]} prev]} {
	    set prev [list]
	}
	if {[catch {
	    slave eval [list set argc [llength $l]]
	    slave eval [list set argv $l]
	} m options]} {
	    return -options $options $m
	}

	return $prev
    }


    # --- open -------------------------------------------------------------

    # Open file or string (via a reflected channel) for reading or writing.
    # Return a tcl channel. Type must be 'file' or 'string', dst is either a
    # file name (file/read or write) a string (string/read) or a global
    # variable name (string/write).
    #
    proc open { type dst mode } {
	switch -glob -- $type|$mode {
	    string|read {
		set ::_tmp $dst
		return [chan create read [namespace code {strchan _tmp}]]
	    }
	    string|write {
		return [chan create write [namespace code [list strchan $dst]]]
	    }
	    file|read {
		set dst [file join [dotgen template dir] $dst]
		puts "reading $dst"
		return [::open $dst r]
	    }
	    file|write {
		# record a move on close
		variable moc
		variable outdir

		set t [mktemp]
		set c [::open $t w]
		dict set moc $c [list $t [file join $outdir $dst]]

		puts "generating $dst in $t"
		return $c
	    }
	}
	template fatal "invalid channel type $type: must be file or string"
    }


    # --- close ------------------------------------------------------------

    # Close channel. If a "move on close" was recorded by open, execute it.
    #
    variable moc [dict create]

    proc close { channel } {
	variable moc
	variable overwrite
	variable move-if-change

	::close $channel
	if {![dict exists $moc $channel]} return

	lassign [dict get $moc $channel] tmp dst
	dict unset moc $channel

	if {${move-if-change} && [file exists $dst]} {
	    set t [::open $tmp r]; set d [::open $dst r]
	    while { "[read $t 4096]" == "[read $d 4096]" } {
		if { [eof $t] && [eof $d] } {
		    close $t; close $d
		    template message "$dst is up-to-date"
		    return
		}
	    }
	    ::close $t; ::close $d
	}

	if {!$overwrite && [file exists $dst]} {
	    template fatal "file $dst would be overwritten"
	    return
	}

	if {[file exists $dst]} {
	    template message "overwriting $dst"
	} else {
	    template message "creating $dst"
	}
	file mkdir [file dirname $dst]
	file copy -force $tmp $dst
	return
    }


    # --- process ----------------------------------------------------------

    # Read template source file from 'in' channel and write parsing result to
    # 'out' channel.
    #
    # Text outside markers is replaced by a TCL program printing the text and
    # <''> and <""> are replaced by the appropriate TCL code (verbatim for <''>
    # and [subst] for <"">. A temporary program is built in a string and then
    # evaluated in the 'slave' interpreter.
    #
    # <''> always produce an empty string, but code inside is evaluated and
    # might invoke 'puts' to produce some output, while <""> is replaced by the
    # result of its evaluation by 'subst'.
    #
    proc process { src in out } {
	variable markup

	# initalize template program source code
	set linenum	1
	set code {}

	# read source and build program
	while { ![eof $in] } {
	    # read input by chunks of 4k, and avoid stopping on a tag limit
	    append raw [read $in 4096]
	    while { [string equal {<} [string index $raw end]] } {
		if [eof $in] break
		append raw [read $in 1]
	    }

	    # look for complete tags - x is the full match including markers, o
	    # and c are the opening and closing markers and t the enclosed
	    # text
	    while {[regexp $markup(full) $raw x o t c]} {

		# get match indices (x is overwritten)
		regexp -indices $markup(full) $raw x io it ic

		# flush raw data before opening tag, if any
		if {[lindex $x 0] > 0} {
		    set notag [string range $raw 0 [lindex $x 0]-1]
		    if [regexp -indices $markup(open) $notag l] {
			incr linenum [linecount $notag 0 [lindex $l 0]]
			error "$src:$linenum: missing closing tag"
		    }
		    if [regexp -indices $markup(close) $notag l] {
			incr linenum [linecount $notag 0 [lindex $l 0]]
			error "$src:$linenum: missing opening tag"
		    }

		    # output raw data
		    append code "puts -nonewline [quote $notag]\n"

		    # update current line number in the template file
		    incr linenum [linecount $notag]
		}

		# generate code to track source line number
		append code						\
		    "set ::__source__ \[list {$src} \[expr { "		\
		    " $linenum - \[dict get \[info frame 0\] line\] "	\
		    "} \]\]; "

		# generate tag program
		switch -- $o$c {
		    {''} { set s $t }
		    {""} { set s "puts -nonewline \[subst [quote $t]\]" }
		    default {
			if {[string equal $o $c]} {
			    error "$src:$linenum: unknown tag '$o'"
			} else {
			    error "$src:$linenum: unbalanced tags '$o' and '$c'"
			}
		    }
		}
		append code "$s\n"

		# update current line number in the template file
		incr linenum [linecount $t]

		# discard processed source text - if the character immediately
		# preceeding the opening <' tag or following the closing '> tag
		# is a \n, it is discarded. This is the intuitive expectation
		# of the template writer.
		if {$o == "'" && [string index $raw [lindex $x 0]-1] == "\n"} {
		    lset x 0 [expr [lindex $x 0] - 1]
		}
		if {$c == "'" && [string index $raw [lindex $x 1]+1] == "\n"} {
		    lset x 1 [expr [lindex $x 1] + 1]
		    incr linenum
		}
		set raw [string replace $raw 0 [lindex $x 1]]
	    }

	    # finished processing all tags in the current buffer, now check for
	    # orphaned closing tag (error)
	    if [regexp -indices $markup(close) $raw l] {
		incr linenum [linecount $raw 0 [lindex $l 0]]
		error "$src:$linenum: missing opening tag"
	    }

	    # incomplete opening tag: must read more text
	    if [regexp $markup(open) $raw] { continue }

	    # concatenate remaining raw output
	    append code "puts -nonewline [quote $raw]\n"
	    incr linenum [linecount $raw]
	    set raw {}
	}

	# finished processing all tags in the file, now check for orphaned tags
	# (error)
	if {[string length $raw] > 0} {
	    if [regexp -indices $markup(open) $raw l] {
		incr linenum [linecount $raw 0 [lindex $l 0]]
		error "$src:$linenum: missing closing tag"
	    }
	    if [regexp -indices $markup(close) $raw l] {
		incr linenum [linecount $raw 0 [lindex $l 0]]
		error "$src:$linenum: missing opening tag"
	    }
	}

	# dump program in debug mode
	variable debug
	if {$debug} {
	    set t [mktemp]
	    set c [::open $t w]
	    puts $c $code
	    close $c
	    puts "dumped template code for $src in $t"
	}

	# execute template program in slave interpreter
	slave alias puts [namespace code "slave-output $out"]
	set s [slave eval catch [list $code] m ctx]
	slave alias puts slave invokehidden puts
	if {$s} {
	    if {![catch { set s [slave eval set ::__source__] }]} {
		set line [lindex $s 1]
		catch {incr line [slave eval {dict get $ctx -errorline}]}
		lset s 1 $line

		variable verbose
		set m "[join $s :]: [slave eval {set m}]"
	    }
	    error $m
	}
	return
    }


    # --- slave-output -----------------------------------------------------

    # An alias for puts in 'slave' interpreter. Catches all invokations of puts
    # with no channel, that are redirected to var, otherwise call
    # standard puts.
    #
    proc slave-output { out args } {
	if {[lindex $args 0] == "-nonewline" } {
	    set args [lrange $args 1 end]
	    set nl ""
	} else {
	    set nl "\n"
	}
	if {[llength $args] != 1} {
	    set args [linsert $args 0 slave invokehidden puts]
	    return [eval $args]
	}

	puts -nonewline $out [lindex $args 0]${nl}
	return
    }


    # --- quote ------------------------------------------------------------

    # Return a self quoting string. The result is s enclosed in braces if s
    # contains no \, { or }. If s contains such characters, they are escaped
    # with a leading backlash and the result is an expression that performs the
    # opposite substitution.
    #
    proc quote { s } {
	if {[regsub -all {([\\{}])} $s {\\\1} s] == 0} {
	    return [format "{%s}" $s]
	}
	return [format {[regsub -all {\\([\\{}])} {%s} {\1}]} $s]
    }


    # --- linecount --------------------------------------------------------

    # Count the number of lines (\n) in the string, optinally starting from
    # index 'start' and stopping at index 'end'.
    #
    proc linecount { s { start 0 } { end end } } {
	return [regexp -all "\n" [string range $s $start $end]]
    }


    # --- mktemp -----------------------------------------------------------

    # Return a unique temporary file name
    #
    variable seed [expr int(rand()*9999)]
    proc mktemp { } {
	variable seed

	for { set r 1 } { 1 } { incr r } {
	    # 4 digits from pid, plus 4 random
	    set f [format %04d%04d [pid] [incr seed]]
	    set f [file join [dotgen template tmpdir] "template$f"]

	    if { ![file exists $f] } { break; }
	}
	if {$r > 1} {
	    puts "$r iterations required for finding a temporary file"
	}
	return $f
    }
    namespace export mktemp


    # --- strchan ----------------------------------------------------------

    # Reflected channel working on string global variables.
    #
    variable strchans [dict create]

    proc strchan { var cmd args } {
	variable strchans

	switch -- $cmd {
	    initialize {
		upvar #0 $var src
		lassign $args c m

		if {$m == "write" } { set src {} }

		dict set strchans $c [encoding convertto identity $src]
		return {initialize finalize watch read write}
	    }

	    finalize {
		upvar #0 $var src
		lassign $args c

		set src [encoding convertfrom identity [dict get $strchans $c]]
		dict unset strchans $c
	    }

	    watch {}

	    read {
		lassign $args c n
		set buffer [dict get $strchans $c]

		set r [string range $buffer 0 $n-1]
		dict set strchans $c [string replace $buffer 0 $n-1]
		return $r
	    }

	    write {
		lassign $args c d
		dict append strchans $c $d
		return [string bytelength $d]
	    }
	}
    }

    namespace ensemble create
}
