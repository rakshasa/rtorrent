#!/usr/bin/perl

# Perl script to add rTorrent fast resume data to torrent files.
#
# Usage:
# rtorrent_fast_resume.pl [base-directory] < plain.torrent > with_fast_resume.torrent
# -OR-
# rtorrent_fast_resume.pl [base-directory] plain.torrent [with_fast_resume.torrent]

use strict;
use warnings;
use Convert::Bencode_XS qw(bencode bdecode);
use File::Spec;  # core module
use POSIX;       # core module

$/ = undef;
$| = 1;

# Process ARGV
my $d = $ARGV[0];
if ($d and not -d $d) {
   if (-f $d and -s $d and not $ARGV[2]) {  # missing directory, but has file
      $ARGV[2] = $ARGV[1];
      $ARGV[1] = $ARGV[0];
      $d = '';
   }
   else { die "$d is not a directory\n"; }
}
$d ||= ".";
$d .= "/" unless $d =~ m#/$#;

my ($in, $out, $msg);
my ($in_file, $out_file) = ('', '');
if ($ARGV[1]) {
   $in_file = $ARGV[1];
   open($in, ($ARGV[2] ? '<' : '+<'), $in_file) || die "Cannot open $in_file for input!\n";
   unless ($ARGV[2]) {
      $out = $in;
      $out_file = $in_file;
   }
}
else { $in = *STDIN; }
if ($ARGV[2]) {
   $out_file = $ARGV[2];
   open($out, '>', $out_file) || die "Cannot open $out_file for output!\n";
   $msg = *STDOUT;
}
elsif (!$ARGV[1]) { $out = *STDOUT; }
$msg //= *STDERR;

print {$msg} "Total input torrent size: ".sprintf('%.2f', (-s $in_file) / 1024)." KB\n" if $in_file;
print {$msg} "Decoding ".($in_file || 'torrent from standard input')."...";
my $t = bdecode(scalar <$in>);

die "No info key.\n" unless ref $t eq "HASH" and exists $t->{info};
my $psize = $t->{info}{"piece length"} or die "No piece length key.\n";
print {$msg} "done\n\n";

my @files;
my $tsize = 0;
if (exists $t->{info}{files}) {
   print {$msg} "Multi file torrent: $t->{info}{name}\n";
   for (@{$t->{info}{files}}) {
      push @files, join "/", $t->{info}{name},@{$_->{path}};
      $tsize += $_->{length};
   }
}
else {
   print {$msg} "Single file torrent: $t->{info}{name}\n";
   @files = ($t->{info}{name});
   $tsize = $t->{info}{length};
}
my $chunks = int(($tsize + $psize - 1) / $psize);
print {$msg} "Total size: ".sprintf('%.2f', $tsize / 1024**2)." MB; $chunks chunks; ", scalar @files, " files.\n\n";

die "Inconsistent piece information!\n" if $chunks*20 != length $t->{info}{pieces};

print {$msg} "Adding fast resume information...";
#      flags    => 1+16+

my $pmod = 0;
$t->{libtorrent_resume}{bitfield} = $chunks;
foreach my $f (0..$#files) {
   die "$d$files[$f] not found.\n" unless -e "$d$files[$f]";
   my $mtime = (stat "$d$files[$f]")[9];
   
   # Compute number of chunks per file
   my $fsize   = (exists $t->{info}{files}) ? $t->{info}{files}[$f]{length} : 1;
   my $fchunks = ($pmod ? 1 : 0);
   if ($pmod >= $fsize) { ($fsize, $pmod ) = (0, $pmod-$fsize); }
   else                 { ($pmod,  $fsize) = (0, $fsize-$pmod); }
   $fchunks +=      ceil($fsize / $psize);
   $pmod   ||= $psize - ($fsize % $psize);
   
   $t->{libtorrent_resume}{files}[$f] = {
      priority  => 0,  # Don't download; we already have the file, so don't clobber it!
      mtime     => $mtime,
      completed => $fchunks,
   };
};
$t->{libtorrent_resume}{'uncertain_pieces.timestamp'} = time;

# Some extra information to re-enforce the fact that this is a finished torrent
if (exists $t->{info}{files}) {
   $d .=  $t->{info}{name};
}

$t->{rtorrent} = {
   state          => 1,  # started
   state_changed  => time,
   state_counter  => 1,
   chunks_wanted  => 0,
   chunks_done    => $chunks,
   complete       => 1,
   hashing        => 0,  # Not hashing
   directory      => File::Spec->file_name_is_absolute($d)        ? $d        : File::Spec->rel2abs($d),
   ((tied_to_file => File::Spec->file_name_is_absolute($out_file) ? $out_file : File::Spec->rel2abs($out_file)) x!! $out_file),
   'timestamp.finished' => 0,
   'timestamp.started'  => time,
};

print {$msg} "done\n";

print {$msg} "Encoding ".($out_file || 'torrent from standard output')."...";
seek($out, 0, 0);  # just in case files are the same
print {$out} bencode($t);

close($in);
close($out);
print {$msg} "done\n";
exit;
