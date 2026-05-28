#!/usr/bin/perl
# Runs the coreutils cut.pl test vectors against our binary.
#
# Usage: perl run_cut_tests.pl <path-to-binary>
#
# Provides the minimal framework stubs that cut.pl expects
# (getlimits, triple_test, run_tests), then do-includes cut.pl.
# Tests that exercise unsupported features are silently skipped.

use strict;
use warnings;
use File::Temp  qw(tempdir);
use File::Basename qw(basename);
use Cwd         qw(abs_path);
use FindBin     qw($Bin);

# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

my $binary = $ARGV[0] or die "Usage: $0 <binary-path>\n";
$binary = abs_path($binary);
-x $binary or die "$0: '$binary' is not executable\n";

my $prog_name = basename($binary);
my $verbose   = $ENV{VERBOSE} // 0;

# ---------------------------------------------------------------------------
# Unsupported-feature filter
# Tests matching any of these arg patterns are skipped.
# ---------------------------------------------------------------------------

my @SKIP_ARG_PATTERNS = (
    qr/^--output-d/,          # --output-delimiter / --output-d=
    qr/^--out=/,              # --out=VALUE
    qr/^--ou=/,               # abbreviated --output-delimiter
    qr/^-O/,                  # -O / -O<val>
    qr/^-z$/,                 # zero-terminated
    qr/^--zero-terminated/,
    qr/^--complement$/,
    qr/^-F$/,                 # fixed-whitespace field mode
    qr/^--whitespace-delimited=trimmed/,
);

sub _skip_reason {
    for my $a (@_) {
        for my $p (@SKIP_ARG_PATTERNS) {
            return $a if $a =~ $p;
        }
    }
    return '';
}

# ---------------------------------------------------------------------------
# Framework stubs  (called by cut.pl as plain function names in main::)
# ---------------------------------------------------------------------------

my $IO_BUFSIZE = 4096;

sub getlimits {
    return {
        IO_BUFSIZE    => $IO_BUFSIZE,
        UINTMAX_MAX   => '18446744073709551615',
        UINTMAX_OFLOW => '18446744073709551616',
    };
}

# Simplified triple_test: just return the original test list unchanged.
# The user-case shell script already covers stdin / dash variants explicitly.
sub triple_test {
    my ($ref) = @_;
    return @$ref;
}

# ---------------------------------------------------------------------------
# Test execution state
# ---------------------------------------------------------------------------

my ($n_pass, $n_fail, $n_skip) = (0, 0, 0);

# ---------------------------------------------------------------------------
# run_tests  — called by cut.pl at the end of its @Tests definition
# ---------------------------------------------------------------------------

sub run_tests {
    my ($me, $prog, $tests_ref, $save_temps, $verbose_flag) = @_;
    # $prog is 'cut' (from cut.pl) — we ignore it and use $binary instead.

    for my $t (@$tests_ref) {
        _run_one($t);
    }

    printf "\nResults: %d passed, %d failed, %d skipped\n",
           $n_pass, $n_fail, $n_skip;

    return $n_fail;
}

# ---------------------------------------------------------------------------
# Single-test runner
# ---------------------------------------------------------------------------

