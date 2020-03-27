#!/usr/bin/perl -w
#
# This is shpdump.pl, a Perl CGI frontend to shpdump.
# Copyright (c) 2006 by Urs-Jakob Ruetschi.
#
# ujr/2006-04-11 started
# ujr/2007-07-20 added field for precision parameter

use strict;
use CGI;
#use CGI::Carp qw(fatalsToBrowser);

my $TITLE = "Shapefile Dumper";
my $VERSION = '2006-04-11';
my $CONTACT = 'uruetsch@geo.unizh.ch';
my $SHPDUMPURL = 'http://www.geo.unizh.ch/~uruetsch/software/shpdump.html';
my $SHPDUMPBIN = './shpdump';

my $cgi = new CGI;
my $script = $cgi->script_name();
my $shapefile = $cgi->param('shapefile');

if ($cgi->param('V')) {
  print "Content-type: text/plain\r\n\r\n";
  exec $SHPDUMPBIN "shpdump", "-V" or
  print "Couldn't execute $SHPDUMPBIN: $!\n";
  exit 0;
}

if ($shapefile) {
  $|=1; # make sure header arrives in time!
  print "Content-type: text/plain\r\n\r\n";

  my $options = "";
  $options .= " -g" if ($cgi->param('g') && !$cgi->param('h'));
  $options .= " -h" if ($cgi->param('h'));
  $options .= " -v" if ($cgi->param('v'));
  $options .= " -x" if ($cgi->param('x'));
  if ($cgi->param('prec')) {
    $options .= " -p " . $cgi->param('prec');
  }

  my $outname = "|$SHPDUMPBIN$options 2>&1";
  if (open DUMPER, "$outname") {
  	my $chunk; # pipe to shpdump in chunks
  	while (read $shapefile, $chunk, 1024) {
  		print DUMPER $chunk;
  	}
  	close DUMPER;
  	print "end\n" unless ($cgi->param('g'));
  }
  else {
  	print "Cannot open $outname: $!\n";
  }
  exit 0;
}

# There was no reasonable POST/GET input,
# so we better present the user interface:
print "Content-type: text/html\r\n\r\n";

print << "ENDHTML";
<html><head><title>$TITLE</title>
<style><!--
 body { margin: 2pc; font-family: sans-serif }
 pre { background-color: #cccccc }
--></style>
</head><body><!--auto-generated html-->
<h1>Dissect Your Shapefile!</h1>
ENDHTML

print << "ENDHTML";
<form action="$script" method="POST"
 enctype="multipart/form-data">
<p>Choose a shapefile (.shp) to dissect:<br>
   <input name="shapefile" type="file" size="50"></p>
<p><input name="g" type="checkbox" value="yes">
 Dump in Arc GENERATE format
<br><input name="h" type="checkbox" value="yes">
 Dump only the shapefile header
<br><input name="v" type="checkbox" value="yes">
 Dump extra information (verbose)
<br><input name="x" type="checkbox" value="yes">
 Report inconsistencies in shapefile
<br>Precision: <input name="prec" type="text" value="2" size="4">
 decimal digits in generated output
</p>
<!--<p><input name="V" type="checkbox" value="yes">
 Report shpdump backend version (ignore other options)</p>-->
<!--<p><input name="plain" type="checkbox" value="yes">
 Plain-text output (no HTML)</p>-->
<p><input type="submit" value="Submit">
 to upload and dissect your shapefile!</p>
</form>
ENDHTML

print << "ENDHTML";
<hr>
<p>Your shapefile will be uploaded and translated into
 a plain-text representation that you may use for further
 analysis. Use copy-paste or save the resulting page.
 Use the browser's &quot;back&quot; button to get back here.</p>
<p>The actual translation of a shapefile into plain-text is done
 by <a href="$SHPDUMPURL">shpdump</a>,
 which you may also use as a standalone program.</p>
<p>This web frontend and shpdump were created by
 <a href="mailto:uruetsch\@geo.unizh.ch">Urs-Jakob R&uuml;etschi</a>.
 Bug reports are most welcome.</p>
</body>
</html>
ENDHTML

exit 0;
