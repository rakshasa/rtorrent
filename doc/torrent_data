#!/usr/bin/perl

# Perl script to read torrent data

use strict;
use warnings;
use Convert::Bencode_XS qw(bdecode);
use Data::Dumper;

$/ = undef;
$| = 1;

my $in;
my $in_file = $ARGV[0];
if ($in_file) { open($in, '<', $in_file) || die "Cannot open $in_file for input!\n"; }
else          { $in = *STDIN; }

print "Total input torrent size: ".sprintf('%.2f', (-s $in_file) / 1024)." KB\n" if $in_file;
print "Decoding ".($in_file || 'torrent from standard input')."...";
my $t = bdecode(scalar <$in>);
print "done\n\n";

print Data::Dumper->new([$t], ['*Torrent'])->Indent(1)->Useqq(1)->Quotekeys(0)->Sortkeys(1)->Dump;
