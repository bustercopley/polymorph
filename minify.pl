#!c:\strawberry\perl\bin\perl.exe
# Minifies Polymorph's GLSL shader sources.
# Not meant for general use.
# Doesn't handle multiline statements.
# Identifiers aren't safe from collisions.

use strict;

my ($infile, $outfile) = @ARGV;

open my $in, "<", $infile or die "Can't open input file \"$infile\": $!";
open my $out, ">", $outfile or die "Can't open output file \"$outfile\": $!";

binmode $out;

my %abbrevs = (
  # Geometry shader
  project       => "I",  # (vec3 x) -> vec4
  pdivide       => "I",  # (vec4 s) -> vec2
  raster        => "J",  # (vec3 x) -> vec2
  perp          => "I",  # (vec2 a, vec2 b) -> vec2
  dist          => "I",  # (vec2 x, vec2 a, vec2 b) -> float
  vertex        => "I",  # (vec3 x, vec4 p, float e [5]) -> void
  triangle      => "I",  # (vec3 A, vec3 B, vec3 C, vec4 x, vec4 y, vec4 z, float e [5], float f [5], float g [5]) -> void
  snub_segment  => "I",  # (vec3 Q, vec3 U, vec3 V, vec4 y, vec4 z) -> void
  aspect        => "I",  # (vec3 Q, vec3 V, vec3 W, vec3 X, vec4 h, vec4 i, vec4 j, vec2 v, vec2 w, vec2 x) -> void
);

%abbrevs = map { qr/\b$_\b/ => $abbrevs {$_} } keys %abbrevs;

my $limit = 900;

sub minify
{
  my ($buffer, $bol) = @_;

  # Abbreviate the specified words.
  $buffer =~ s/$_/$abbrevs{$_}/g for keys %abbrevs;

  # Keep space only if it separates two words.
  $buffer =~ s{\B }{}g; # strip space not after a word
  $buffer =~ s{ \B}{}g; # strip space not before a word
  $buffer =~ m{ \B} and die "That didn't do what I expected";

  # Delete unnecessary zeros from floating-point literals.
  $buffer =~ s{(\d\.\d*)0+\b}{$1}g;
  $buffer =~ s{\b0\.(\d)}{.$1}g;
  while ($limit and $buffer ne "") {
    # Zero or more declarations.
    # Coalesce consecutive declarations that share a common type into one declaration.
    my $oldtype;
    my $oldvars;
    my $v = qr/\w+(?:\[\w+\])?(?:=[^;]+)?/;
    while ($limit and $buffer =~ s/^((?:\w+ )+)($v(?:,$v)*);//) {
      my ($type, $vars) = ($1, $2);
      -- $limit if $limit;
      if ((defined $oldtype) && ($type eq $oldtype)) {
        $oldvars .= ",$vars";
      }
      else {
        if (defined $oldtype) {
          print $out "$oldtype$oldvars;";
          $bol = 0;
        }
        $oldtype = $type;
        $oldvars = $vars;
      }
    }
    if (defined $oldtype)
    {
      print $out "$oldtype$oldvars;";
      undef $oldtype;
      $bol = 0;
    }
    # A statement that isn't a declaration.
    if ($buffer =~ s/^([^;{}]*(?:[;{}]+|$))//) {
      my $statement = $1;
      print $out $statement;
      $bol = 0;
    }
    -- $limit if $limit;
  }

  return $bol;
}

my $buffer = "";
my $bol = 1;
while (defined (my $line = <$in>))
{
  $line =~ s{^\s+}{};         # trim leading space
  $line =~ s{//.*$}{};        # strip comment
  $line =~ s{\s+$}{};         # trim trailing space
  $line =~ s{\s+}{ };         # squeeze repeated space
  next if $line eq "";        # skip blank line

  # Print directive on a separate line.
  if ($line =~ m/^#/)
  {
    $bol = minify $buffer, $bol;
    $buffer = "";
    print $out "\n" unless $bol;
    print $out "$line\n";
    $bol = 1;
  }
  else
  {
    $buffer = "$buffer $line";
  }
}

$bol = minify $buffer, $bol;
#print $out "\n" unless $bol;
