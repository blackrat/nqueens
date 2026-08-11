# History

Copyright (c) 1992-2026 Paul McKibbin

How this program got here: what the original looked like, and what is known
about when it was written.

## The original

The original was five `.c` files — `8qrecurs.c`, `8qalife.c`, `work.c`,
`gnuwork.c`, `test1.c`. Four of them are near-identical and carry *both*
solvers, recursive and genetic, in one source behind `#ifdef GENETIC`, differing
only in `#define`d constants and which branch was left live; `test1.c` is an
earlier genetic-only program with no recursive path at all. Board size,
population, time budget and the choice of algorithm were all compile-time, so
trying a different board size meant editing a source file, and trying a
different *combination* meant keeping another copy of the whole program. They
are one program with runtime options now; that is the bulk of the change.

The single-purpose filenames (`8qrecurs.c` versus `8qalife.c`) came later than
the dual-algorithm sources, splitting the program so that building one or the
other no longer meant toggling a flag. That ordering matters for dating, and is
set out under "Dating the original" below.

## Dating the original

This file records what can and cannot be established about *when* the original
program was written, and on what evidence. It exists because the date matters
for establishing priority, and because most of the obvious evidence in this
repository is worthless for that purpose.

Everything below was gathered on 2026-08-11 from the working tree and from the
archived copies held on the household archive host. Paths below are given
relative to its archive root, as `archive/paul/...`. An unredacted copy of this
file, naming the host and its addresses, is kept in the private repository.

### What is NOT evidence

Read this first, because it rules out the things one instinctively reaches for.

- **File modification times in this repository.** Git stores no mtimes. Every
  file in a checkout is stamped with the moment git wrote it to disk. In this
  working tree that is `2026-08-10 10:25` for all of them.
- **File modification times in the archived copies.** They are uniform per
  directory - `2008-03-31 12:04` for the whole `oldsrc/proj/queens` tree,
  `2008-03-30 18:57` for `Work in Progress/src/queens`, `2007-04-03 05:34` for
  `My Documents/wip/src/queens`. A whole directory sharing one timestamp to the
  minute is the signature of a copy or restore, not of authorship. Where a file
  *was* genuinely edited later the stamp does survive and stands out: in
  `unfiled/ssprojects/common/alife/queens`, `8qgenetic.c` and `8qrecurs.c` carry
  `2012-03-11`, distinct from the `2008-03-30` bulk stamp around them.
- **The git history.** The earliest commit, `a88f02b "standardise layout"`, is
  dated `2014-02-08`. That is an import of code already long finished.
- **A copyright notice.** Including the one now at the top of these files. It is
  an assertion, not a record. It carries no weight on its own.

### What IS evidence

Each of these is a tool artefact embedded in a file. None could have been
produced before the tool that made it existed, which makes them hard lower
bounds that survive copying, restoring and re-importing.

