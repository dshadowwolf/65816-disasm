#!/usr/bin/perl -w
print <<END;
typedef struct ocdb_s {
    const char *opcode;
    const uint8_t psize;
    int (*munge)(int);
    const uint8_t flags;
} ocdb_t;

const ocdb_t[256] opcodes = {
END

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
    printf("{ \"%3s\", %d, % 5s, %s /* %s %s */ },\n", $opcode, $operandsize, $munger, $flags, $opcode, $params);
}
print "};\n";