sub _run_one {
    my ($test) = @_;
    my ($name, @items) = @$test;

    my @args;
    my $stdin_str   = '';
    my %file_inputs;           # fname => content  (from {IN=>{fname=>content}})
    my $exp_stdout  = '';
    my $exp_stderr  = '';
    my $exp_exit    = 0;
    my %extra_env;

    for my $item (@items) {
        if (ref $item eq 'HASH') {
            if (exists $item->{IN}) {
                my $v = $item->{IN};
                if (ref $v eq 'HASH') {
                    # Named-file input(s)
                    %file_inputs = (%file_inputs, %$v);
                } else {
                    # Stdin content
                    $stdin_str .= $v;
                }
            }
            elsif (exists $item->{OUT})  { $exp_stdout = $item->{OUT}  }
            elsif (exists $item->{ERR})  { $exp_stderr = $item->{ERR}  }
            elsif (exists $item->{EXIT}) { $exp_exit   = $item->{EXIT} }
            elsif (exists $item->{ENV})  {
                $extra_env{$1} = $2 if $item->{ENV} =~ /^(\w+)=(.*)$/;
            }
        } else {
            push @args, $item;
        }
    }

    # ---- Skip unsupported features ----
    if (my $reason = _skip_reason(@args)) {
        $n_skip++;
        print "SKIP  $name  (unsupported: $reason)\n" if $verbose;
        return;
    }

    # ---- Skip if locale is unavailable ----
    if (my $lc = $extra_env{LC_ALL}) {
        if ($lc ne 'C') {
            my $avail = (system("LC_ALL=\Q$lc\E locale charmap >/dev/null 2>&1") == 0);
            unless ($avail) {
                $n_skip++;
                print "SKIP  $name  (locale $lc unavailable)\n" if $verbose;
                return;
            }
        }
    }

    # ---- Adapt expected error messages to our binary name ----
    # cut.pl uses 'cut:' as the prefix; our binary uses its own basename.
    (my $exp_err = $exp_stderr) =~ s/\bcut: /$prog_name: /g;
    $exp_err =~ s/'cut (--help)'/'$prog_name $1'/g;

    # ---- Build temp dir and write named-file inputs ----
    my $tmpdir = tempdir(CLEANUP => 1);
    my @file_args;
    for my $fname (sort keys %file_inputs) {
        my $path = "$tmpdir/$fname";
        open my $fh, '>', $path or die "Cannot write $path: $!";
        binmode $fh;
        print $fh $file_inputs{$fname};
        close $fh;
        push @file_args, $path;
    }

    my @final_args = (@args, @file_args);

    # ---- Execute ----
    my ($got_out, $got_err, $got_exit) = _exec($binary, \@final_args, $stdin_str, \%extra_env);

    # ---- Compare ----
    my @mismatches;
    push @mismatches, 'stdout' if $got_out  ne $exp_stdout;
    push @mismatches, 'stderr' if $got_err  ne $exp_err;
    push @mismatches, 'exit'   if $got_exit != $exp_exit;

    if (!@mismatches) {
        $n_pass++;
        print "PASS  $name\n" if $verbose;
    } else {
        $n_fail++;
        print "FAIL  $name  [", join(', ', @mismatches), "]\n";
        if ($got_out ne $exp_stdout) {
            print "    stdout exp: ", _vis($exp_stdout), "\n";
            print "    stdout got: ", _vis($got_out),    "\n";
        }
        if ($got_err ne $exp_err) {
            print "    stderr exp: ", _vis($exp_err),  "\n";
            print "    stderr got: ", _vis($got_err),  "\n";
        }
        if ($got_exit != $exp_exit) {
            print "    exit   exp=$exp_exit  got=$got_exit\n";
        }
    }
}

# ---------------------------------------------------------------------------
# Process execution  (fork+exec with temp-file I/O for portability)
# ---------------------------------------------------------------------------

sub _exec {
    my ($bin, $args_ref, $stdin_str, $env_ref) = @_;

    my $tmpdir = tempdir(CLEANUP => 1);
    my ($in_f, $out_f, $err_f) = map { "$tmpdir/$_" } qw(in out err);

    { open my $f, '>', $in_f or die "open: $!"; binmode $f; print $f $stdin_str; }

    my $pid = fork() // die "fork: $!";
    if ($pid == 0) {
        $ENV{$_} = $env_ref->{$_} for keys %$env_ref;
        open STDIN,  '<', $in_f  or die;
        open STDOUT, '>', $out_f or die;
        open STDERR, '>', $err_f or die;
        exec { $bin } $bin, @$args_ref or die "exec: $!";
    }
    waitpid($pid, 0);
    my $ec = $? >> 8;

    local $/;
    my ($out, $err) = ('', '');
    { open my $f, '<', $out_f or die; binmode $f; $out = <$f> // ''; }
    { open my $f, '<', $err_f or die; binmode $f; $err = <$f> // ''; }

    return ($out, $err, $ec);
}

# ---------------------------------------------------------------------------
# Pretty-print a string for diff output
# ---------------------------------------------------------------------------

sub _vis {
    my ($s) = @_;
    $s =~ s/\\/\\\\/g;
    $s =~ s/\n/\\n/g;
    $s =~ s/\t/\\t/g;
    $s =~ s/[\x00-\x1f\x7f-\xff]/sprintf('\\x%02x', ord($&))/ge;
    return qq("$s");
}

# ---------------------------------------------------------------------------
# Load and execute cut.pl  (it will call run_tests above)
# ---------------------------------------------------------------------------

my $cut_pl = "$Bin/cut.pl";
-f $cut_pl or die "Cannot find cut.pl at: $cut_pl\n";

do $cut_pl;
die $@ if $@;
