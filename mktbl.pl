#!/usr/bin/perl -w
use bigint;
my $ln = 0;
foreach (<>) {
    chomp();
    my @d = split(/\s+,\s/);
    foreach (<@d>) { chomp(); }
    my $opcode = $d[0];
    my $params = $d[1];
    my $codeval = $d[2];
    my $flags = join( ' | ', split(/\s/, $d[3]));
    my $operandsize = int($d[4])-1;
    my $munger = $d[5];
    my $state = $d[6];
    my $extra = $d[7];
    my $reader = $d[8];
    my $opcodeval = uc($ln->as_hex);
    format STDOUT = 
{ "@<<", @##, @<<<<, @<<<, @<<<, @<<<<<<<<, @<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< /* @<<< @<<<<<<<< */ }, // @>>>>
    $opcode, $operandsize, $munger, $state, $extra, $reader, $flags, $opcode, $params, $opcodeval
.
    $ln++;
    write(STDOUT);
}
