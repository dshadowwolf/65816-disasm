#!/usr/bin/perl -w
print <<END;
typedef struct opcode_s {
    const char *opcode;
    const uint8_t psize;
    int (*munge)(int);
    void (*state)(unsigned char);
    void (*extra)(uint32_t, ...);
    int reader();
    const uint8_t flags;
} opcode_t;

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
    my $state = $d[6];
    my $extra = $d[7];
    my $reader = $d[8];
    printf("{ \"%3s\", %02d, % 5s, % 4s, % 4s, % 9s, % 36s /* %3s %8s */ },\n", $opcode, $operandsize, $munger, $state, $extra, $reader, $flags, $opcode, $params);
}
print "};\n";