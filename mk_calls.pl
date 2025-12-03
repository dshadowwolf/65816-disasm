#!/usr/bin/env perl

use strict;

foreach (<>) {
    chomp();
    my @fields = split(/\s+,\s/);
    my $opcall = $fields[9];

    format STDOUT = 
state_t* @<<<<<<<<<<<< (state_t* machine, uint16_t arg_one, uint16_t arg_two) {};
    $opcall
.
    write(STDOUT);
}