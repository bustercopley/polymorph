#!c:\strawberry\perl\bin\perl.exe
# Minifies polymorph's hlsl shader sources.
# Not meant for general use.
# Doesn't handle multiline statements.
# Identifiers aren't safe from collisions.

use strict;

my ($infile, $outfile) = @ARGV;

open my $in, "<", $infile or die "Can't open input file \"$infile\": $!";
open my $out, ">", $outfile or die "Can't open output file \"$outfile\": $!";

my $buffer = "";
my $oldtype;
my $oldvars;
my $bol = 1;

# Shorten certain identifiers.
my %abbrevs = (
  "triangle" => "I", # 9 arguments
  "segment" => "I",  # 5 or 7 arguments
  "amplify" => "I",  # 1 argument
  "aspect" => "I",   # 6 arguments
  "vertex" => "I",   # 3 arguments
  "color" => "J",    # 1 argument
  "snub" => "I",     # 8 arguments
  "sq" => "K",       # 1 argument
);

%abbrevs = map { qr/\b$_\b/ => $abbrevs {$_} } keys %abbrevs;

while (defined (my $line = <$in>))
{
  $line =~ s{^\s+}{};   # trim leading space
  $line =~ s{//.*$}{};  # strip comment
  $line =~ s{\s+$}{};   # trim trailing space
  $line =~ s{\s+}{ };   # squeeze repeated space
  next if $line eq "";  # skip blank line

  # Coalesce consecutive declarations that share a common type into one declaration.

  # Print directive on a separate line.
  if ($line =~ m/^#/)
  {
    if (defined $oldtype)
    {
      print $out "$oldtype$oldvars;";
      undef $oldtype;
      $bol = 0;
    }
    print $out "\n" if ! $bol;
    print $out "$line\n";
    $bol = 1;
    next;
  }
  else
  {
    s/$_/$abbrevs{$_}/ for keys %abbrevs;

    $buffer = "$buffer $line";

    # Keep space only if it separates two words.
    $buffer =~ s{\B }{}g; # strip space not after a word
    $buffer =~ s{ \B}{}g; # strip space not before a word

    while ($buffer =~ m/^((?:\w+ )+)(\w+(?:=[^;]*)?);(.*)$/)
    {
      my $type = $1;
      my $vars = $2;
      $buffer = $3;

      if ((defined $oldtype) && ($type eq $oldtype))
      {
        $oldvars .= ",$vars";
      }
      else
      {
        if (defined $oldtype)
        {
          print $out "$oldtype$oldvars;";
        }
        $oldtype = $type;
        $oldvars = $vars;
      }
    }
    if ($buffer ne "")
    {
      if (defined $oldtype)
      {
        print $out "$oldtype$oldvars;";
      }
      undef $oldtype;
      print $out $buffer;
      $buffer = "";
      $bol = 0;
    }
    next;
  }
}