| Artefact | Contents | Establishes |
| --- | --- | --- |
| `qsort.c` | `This file may have been modified by DJ Delorie (Jan 1991)` - the DJGPP port of the 4.3BSD qsort | not before **Jan 1991** |
| `8q.exe`, `gnuwork.exe` | Runtime banner `Borland C++ - Copyright 1991 Borland Intl.` | not before **1991** |
| `8qrecurs.exe` | Runtime banner `WATCOM C 386 Run-Time system. (c) Copyright by WATCOM Systems Inc. 1989, 1992` | not before **1992** |
| `8qrec.xls`, `8x81.xls` | First record `0x0409` = BIFF4, the Excel 4.0 stream format | not before **1992** (Excel 4.0's release) |
| `8qrec.xls`, `8x81.xls` | Still BIFF4, *not* the OLE2 compound format Excel 5.0 introduced in 1993 | never re-saved after ~1993 |
| `gnutest.exe` | DJGPP `a.out` (`0x010b` ZMAGIC), links `d:/gnu/lib/crt0.o` | early-90s DJGPP |
| `command.com` | `Microsoft(R) MS-DOS(R) Version 5.00 / (C)Copyright Microsoft Corp 1981-1991`, 47,845 bytes - the MS-DOS 5.0 fingerprint | machine ran **MS-DOS 5.0**, released Jun 1991, superseded Mar 1993 |
| `tc0000.swp` | Turbo C IDE swap file, 256 KB, left in the directory | an active Turbo C workspace, not a copy of someone else's tree |
| `delta.mvm` | PVCS Version Manager config, with a `/queens` subdirectory | PVCS Version Manager, an early-90s product |
| `8q.tfa` | Turbo Profiler areas file for `F:\PROJ\QUEENS\8Q.EXE` | Borland Turbo Profiler |
| `8q.prj`, `tdconfig.td`, `tfconfig.tf` | Borland project, Turbo Debugger and Turbo Profiler configs; include path `C:\TRANS\BC\INCLUDE` | Borland DOS toolchain |

**Conclusion: the original work dates to 1992-1993.**

Three independent artefacts put a floor under it - the Watcom C/386 runtime
(1992), the Excel 4.0 stream format (Excel 4.0 for Windows shipped April 1992),
and the vendored DJGPP `qsort.c` (Jan 1991). Two more bracket it from above: the
machine was running MS-DOS 5.0, current from June 1991 until MS-DOS 6.0 in March
1993, and the spreadsheets were never re-saved into the OLE2 format Excel 5.0
introduced in 1993. Nothing in the set postdates 1992, and the whole set is
mutually consistent with a single working period on one machine.

The `tc0000.swp` left behind shows this was the working directory the program
was actually developed in, not an archive copy of someone else's.

This covers **both** solvers. The early sources hold the recursive and the
genetic algorithm in one file behind `#ifdef GENETIC`; see "Both algorithms are
in the oldest set" below.

A second, later working period is also visible and should not be confused with
the first: `8qalife.dsw`, `.opt`, `.ncb` and `.plg` are Visual C++ 6 files
(1998+), and `8qrecurs.plg` records builds from `Y:\Work in Progress\Src\
queens\`. A third touch, on Linux, is dated `2012-03-11`.

### Chain of custody

The evidence above lives in the archived directory; the code being rewritten
lives in this git repository. The two are the same bytes. Every original file
tracked in git at commit `a88f02b` hashes identically to its counterpart in
`archive/.../oldsrc/proj/queens` on the archive host, verified 2026-08-11 with SHA-256:

| File | SHA-256 |
| --- | --- |
| `8qrecurs.c` | `dd79067e5ab7d96596a1baebfde8060c4fead186f2346a999e2172e30c037b6f` |
| `gnuwork.c` | `01a6424e22e54066c5b6ec5d0ff5ab27d5f93b83dadd8d623e54b760a4e6110f` |
| `work.c` | `f514894ce36f34f9d2693bd15616f0b146e8c0bb11d8f70ba6986cd7b2c1e268` |
| `test1.c` | `ceedf2433ef4741557ed5e0bc3d31e34a8ac74deaf78c092810d363f3d19c3ef` |
| `qsort.c` | `d3d79d65a0c2650a03f707bb0ec1b1ba387e5dc20fe2e890e88db26bd86dea92` |
| `solns.txt` | `85592cbb2096a0798badb060341527416083d2627652ebc541d3f005579085f5` |
| `8q.prj` | `c5ee5761071436950de3ae80205c3b26ae0545f2938cf366c3a5ba50ca03a947` |
| `8q.tfa` | `5fd9e13716982dd22c99120ce234e2e425125fa7a2a0e7402e33cefc3bafa4de` |
| `delta.mvm` | `b501ec6cf23b8783ff58f566d7fe7cc6780cf1f849f65ba20e8d8d31a4a31205` |
| `tdconfig.td` | `4817d5217beeb4b941a0f9950b6d39c4a71d546061f498da5c5e9a30399eed90` |
| `tfconfig.tf` | `86055917914d8fce4e94d462c024e5628c79536323e0cdbbdb62505a29125a03` |

That matters because it is what ties the dating evidence to this code. The
`command.com`, the Watcom-linked `8qrecurs.exe`, the Excel 4.0 spreadsheets and
the Turbo C swap file are not in git - but they sat in the same directory as
these exact bytes, which are.

Checksums of the remaining artefacts in that directory, none of which are in
git, are worth recording for the same reason:

| File | SHA-256 |
| --- | --- |
| `8q.exe` | `5307e7c7223ed4ac7813a3ef132980f64419c04a10919e02a86e7a0c71dca034` |
| `8qrecurs.exe` (Watcom) | `bba18c5f78e5938cd0cf7a051085588a7f27edd6b2ca7ae07d2af529c7be6802` |
| `8qrecur.exe` | `5bf411c9ebc56abb9267c4577adf91c108e2b6fc61bedb9b3fe864ed65185d97` |
| `gnuwork.exe` | `2c8de7d271c84095a2dd8291dd1c61b422191aea4a9eaa305e999a5170e281b2` |
| `gnutest.exe` (DJGPP) | `ccb4141f614fc31503dd8acafa07614f895da1e3a63fcea53cb9b2ae344c623e` |
| `test1.exe` | `fb83bf0b4d53de4b0566be33925922024272ba0830773c2f70932fa554efefe7` |
| `8qrecurs.obj` | `1270fd58d4d302f22a68d8890de82497b62873c0eeef2ae6cc577d2a86a87187` |
| `8q.map` | `ca4d33536500fdf0dd3eb53b0061639ac800e07203af812af555dc49f876d9e1` |
| `8qrec.xls` (BIFF4) | `79575af71fabebae15f1bfbd41f5bedf377e4783046531e8a08f2410facc55ad` |
| `8x81.xls` (BIFF4) | `5e18dba9207a6eed7044995b1062b7cda57006f0717a6fda31d2be0232ab708f` |
| `command.com` (MS-DOS 5.0) | `74ad25067e7d9f5c3a0ac2f2967347defb0ffef9e6ed60b60e6499f0b1a8cc89` |
| `tc0000.swp` | `4cdce519297f7c8792d89c9ff05e5604b06bbd75fb7fae383c8705cbe79bba6c` |

#### Both algorithms are in the oldest set

It would be easy to read the filenames in the oldest directory - `8qrecurs.c`,
`work.c`, `gnuwork.c` - and conclude that only the recursive solver is evidenced
at 1992-1993, with the genetic algorithm arriving later as `8qalife.c`. That
reading is wrong, and the sources say so.

The single-purpose filenames came later, splitting the program so a build no
longer meant editing a flag. The early files carry **both** algorithms in one
source, chosen by `#ifdef GENETIC`:

| File | Genetic code present | Recursive code present | Flag as committed |
| --- | --- | --- | --- |
| `test1.c` | yes - `GENE`, `breed`, `crossover`, `play_gene` | **no `place_queens` at all** | n/a, it is genetic-only |
| `work.c` | yes | yes | `#define GENETIC` - **active** |
| `8qrecurs.c` | yes | yes | `/* #define GENETIC /**/` - off |
| `gnuwork.c` | yes | yes | `/*#define GENETIC*/` - off |

All four are in `oldsrc/proj/queens`, and all four are byte-identical to
the copies in git (hashes above). The genetic machinery in them is complete, not
a stub: a `GENE` record of code plus fitness, a population array, rank-sorted
breeding of the bottom half from top-half parents, one-point crossover, and
separately switchable son and daughter mutation:

```c
/* 8qrecurs.c, lines 23-30 */
/* #define GENETIC /**/
#ifdef GENETIC
	#define MUTSON		/* Chance of Son mutating */
	#define MUTDAUGHTER /* Chance of Daughter mutating */
/*	#define FITTEST	/* Already found solution has weight of zero */
#else
	#define RECURSIVE
#endif
```

`test1.c` is the clearest single artefact for the genetic work: it contains the
genetic solver and nothing else - no `place_queens`, no recursive path - so it
cannot be read as a recursive program that happens to carry unused code.

**The 1992-1993 dating therefore covers the genetic algorithm as squarely as the
recursive one.** `8qalife.c` in the later trees is a split-out of code already
present here, not the first appearance of the technique.

### On Subversion

The original working assumption was that the history lived in a Subversion
archive. It cannot: **Subversion did not exist until 2000** (1.0 shipped in
2004), so no SVN record can predate the artefacts above by any margin. A search
of `archive/paul` found `.svn` working copies for other projects
(Asterisk, HomeSystem, minimyth, rails_misc) but **none in any `queens` tree**,
and no repository (`db/fs-type`), dump or tarball for it.

If an SVN copy does turn up elsewhere it is a later custodial record, useful for
continuity of possession but not for the origin date. Note also that `svn:date`
is a *revision property* and revision properties are mutable
(`svn propset --revprop`), so an SVN date is weaker evidence than the tool
artefacts here, not stronger.

### Strengthening the record

The evidence above is circumstantial but independent and hard to fake after the
fact. If priority genuinely needs to be defended, the things that would harden
it are:

- Original media - floppies, tapes, CD-Rs - with intact filesystem dates, or the
  oldest backup set that contains the tree.
- Anything contemporaneous and externally dated: correspondence, a dated
  printout, a letter.
- A cryptographic timestamp on the current tree (an RFC 3161 token, or an OpenTimestamps
  proof). This proves the files existed no *later* than now, which does not help
  with 1992 priority, but it stops the record drifting further and is cheap.

### Third-party code

For completeness, since it bears on what is and is not original work here:

- The original tree vendored `qsort.c`, a 4.3BSD routine (Regents of the
  University of California, 1980/1983) as modified by DJ Delorie (1991). It was
  **removed** in the 2026 rewrite - the C library's `qsort` is used instead - so
  no Berkeley or DJGPP code remains.
- An earlier draft of the 2026 rewrite used PCG32 (M. E. O'Neill, 2014) as the
  genetic search's random source. It was **replaced on 2026-08-11** by a plain
  64-bit linear congruential generator - Lehmer's method (1949), Knuth TAOCP
  Vol. 2 sec. 3.2.1 - so that nothing in the tree postdates the copyright the
  tree carries. Textbook arithmetic and published constants; no outside
  pedigree, and no anachronism in a file marked 1992.

**Every line in `src/`, `tests/` and `tools/` is now either original to this
work or textbook method long predating it.** No third-party source remains.
