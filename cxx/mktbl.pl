use Text::CSV qw(csv);
use strict;
use warnings;
use Data::Dumper;
use bigint;

my $entries = csv (in => "opcodes-all.csv", headers => "auto", encoding=>"UTF-8");
my @data = @$entries;


print "const Opcode opcodes[] = {\n";
foreach my $entry (@data) {
#    print Dumper($entry);
format STDOUT = 
Opcode( "@<<", @#, @<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<, @<<<<<<<<, @<<<, @<<<, @<<<<<, @<<<<< ), // @<<<< -- @<< @<<<<<<<<
$entry->{'Name'}, $entry->{'Base Size'}, $entry->{'Flags'}, $entry->{'Data Reader'}, $entry->{'State Handler'}, $entry->{'Label Handler'}, $entry->{'M Flag Use'}, $entry->{'X Flag Use'}, $entry->{'Value'}, $entry->{'Name'}, $entry->{'Params Example'}
.
    write();
}

print "};\n";