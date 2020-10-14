/* (N)compress.c - File compression ala IEEE Computer, Mar 1992.
 *
 * Authors:
 *   Spencer W. Thomas   (decvax!harpo!utah-cs!utah-gr!thomas)
 *   Jim McKie           (decvax!mcvax!jim)
 *   Steve Davies        (decvax!vax135!petsd!peora!srd)
 *   Ken Turkowski       (decvax!decwrl!turtlevax!ken)
 *   James A. Woods      (decvax!ihnp4!ames!jaw)
 *   Joe Orost           (decvax!vax135!petsd!joe)
 *   Dave Mack           (csu@alembic.acs.com)
 *   Peter Jannesen, Network Communication Systems
 *                       (peter@ncs.nl)
 *   Mike Frysinger      (vapier@gmail.com)
 *
 * Revision 4.2.3  92/03/14 peter@ncs.nl
 *   Optimise compress and decompress function and a lot of cleanups.
 *   New fast hash algoritme added (if more than 800Kb available).
 *
 * Revision 4.1  91/05/26 csu@alembic.acs.com
 *   Modified to recursively compress directories ('r' flag). As a side
 *   effect, compress will no longer attempt to compress things that
 *   aren't "regular" files. See Changes.
 *
 * Revision 4.0  85/07/30  12:50:00  joe
 *   Removed ferror() calls in output routine on every output except first.
 *   Prepared for release to the world.
 * 
 * Revision 3.6  85/07/04  01:22:21  joe
 *   Remove much wasted storage by overlaying hash table with the tables
 *   used by decompress: tab_suffix[1<<BITS], stack[8000].  Updated USERMEM
 *   computations.  Fixed dump_tab() DEBUG routine.
 *
 * Revision 3.5  85/06/30  20:47:21  jaw
 *   Change hash function to use exclusive-or.  Rip out hash cache.  These
 *   speedups render the megamemory version defunct, for now.  Make decoder
 *   stack global.  Parts of the RCS trunks 2.7, 2.6, and 2.1 no longer apply.
 *
 * Revision 3.4  85/06/27  12:00:00  ken
 *   Get rid of all floating-point calculations by doing all compression ratio
 *   calculations in fixed point.
 *
 * Revision 3.3  85/06/24  21:53:24  joe
 *   Incorporate portability suggestion for M_XENIX.  Got rid of text on #else
 *   and #endif lines.  Cleaned up #ifdefs for vax and interdata.
 *
 * Revision 3.2  85/06/06  21:53:24  jaw
 *   Incorporate portability suggestions for Z8000, IBM PC/XT from mailing list.
 *   Default to "quiet" output (no compression statistics).
 *
 * Revision 3.1  85/05/12  18:56:13  jaw
 *   Integrate decompress() stack speedups (from early pointer mods by McKie).
 *   Repair multi-file USERMEM gaffe.  Unify 'force' flags to mimic semantics
 *   of SVR2 'pack'.  Streamline block-compress table clear logic.  Increase 
 *   output byte count by magic number size.
 * 
 * Revision 3.0   84/11/27  11:50:00  petsd!joe
 *   Set HSIZE depending on BITS.  Set BITS depending on USERMEM.  Unrolled
 *   loops in clear routines.  Added "-C" flag for 2.0 compatibility.  Used
 *   unsigned compares on Perkin-Elmer.  Fixed foreground check.
 *
 * Revision 2.7   84/11/16  19:35:39  ames!jaw
 *   Cache common hash codes based on input statistics; this improves
 *   performance for low-density raster images.  Pass on #ifdef bundle
 *   from Turkowski.
 *
 * Revision 2.6   84/11/05  19:18:21  ames!jaw
 *   Vary size of hash tables to reduce time for small files.
 *   Tune PDP-11 hash function.
 *
 * Revision 2.5   84/10/30  20:15:14  ames!jaw
 *   Junk chaining; replace with the simpler (and, on the VAX, faster)
 *   double hashing, discussed within.  Make block compression standard.
 *
 * Revision 2.4   84/10/16  11:11:11  ames!jaw
 *   Introduce adaptive reset for block compression, to boost the rate
 *   another several percent.  (See mailing list notes.)
 *
 * Revision 2.3   84/09/22  22:00:00  petsd!joe
 *   Implemented "-B" block compress.  Implemented REVERSE sorting of tab_next.
 *   Bug fix for last bits.  Changed fwrite to putchar loop everywhere.
 *
 * Revision 2.2   84/09/18  14:12:21  ames!jaw
 *   Fold in news changes, small machine typedef from thomas,
 *   #ifdef interdata from joe.
 *
 * Revision 2.1   84/09/10  12:34:56  ames!jaw
 *   Configured fast table lookup for 32-bit machines.
 *   This cuts user time in half for b <= FBITS, and is useful for news batching
 *   from VAX to PDP sites.  Also sped up decompress() [fwrite->putc] and
 *   added signal catcher [plus beef in write_error()] to delete effluvia.
 *
 * Revision 2.0   84/08/28  22:00:00  petsd!joe
 *   Add check for foreground before prompting user.  Insert maxbits into
 *   compressed file.  Force file being uncompressed to end with ".Z".
 *   Added "-c" flag and "zcat".  Prepared for release.
 *
 * Revision 1.10  84/08/24  18:28:00  turtlevax!ken
 *   Will only compress regular files (no directories), added a magic number
 *   header (plus an undocumented -n flag to handle old files without headers),
 *   added -f flag to force overwriting of possibly existing destination file,
 *   otherwise the user is prompted for a response.  Will tack on a .Z to a
 *   filename if it doesn't have one when decompressing.  Will only replace
 *   file if it was compressed.
 *
 * Revision 1.9  84/08/16  17:28:00  turtlevax!ken
 *   Removed scanargs(), getopt(), added .Z extension and unlimited number of
 *   filenames to compress.  Flags may be clustered (-Ddvb12) or separated
 *   (-D -d -v -b 12), or combination thereof.  Modes and other status is
 *   copied with copystat().  -O bug for 4.2 seems to have disappeared with
 *   1.8.
 *
 * Revision 1.8  84/08/09  23:15:00  joe
 *   Made it compatible with vax version, installed jim's fixes/enhancements
 *
 * Revision 1.6  84/08/01  22:08:00  joe
 *   Sped up algorithm significantly by sorting the compress chain.
 *
 * Revision 1.5  84/07/13  13:11:00  srd
 *   Added C version of vax asm routines.  Changed structure to arrays to
 *   save much memory.  Do unsigned compares where possible (faster on
 *   Perkin-Elmer)
 *
 * Revision 1.4  84/07/05  03:11:11  thomas
 *   Clean up the code a little and lint it.  (Lint complains about all
 *   the regs used in the asm, but I'm not going to "fix" this.)
 *
 * Revision 1.3  84/07/05  02:06:54  thomas
 *   Minor fixes.
 *
 * Revision 1.2  84/07/05  00:27:27  thomas
 *   Add variable bit length output.
 *
 */

#ifdef _MSC_VER
#	define	WINDOWS
#endif

#ifdef __MINGW32__
#	define	MINGW
#endif

#include	<stdint.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<fcntl.h>
#include	<ctype.h>
#include	<signal.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<errno.h>

#if !defined(DOS) && !defined(WINDOWS)
#	include	<dirent.h>
#	define RECURSIVE 1
#	include	<unistd.h>
#endif

#ifdef UTIME_H
#	include	<utime.h>
#else
	struct utimbuf {
		time_t actime;
		time_t modtime;
	};
#endif

#ifdef	__STDC__
#	define	ARGS(a)				a
#else
#	define	ARGS(a)				()
#endif

#ifndef SIG_TYPE
#	define	SIG_TYPE	void (*)()
#endif

#if defined(AMIGA) || defined(DOS) || defined(MINGW) || defined(WINDOWS)
#	define	chmod(pathname, mode) 0
#	define	chown(pathname, owner, group) 0
#	define	utime(pathname, times) 0
#endif

#if defined(WINDOWS)
#	define isatty(fd) 0
#	define open _open
#	define close _close
#	define read _read
#	define strdup _strdup
#	define unlink _unlink
#	define write _write
#endif

#ifndef	LSTAT
#	define	lstat	stat
#endif

#if defined(DOS) || defined(WINDOWS)
#	define	F_OK	0
static inline int access(const char *pathname, int mode)
{
	struct stat st;
	return lstat(pathname, &st);
}
#endif

#include "patchlevel.h"

#undef	min
#define	min(a,b)	((a>b) ? b : a)

#ifndef	IBUFSIZ
#	define	IBUFSIZ	BUFSIZ	/* Defailt input buffer size							*/
#endif
#ifndef	OBUFSIZ
#	define	OBUFSIZ	BUFSIZ	/* Default output buffer size							*/
#endif

							/* Defines for third byte of header 					*/
#define	MAGIC_1		(char_type)'\037'/* First byte of compressed file				*/
#define	MAGIC_2		(char_type)'\235'/* Second byte of compressed file				*/
#define BIT_MASK	0x1f			/* Mask for 'number of compresssion bits'		*/
									/* Masks 0x20 and 0x40 are free.  				*/
									/* I think 0x20 should mean that there is		*/
									/* a fourth header byte (for expansion).    	*/
#define BLOCK_MODE	0x80			/* Block compresssion if table is full and		*/
									/* compression rate is dropping flush tables	*/

			/* the next two codes should not be changed lightly, as they must not	*/
			/* lie within the contiguous general code space.						*/
#define FIRST	257					/* first free entry 							*/
#define	CLEAR	256					/* table clear output code 						*/

#define INIT_BITS 9			/* initial number of bits/code */

#ifndef SACREDMEM
	/*
 	 * SACREDMEM is the amount of physical memory saved for others; compress
 	 * will hog the rest.
 	 */
#	define SACREDMEM	0
#endif

#ifndef USERMEM
	/*
 	 * Set USERMEM to the maximum amount of physical user memory available
 	 * in bytes.  USERMEM is used to determine the maximum BITS that can be used
 	 * for compression.
	 */
#	define USERMEM 	450000	/* default user memory */
#endif

#ifndef	BYTEORDER
#	define	BYTEORDER	0000
#endif

/*
 * machine variants which require cc -Dmachine:  pdp11, z8000, DOS
 */

#ifdef	DOS			/* PC/XT/AT (8088) processor									*/
#	define	BITS   16	/* 16-bits processor max 12 bits							*/
#	if BITS == 16
#		define	MAXSEG_64K
#	endif
#	undef	BYTEORDER
#	define	BYTEORDER 	4321
#endif /* DOS */

#ifndef	O_BINARY
#	define	O_BINARY	0	/* System has no binary mode							*/
#endif

#ifdef M_XENIX			/* Stupid compiler can't handle arrays with */
#	if BITS == 16 		/* more than 65535 bytes - so we fake it */
# 		define MAXSEG_64K
#	else
#	if BITS > 13			/* Code only handles BITS = 12, 13, or 16 */
#		define BITS	13
#	endif
#	endif
#endif

#ifndef BITS		/* General processor calculate BITS								*/
#	if USERMEM >= (800000+SACREDMEM)
#		define FAST
#	else
#	if USERMEM >= (433484+SACREDMEM)
#		define BITS	16
#	else
#	if USERMEM >= (229600+SACREDMEM)
#		define BITS	15
#	else
#	if USERMEM >= (127536+SACREDMEM)
#		define BITS	14
#   else
#	if USERMEM >= (73464+SACREDMEM)
#		define BITS	13
#	else
#		define BITS	12
#	endif
#	endif
#   endif
#	endif
#	endif
#endif /* BITS */

#ifdef FAST
#	define	HBITS		17			/* 50% occupancy */
#	define	HSIZE	   (1<<HBITS)
#	define	HMASK	   (HSIZE-1)
#	define	HPRIME		 9941
#	define	BITS		   16
#	undef	MAXSEG_64K
#else
#	if BITS == 16
#		define HSIZE	69001		/* 95% occupancy */
#	endif
#	if BITS == 15
#		define HSIZE	35023		/* 94% occupancy */
#	endif
#	if BITS == 14
#		define HSIZE	18013		/* 91% occupancy */
#	endif
#	if BITS == 13
#		define HSIZE	9001		/* 91% occupancy */
#	endif
#	if BITS <= 12
#		define HSIZE	5003		/* 80% occupancy */
#	endif
#endif

#define CHECK_GAP 10000

typedef long int			code_int;

#ifdef SIGNED_COMPARE_SLOW
	typedef unsigned long int	count_int;
	typedef unsigned short int	count_short;
	typedef unsigned long int	cmp_code_int;	/* Cast to make compare faster	*/
#else
	typedef long int	 		count_int;
	typedef long int			cmp_code_int;
#endif

typedef	unsigned char	char_type;

#define ARGVAL() (*++(*argv) || (--argc && *++argv))

#define MAXCODE(n)	(1L << (n))

union	bytes
{
	long	word;
	struct
	{
#if BYTEORDER == 4321
		char_type	b1;
		char_type	b2;
		char_type	b3;
		char_type	b4;
#else
#if BYTEORDER == 1234
		char_type	b4;
		char_type	b3;
		char_type	b2;
		char_type	b1;
#else
#	undef	BYTEORDER
		int				dummy;
#endif
#endif
	} bytes;
} ;
#ifdef BYTEORDER
#define	output(b,o,c,n)	{	char_type	*p = &(b)[(o)>>3];					\
							union bytes i;									\
							i.word = ((long)(c))<<((o)&0x7);				\
							p[0] |= i.bytes.b1;								\
							p[1] |= i.bytes.b2;								\
							p[2] |= i.bytes.b3;								\
							(o) += (n);										\
						}
#else
#define	output(b,o,c,n)	{	char_type	*p = &(b)[(o)>>3];					\
							long		 i = ((long)(c))<<((o)&0x7);		\
							p[0] |= (char_type)(i);							\
							p[1] |= (char_type)(i>>8);						\
							p[2] |= (char_type)(i>>16);						\
							(o) += (n);										\
						}
#endif
#define	input(b,o,c,n,m){	char_type 		*p = &(b)[(o)>>3];				\
							(c) = ((((long)(p[0]))|((long)(p[1])<<8)|		\
									 ((long)(p[2])<<16))>>((o)&0x7))&(m);	\
							(o) += (n);										\
						}

char			*progname;			/* Program name									*/
int 			silent = 0;			/* don't tell me about errors					*/
int 			quiet = 1;			/* don't tell me about compression 				*/
int				do_decomp = 0;		/* Decompress mode								*/
int				force = 0;			/* Force overwrite of files and links			*/
int				nomagic = 0;		/* Use a 3-byte magic number header,			*/
									/* unless old file 								*/
int				maxbits = BITS;		/* user settable max # bits/code 				*/
int 			zcat_flg = 0;		/* Write output on stdout, suppress messages 	*/
int				recursive = 0;  	/* compress directories 						*/
int				exit_code = -1;		/* Exitcode of compress (-1 no file compressed)	*/

char_type		inbuf[IBUFSIZ+64];	/* Input buffer									*/
char_type		outbuf[OBUFSIZ+2048];/* Output buffer								*/

struct stat		infstat;			/* Input file status							*/
char			*ifname;			/* Input filename								*/
int				remove_ofname = 0;	/* Remove output file on a error				*/
char			*ofname = NULL;		/* Output filename								*/
int				fgnd_flag = 0;		/* Running in background (SIGINT=SIGIGN)		*/

long 			bytes_in;			/* Total number of byte from input				*/
long 			bytes_out;			/* Total number of byte to output				*/

/*
 * 8086 & 80286 Has a problem with array bigger than 64K so fake the array
 * For processors with a limited address space and segments.
 */
/*
 * To save much memory, we overlay the table used by compress() with those
 * used by decompress().  The tab_prefix table is the same size and type
 * as the codetab.  The tab_suffix table needs 2**BITS characters.  We
 * get this from the beginning of htab.  The output stack uses the rest
 * of htab, and contains characters.  There is plenty of room for any
 * possible stack (stack used to be 8000 characters).
 */
#ifdef MAXSEG_64K
	count_int htab0[8192];
	count_int htab1[8192];
	count_int htab2[8192];
	count_int htab3[8192];
	count_int htab4[8192];
	count_int htab5[8192];
	count_int htab6[8192];
	count_int htab7[8192];
	count_int htab8[HSIZE-65536];
	count_int * htab[9] = {htab0,htab1,htab2,htab3,htab4,htab5,htab6,htab7,htab8};

	unsigned short code0tab[16384];
	unsigned short code1tab[16384];
	unsigned short code2tab[16384];
	unsigned short code3tab[16384];
	unsigned short code4tab[16384];
	unsigned short * codetab[5] = {code0tab,code1tab,code2tab,code3tab,code4tab};

#	define	htabof(i)			(htab[(i) >> 13][(i) & 0x1fff])
#	define	codetabof(i)		(codetab[(i) >> 14][(i) & 0x3fff])
#	define	tab_prefixof(i)		codetabof(i)
#	define	tab_suffixof(i)		((char_type *)htab[(i)>>15])[(i) & 0x7fff]
#	define	de_stack			((char_type *)(&htab2[8191]))
	void	clear_htab()
	{
		memset(htab0, -1, sizeof(htab0));
		memset(htab1, -1, sizeof(htab1));
		memset(htab2, -1, sizeof(htab2));
		memset(htab3, -1, sizeof(htab3));
		memset(htab4, -1, sizeof(htab4));
		memset(htab5, -1, sizeof(htab5));
		memset(htab6, -1, sizeof(htab6));
		memset(htab7, -1, sizeof(htab7));
		memset(htab8, -1, sizeof(htab8));
	 }
#	define	clear_tab_prefixof()	memset(code0tab, 0, 256);
#else	/* Normal machine */
	count_int		htab[HSIZE];
	unsigned short	codetab[HSIZE];

#	define	htabof(i)				htab[i]
#	define	codetabof(i)			codetab[i]
#	define	tab_prefixof(i)			codetabof(i)
#	define	tab_suffixof(i)			((char_type *)(htab))[i]
#	define	de_stack				((char_type *)&(htab[HSIZE-1]))
#	define	clear_htab()			memset(htab, -1, sizeof(htab))
#	define	clear_tab_prefixof()	memset(codetab, 0, 256);
#endif	/* MAXSEG_64K */

#ifdef FAST
	int primetab[256] =		/* Special secudary hash table.		*/
	{
    	 1013, -1061, 1109, -1181, 1231, -1291, 1361, -1429,
    	 1481, -1531, 1583, -1627, 1699, -1759, 1831, -1889,
    	 1973, -2017, 2083, -2137, 2213, -2273, 2339, -2383,
    	 2441, -2531, 2593, -2663, 2707, -2753, 2819, -2887,
    	 2957, -3023, 3089, -3181, 3251, -3313, 3361, -3449,
    	 3511, -3557, 3617, -3677, 3739, -3821, 3881, -3931,
    	 4013, -4079, 4139, -4219, 4271, -4349, 4423, -4493,
    	 4561, -4639, 4691, -4783, 4831, -4931, 4973, -5023,
    	 5101, -5179, 5261, -5333, 5413, -5471, 5521, -5591,
    	 5659, -5737, 5807, -5857, 5923, -6029, 6089, -6151,
    	 6221, -6287, 6343, -6397, 6491, -6571, 6659, -6709,
    	 6791, -6857, 6917, -6983, 7043, -7129, 7213, -7297,
    	 7369, -7477, 7529, -7577, 7643, -7703, 7789, -7873,
    	 7933, -8017, 8093, -8171, 8237, -8297, 8387, -8461,
    	 8543, -8627, 8689, -8741, 8819, -8867, 8963, -9029,
    	 9109, -9181, 9241, -9323, 9397, -9439, 9511, -9613,
    	 9677, -9743, 9811, -9871, 9941,-10061,10111,-10177,
   		10259,-10321,10399,-10477,10567,-10639,10711,-10789,
   		10867,-10949,11047,-11113,11173,-11261,11329,-11423,
   		11491,-11587,11681,-11777,11827,-11903,11959,-12041,
   		12109,-12197,12263,-12343,12413,-12487,12541,-12611,
   		12671,-12757,12829,-12917,12979,-13043,13127,-13187,
   		13291,-13367,13451,-13523,13619,-13691,13751,-13829,
   		13901,-13967,14057,-14153,14249,-14341,14419,-14489,
   		14557,-14633,14717,-14767,14831,-14897,14983,-15083,
   		15149,-15233,15289,-15359,15427,-15497,15583,-15649,
   		15733,-15791,15881,-15937,16057,-16097,16189,-16267,
   		16363,-16447,16529,-16619,16691,-16763,16879,-16937,
   		17021,-17093,17183,-17257,17341,-17401,17477,-17551,
   		17623,-17713,17791,-17891,17957,-18041,18097,-18169,
   		18233,-18307,18379,-18451,18523,-18637,18731,-18803,
   		18919,-19031,19121,-19211,19273,-19381,19429,-19477
	} ;
#endif

int  	main			ARGS((int,char **));
void  	Usage			ARGS((int));
void  	comprexx		ARGS((const char *));
void  	compdir			ARGS((char *));
void  	compress		ARGS((int,int));
void  	decompress		ARGS((int,int));
void  	read_error		ARGS((void));
void  	write_error		ARGS((void));
void 	abort_compress	ARGS((void));
void  	prratio			ARGS((FILE *,long,long));
void  	about			ARGS((void));

/*****************************************************************
 * TAG( main )
 *
 * Algorithm from "A Technique for High Performance Data Compression",
 * Terry A. Welch, IEEE Computer Vol 17, No 6 (June 1984), pp 8-19.
 *
 * Usage: compress [-dfvc] [-b bits] [file ...]
 * Inputs:
 *   -d:     If given, decompression is done instead.
 *
 *   -c:     Write output on stdout, don't remove original.
 *
 *   -b:     Parameter limits the max number of bits/code.
 *
 *   -f:     Forces output file to be generated, even if one already
 *           exists, and even if no space is saved by compressing.
 *           If -f is not used, the user will be prompted if stdin is
 *           a tty, otherwise, the output file will not be overwritten.
 *
 *   -v:     Write compression statistics
 *
 *   -r:     Recursive. If a filename is a directory, descend
 *           into it and compress everything in it.
 *
 * file ...:
 *           Files to be compressed.  If none specified, stdin is used.
 * Outputs:
 *   file.Z:     Compressed form of file with same mode, owner, and utimes
 *   or stdout   (if stdin used as input)
 *
 * Assumptions:
 *   When filenames are given, replaces with the compressed version
 *   (.Z suffix) only if the file decreases in size.
 *
 * Algorithm:
 *   Modified Lempel-Ziv method (LZW).  Basically finds common
 *   substrings and replaces them with a variable size code.  This is
 *   deterministic, and can be done on the fly.  Thus, the decompression
 *   procedure needs no input table, but tracks the way the table was built.
 */ 
int
main(argc, argv)
	int 	 argc;
	char	*argv[];
	{
		char **filelist;
		char **fileptr;
		int seen_double_dash = 0;

#ifdef SIGINT
		if ((fgnd_flag = (signal(SIGINT, SIG_IGN)) != SIG_IGN))
			signal(SIGINT, (SIG_TYPE)abort_compress);
#endif

#ifdef SIGTERM
		signal(SIGTERM, (SIG_TYPE)abort_compress);
#endif
#ifdef SIGHUP
		signal(SIGHUP, (SIG_TYPE)abort_compress);
#endif

#ifdef COMPATIBLE
    	nomagic = 1;	/* Original didn't have a magic number */
#endif

    	filelist = (char **)malloc(argc*sizeof(char *));
    	if (filelist == NULL)
		{
			fprintf(stderr, "Cannot allocate memory for file list.\n");
			exit (1);
		}
    	fileptr = filelist;
    	*filelist = NULL;

    	if((progname = strrchr(argv[0], '/')) != 0)
			progname++;
		else
			progname = argv[0];

    	if (strcmp(progname, "uncompress") == 0)
			do_decomp = 1;
		else
		if (strcmp(progname, "zcat") == 0)
			do_decomp = zcat_flg = 1;

    	/* Argument Processing
     	 * All flags are optional.
     	 * -V => print Version; debug verbose
     	 * -d => do_decomp
     	 * -v => unquiet
     	 * -f => force overwrite of output file
     	 * -n => no header: useful to uncompress old files
     	 * -b maxbits => maxbits.  If -b is specified, then maxbits MUST be given also.
     	 * -c => cat all output to stdout
     	 * -C => generate output compatible with compress 2.0.
     	 * -r => recursively compress directories
     	 * if a string is left, must be an input filename.
     	 */

    	for (argc--, argv++; argc > 0; argc--, argv++)
		{
			if (strcmp(*argv, "--") == 0)
			{
				seen_double_dash = 1;
				continue;
			}

			if (seen_double_dash == 0 && **argv == '-')
			{/* A flag argument */
		    	while (*++(*argv))
				{/* Process all flags in this arg */
					switch (**argv)
					{
			    	case 'V':
						about();
						break;

					case 's':
						silent = 1;
						quiet = 1;
						break;

			    	case 'v':
						silent = 0;
						quiet = 0;
						break;

			    	case 'd':
						do_decomp = 1;
						break;

			    	case 'f':
			    	case 'F':
						force = 1;
						break;

			    	case 'n':
						nomagic = 1;
						break;

			    	case 'b':
						if (!ARGVAL())
						{
					    	fprintf(stderr, "Missing maxbits\n");
							Usage(1);
						}

						maxbits = atoi(*argv);
						goto nextarg;

		    		case 'c':
						zcat_flg = 1;
						break;

			    	case 'q':
						quiet = 1;
						break;
			    	case 'r':
			    	case 'R':
#ifdef	RECURSIVE
						recursive = 1;
#else
						fprintf(stderr, "%s -r not available (due to missing directory functions)\n", *argv);
#endif
						break;

					case 'h':
						Usage(0);
						break;

			    	default:
						fprintf(stderr, "Unknown flag: '%c'; ", **argv);
						Usage(1);
					}
		    	}
			}
			else
			{
		    	*fileptr++ = *argv;	/* Build input file list */
		    	*fileptr = NULL;
			}

nextarg:	continue;
    	}

    	if (maxbits < INIT_BITS)	maxbits = INIT_BITS;
    	if (maxbits > BITS) 		maxbits = BITS;

    	if (*filelist != NULL)
		{
      		for (fileptr = filelist; *fileptr; fileptr++)
				comprexx(*fileptr);
    	}
		else
		{/* Standard input */
			ifname = "";
			exit_code = 0;
			remove_ofname = 0;

			if (do_decomp == 0)
			{
				compress(0, 1);

				if (zcat_flg == 0 && !quiet)
				{
					fprintf(stderr, "Compression: ");
					prratio(stderr, bytes_in-bytes_out, bytes_in);
					fprintf(stderr, "\n");
				}

				if (bytes_out >= bytes_in && !(force))
					exit_code = 2;
			}
			else
				decompress(0, 1);
		}

		if (recursive && exit_code == -1) {
			fprintf(stderr, "no files processed after recursive search\n");
		}
		exit((exit_code== -1) ? 1:exit_code);
	}

void
Usage(int status)
	{
		fprintf(status ? stderr : stdout, "\
Usage: %s [-dfhvcVr] [-b maxbits] [--] [file ...]\n\
       -d   If given, decompression is done instead.\n\
       -c   Write output on stdout, don't remove original.\n\
       -b   Parameter limits the max number of bits/code.\n", progname);
		fprintf(status ? stderr : stdout, "\
       -f   Forces output file to be generated, even if one already.\n\
            exists, and even if no space is saved by compressing.\n\
            If -f is not used, the user will be prompted if stdin is.\n\
            a tty, otherwise, the output file will not be overwritten.\n\
       -h   This help output.\n\
       -v   Write compression statistics.\n\
       -V   Output version and compile options.\n\
       -r   Recursive. If a filename is a directory, descend\n\
            into it and compress everything in it.\n");

    		exit(status);
	}

void
comprexx(fileptr)
	const char	*fileptr;
	{
		int				 fdin = -1;
		int				 fdout = -1;
		int				 has_z_suffix;
		char			*tempname;
		unsigned long	 namesize = strlen(fileptr);

		/* Create a temp buffer to add/remove the .Z suffix. */
		tempname = malloc(namesize + 3);
		if (tempname == NULL)
		{
			perror("malloc");
			goto error;
		}

		strcpy(tempname,fileptr);
		has_z_suffix = (namesize >= 2 && strcmp(&tempname[namesize - 2], ".Z") == 0);
		errno = 0;

		if (lstat(tempname,&infstat) == -1)
		{
		  	if (do_decomp)
			{
	    		switch (errno)
				{
		    	case ENOENT:	/* file doesn't exist */
	      			/*
	      			** if the given name doesn't end with .Z, try appending one
	      			** This is obviously the wrong thing to do if it's a 
	      			** directory, but it shouldn't do any harm.
	      			*/
					if (!has_z_suffix)
					{
						memcpy(&tempname[namesize], ".Z", 3);
						namesize += 2;
						has_z_suffix = 1;
						errno = 0;
						if (lstat(tempname,&infstat) == -1)
						{
						  	perror(tempname);
							goto error;
						}

						if ((infstat.st_mode & S_IFMT) != S_IFREG)
						{
							fprintf(stderr, "%s: Not a regular file.\n", tempname);
							goto error;
						}
		      		}
		      		else
					{
						perror(tempname);
						goto error;
		      		}

		      		break;

	    		default:
		      		perror(tempname);
					goto error;
	    		}
	  		}
	  		else
			{
	      		perror(tempname);
				goto error;
	  		}
		}

		switch (infstat.st_mode & S_IFMT)
		{
		case S_IFDIR:	/* directory */
#ifdef	RECURSIVE
		  	if (recursive)
		    	compdir(tempname);
		  	else
#endif
			if (!quiet)
		    	fprintf(stderr,"%s is a directory -- ignored\n", tempname);
		  	break;

		case S_IFREG:	/* regular file */
		  	if (do_decomp != 0)
			{/* DECOMPRESSION */
		    	if (!zcat_flg)
				{
					if (!has_z_suffix)
					{
						/* Ignore this scenario in recursive mode: while we
						 * decompress files in this dir, our readdir scan
						 * might turn up those new files.  There is no way
						 * to efficiently handle this on very large dirs,
						 * so we ignore it.  This also allows the user to
						 * easily resume partial decompressions.
						 */
						if (recursive) {
							free(tempname);
							return;
						}

						if (!quiet)
		  					fprintf(stderr,"%s - no .Z suffix\n",tempname);

						goto error;
	      			}
		    	}

				free(ofname);
				ofname = strdup(tempname);
				if (ofname == NULL)
				{
					perror("strdup");
					goto error;
				}

				/* Strip of .Z suffix */
				if (has_z_suffix)
					ofname[namesize - 2] = '\0';
	   		}
	   		else
			{/* COMPRESSION */
		    	if (!zcat_flg)
				{
					if (has_z_suffix)
					{
						/* Ignore this scenario in recursive mode.
						 * See comment above in the decompress path.
						 */
						if (!recursive)
							fprintf(stderr, "%s: already has .Z suffix -- no change\n", tempname);
						free(tempname);
						return;
					}

					if (infstat.st_nlink > 1 && (!force))
					{
						fprintf(stderr, "%s has %jd other links: unchanged\n",
										tempname, (intmax_t)(infstat.st_nlink - 1));
						goto error;
					}
				}

				ofname = malloc(namesize + 3);
				if (ofname == NULL)
				{
					perror("malloc");
					goto error;
				}
				memcpy(ofname, tempname, namesize);
				strcpy(&ofname[namesize], ".Z");
    		}

	    	if ((fdin = open(ifname = tempname, O_RDONLY|O_BINARY)) == -1)
			{
		      	perror(tempname);
				goto error;
	    	}

    		if (zcat_flg == 0)
			{
				if (access(ofname, F_OK) == 0)
				{
					if (!force)
					{
		    			inbuf[0] = 'n';

		    			fprintf(stderr, "%s already exists.\n", ofname);

		    			if (fgnd_flag && isatty(0))
						{
							fprintf(stderr, "Do you wish to overwrite %s (y or n)? ", ofname);
							fflush(stderr);
	
			    			if (read(0, inbuf, 1) > 0)
							{
								if (inbuf[0] != '\n')
								{
									do
									{
										if (read(0, inbuf+1, 1) <= 0)
										{
											perror("stdin");
											break;
										}
									}
									while (inbuf[1] != '\n');
								}
							}
							else
								perror("stdin");
		    			}

		    			if (inbuf[0] != 'y')
						{
							fprintf(stderr, "%s not overwritten\n", ofname);
							goto error;
		    			}
					}

					if (unlink(ofname))
					{
						fprintf(stderr, "Can't remove old output file\n");
						perror(ofname);
						goto error;
					}
				}

		    	if ((fdout = open(ofname, O_WRONLY|O_CREAT|O_EXCL|O_BINARY,0600)) == -1)
				{
			      	perror(tempname);
					goto error;
		    	}

				if(!quiet)
					fprintf(stderr, "%s: ", tempname);

				remove_ofname = 1;
    		}
			else
			{
				fdout = 1;
				remove_ofname = 0;
			}

    		if (do_decomp == 0)
				compress(fdin, fdout);
    		else
				decompress(fdin, fdout);

			close(fdin);

			if (fdout != 1 && close(fdout))
				write_error();

			if ( (bytes_in == 0) && (force == 0 ) )
			{
				if (remove_ofname)
				{
					if(!quiet)
						fprintf(stderr, "No compression -- %s unchanged\n", ifname);
					if (unlink(ofname))	/* Remove input file */
					{
						fprintf(stderr, "\nunlink error (ignored) ");
	    				perror(ofname);
					}
		
					remove_ofname = 0;
					exit_code = 2;
				}
			}
			else
    		if (zcat_flg == 0)
			{
		    	struct utimbuf	timep;

		    	if (!do_decomp && bytes_out >= bytes_in && (!force))
				{/* No compression: remove file.Z */
					if(!quiet)
						fprintf(stderr, "No compression -- %s unchanged\n", ifname);

			    	if (unlink(ofname))
					{
						fprintf(stderr, "unlink error (ignored) ");
						perror(ofname);
					}

					remove_ofname = 0;
					exit_code = 2;
		    	}
				else
				{/* ***** Successful Compression ***** */
					if(!quiet)
					{
						fprintf(stderr, " -- replaced with %s",ofname);

						if (!do_decomp)
						{
							fprintf(stderr, " Compression: ");
							prratio(stderr, bytes_in-bytes_out, bytes_in);
						}

						fprintf(stderr, "\n");
					}

					timep.actime = infstat.st_atime;
					timep.modtime = infstat.st_mtime;

					if (utime(ofname, &timep))
					{
						fprintf(stderr, "\nutime error (ignored) ");
				    	perror(ofname);
					}

					if (chmod(ofname, infstat.st_mode & 07777))		/* Copy modes */
					{
						fprintf(stderr, "\nchmod error (ignored) ");
				    	perror(ofname);
					}

					if (chown(ofname, infstat.st_uid, infstat.st_gid))	/* Copy ownership */
					{
						fprintf(stderr, "\nchown error (ignored) ");
						perror(ofname);
					}

					remove_ofname = 0;

					if (unlink(ifname))	/* Remove input file */
					{
						fprintf(stderr, "\nunlink error (ignored) ");
	    				perror(ifname);
					}
    			}
    		}

			if (exit_code == -1)
				exit_code = 0;

	  		break;

		default:
	  		fprintf(stderr,"%s is not a directory or a regular file - ignored\n",
			  		tempname);
	  		break;
		}

		free(tempname);
		if (!remove_ofname)
		{
			free(ofname);
			ofname = NULL;
		}
		return;

error:
		free(ofname);
		ofname = NULL;
		free(tempname);
		exit_code = 1;
		if (fdin != -1)
			close(fdin);
		if (fdout != -1)
			close(fdout);
	}

#ifdef	RECURSIVE
void
compdir(dir)
	char	*dir;
	{
		struct dirent *dp;
		DIR *dirp;
		char					*nptr;
		char					*fptr;
		unsigned long			 dir_size = strlen(dir);
		/* The +256 is a lazy optimization. We'll resize on demand. */
		unsigned long			 size = dir_size + 256;

		nptr = malloc(size);
		if (nptr == NULL)
		{
			perror("malloc");
			exit_code = 1;
			return;
		}
		memcpy(nptr, dir, dir_size);
		nptr[dir_size] = '/';
		fptr = &nptr[dir_size + 1];

		dirp = opendir(dir);

		if (dirp == NULL)
		{
			free(nptr);
			printf("%s unreadable\n", dir);		/* not stderr! */
			return ;
		}

		while ((dp = readdir(dirp)) != NULL)
		{
			if (dp->d_ino == 0)
				continue;

			if (strcmp(dp->d_name,".") == 0 || strcmp(dp->d_name,"..") == 0)
				continue;

			if (size < dir_size + strlen(dp->d_name) + 2)
			{
				size = dir_size + strlen(dp->d_name) + 2;
				nptr = realloc(nptr, size);
				if (nptr == NULL)
				{
					perror("realloc");
					exit_code = 1;
					break;
				}
				fptr = &nptr[dir_size + 1];
			}

			strcpy(fptr, dp->d_name);
			comprexx(nptr);
  		}

		closedir(dirp);

		free(nptr);
	}
#endif
/*
 * compress fdin to fdout
 *
 * Algorithm:  use open addressing double hashing (no chaining) on the 
 * prefix code / next character combination.  We do a variant of Knuth's
 * algorithm D (vol. 3, sec. 6.4) along with G. Knott's relatively-prime
 * secondary probe.  Here, the modular division first probe is gives way
 * to a faster exclusive-or manipulation.  Also do block compression with
 * an adaptive reset, whereby the code table is cleared when the compression
 * ratio decreases, but after the table fills.  The variable-length output
 * codes are re-sized at this point, and a special CLEAR code is generated
 * for the decompressor.  Late addition:  construct the table according to
 * file size for noticeable speed improvement on small files.  Please direct
 * questions about this implementation to ames!jaw.
 */
void
compress(fdin, fdout)
	int		fdin;
	int		fdout;
	{
		long hp;
		int rpos;
		long fc;
		int outbits;
		int rlop;
		int rsize;
		int stcode;
		code_int free_ent;
		int boff;
		int n_bits;
		int ratio;
		long checkpoint;
		code_int extcode;
		union
		{
			long			code;
			struct
			{
				char_type		c;
				unsigned short	ent;
			} e;
		} fcode;

		ratio = 0;
		checkpoint = CHECK_GAP;
		extcode = MAXCODE(n_bits = INIT_BITS)+1;
		stcode = 1;
		free_ent = FIRST;

		memset(outbuf, 0, sizeof(outbuf));
		bytes_out = 0; bytes_in = 0;
		outbuf[0] = MAGIC_1;
		outbuf[1] = MAGIC_2;
		outbuf[2] = (char)(maxbits | BLOCK_MODE);
		boff = outbits = (3<<3);
		fcode.code = 0;

		clear_htab();

		while ((rsize = read(fdin, inbuf, IBUFSIZ)) > 0)
		{
			if (bytes_in == 0)
			{
				fcode.e.ent = inbuf[0];
				rpos = 1;
			}
			else
				rpos = 0;

			rlop = 0;

			do
			{
				if (free_ent >= extcode && fcode.e.ent < FIRST)
				{
					if (n_bits < maxbits)
					{
						boff = outbits = (outbits-1)+((n_bits<<3)-
								  	((outbits-boff-1+(n_bits<<3))%(n_bits<<3)));
						if (++n_bits < maxbits)
							extcode = MAXCODE(n_bits)+1;
						else
							extcode = MAXCODE(n_bits);
					}
					else
					{
						extcode = MAXCODE(16)+OBUFSIZ;
						stcode = 0;
					}
				}

				if (!stcode && bytes_in >= checkpoint && fcode.e.ent < FIRST)
				{
					long int rat;

					checkpoint = bytes_in + CHECK_GAP;

					if (bytes_in > 0x007fffff)
					{							/* shift will overflow */
						rat = (bytes_out+(outbits>>3)) >> 8;

						if (rat == 0)				/* Don't divide by zero */
							rat = 0x7fffffff;
						else
							rat = bytes_in / rat;
					}
					else
						rat = (bytes_in << 8) / (bytes_out+(outbits>>3));	/* 8 fractional bits */
					if (rat >= ratio)
						ratio = (int)rat;
					else
					{
						ratio = 0;
						clear_htab();
						output(outbuf,outbits,CLEAR,n_bits);
						boff = outbits = (outbits-1)+((n_bits<<3)-
								  	((outbits-boff-1+(n_bits<<3))%(n_bits<<3)));
						extcode = MAXCODE(n_bits = INIT_BITS)+1;
						free_ent = FIRST;
						stcode = 1;
					}
				}

				if (outbits >= (OBUFSIZ<<3))
				{
					if (write(fdout, outbuf, OBUFSIZ) != OBUFSIZ)
						write_error();

					outbits -= (OBUFSIZ<<3);
					boff = -(((OBUFSIZ<<3)-boff)%(n_bits<<3));
					bytes_out += OBUFSIZ;

					memcpy(outbuf, outbuf+OBUFSIZ, (outbits>>3)+1);
					memset(outbuf+(outbits>>3)+1, '\0', OBUFSIZ);
				}

				{
					int i;

					i = rsize-rlop;

					if ((code_int)i > extcode-free_ent)	i = (int)(extcode-free_ent);
					if (i > ((sizeof(outbuf) - 32)*8 - outbits)/n_bits)
						i = ((sizeof(outbuf) - 32)*8 - outbits)/n_bits;
					
					if (!stcode && (long)i > checkpoint-bytes_in)
						i = (int)(checkpoint-bytes_in);

					rlop += i;
					bytes_in += i;
				}

				goto next;
hfound:			fcode.e.ent = codetabof(hp);
next:  			if (rpos >= rlop)
	   				goto endlop;
next2: 			fcode.e.c = inbuf[rpos++];
#ifndef FAST
				{
					code_int i;
					fc = fcode.code;
					hp = (((long)(fcode.e.c)) << (BITS-8)) ^ (long)(fcode.e.ent);

					if ((i = htabof(hp)) == fc)
						goto hfound;

					if (i != -1)
					{
						long disp;

						disp = (HSIZE - hp)-1;	/* secondary hash (after G. Knott) */

						do
						{
							if ((hp -= disp) < 0)	hp += HSIZE;

							if ((i = htabof(hp)) == fc)
								goto hfound;
						}
						while (i != -1);
					}
				}
#else
				{
					long i;
					long p;
					fc = fcode.code;
					hp = ((((long)(fcode.e.c)) << (HBITS-8)) ^ (long)(fcode.e.ent));

					if ((i = htabof(hp)) == fc)	goto hfound;
					if (i == -1)				goto out;

					p = primetab[fcode.e.c];
lookup:				hp = (hp+p)&HMASK;
					if ((i = htabof(hp)) == fc)	goto hfound;
					if (i == -1)				goto out;
					hp = (hp+p)&HMASK;
					if ((i = htabof(hp)) == fc)	goto hfound;
					if (i == -1)				goto out;
					hp = (hp+p)&HMASK;
					if ((i = htabof(hp)) == fc)	goto hfound;
					if (i == -1)				goto out;
					goto lookup;
				}
out:			;
#endif
				output(outbuf,outbits,fcode.e.ent,n_bits);

				{
					long fc = fcode.code;
					fcode.e.ent = fcode.e.c;

					if (stcode)
					{
						codetabof(hp) = (unsigned short)free_ent++;
						htabof(hp) = fc;
					}
				} 

				goto next;

endlop:			if (fcode.e.ent >= FIRST && rpos < rsize)
					goto next2;

				if (rpos > rlop)
				{
					bytes_in += rpos-rlop;
					rlop = rpos;
				}
			}
			while (rlop < rsize);
		}

		if (rsize < 0)
			read_error();

		if (bytes_in > 0)
			output(outbuf,outbits,fcode.e.ent,n_bits);

		if (write(fdout, outbuf, (outbits+7)>>3) != (outbits+7)>>3)
			write_error();

		bytes_out += (outbits+7)>>3;

		return;
	}

/*
 * Decompress stdin to stdout.  This routine adapts to the codes in the
 * file building the "string" table on-the-fly; requiring no table to
 * be stored in the compressed file.  The tables used herein are shared
 * with those of the compress() routine.  See the definitions above.
 */

void
decompress(fdin, fdout)
	int		fdin;
	int		fdout;
	{
		char_type *stackp;
		code_int code;
		int finchar;
		code_int oldcode;
		code_int incode;
		int inbits;
		int posbits;
		int outpos;
		int insize;
		int bitmask;
		code_int free_ent;
		code_int maxcode;
		code_int maxmaxcode;
		int n_bits;
		int rsize;
		int block_mode;

		bytes_in = 0;
		bytes_out = 0;
		insize = 0;

		while (insize < 3 && (rsize = read(fdin, inbuf+insize, IBUFSIZ)) > 0)
			insize += rsize;

		if (insize < 3 || inbuf[0] != MAGIC_1 || inbuf[1] != MAGIC_2)
		{
			if (rsize < 0)
				read_error();

			if (insize > 0)
			{
				fprintf(stderr, "%s: not in compressed format\n",
									(ifname[0] != '\0'? ifname : "stdin"));
				exit_code = 1;
			}

			return ;
		}

		maxbits = inbuf[2] & BIT_MASK;
		block_mode = inbuf[2] & BLOCK_MODE;

		if (maxbits > BITS)
		{
			fprintf(stderr,
					"%s: compressed with %d bits, can only handle %d bits\n",
					(*ifname != '\0' ? ifname : "stdin"), maxbits, BITS);
			exit_code = 4;
			return;
		}

		maxmaxcode = MAXCODE(maxbits);

		bytes_in = insize;
	    maxcode = MAXCODE(n_bits = INIT_BITS)-1;
		bitmask = (1<<n_bits)-1;
		oldcode = -1;
		finchar = 0;
		outpos = 0;
		posbits = 3<<3;

	    free_ent = ((block_mode) ? FIRST : 256);

		clear_tab_prefixof();	/* As above, initialize the first
								   256 entries in the table. */

	    for (code = 255 ; code >= 0 ; --code)
			tab_suffixof(code) = (char_type)code;

		do
		{
resetbuf:	;
			{
				int i;
				int e;
				int o;

				o = posbits >> 3;
				e = o <= insize ? insize - o : 0;

				for (i = 0 ; i < e ; ++i)
					inbuf[i] = inbuf[i+o];

				insize = e;
				posbits = 0;
			}

			if (insize < sizeof(inbuf)-IBUFSIZ)
			{
				if ((rsize = read(fdin, inbuf+insize, IBUFSIZ)) < 0)
					read_error();

				insize += rsize;
			}

			inbits = ((rsize > 0) ? (insize - insize%n_bits)<<3 : 
									(insize<<3)-(n_bits-1));

			while (inbits > posbits)
			{
				if (free_ent > maxcode)
				{
					posbits = ((posbits-1) + ((n_bits<<3) -
									 (posbits-1+(n_bits<<3))%(n_bits<<3)));

					++n_bits;
					if (n_bits == maxbits)
						maxcode = maxmaxcode;
					else
					    maxcode = MAXCODE(n_bits)-1;

					bitmask = (1<<n_bits)-1;
					goto resetbuf;
				}

				input(inbuf,posbits,code,n_bits,bitmask);

				if (oldcode == -1)
				{
					if (code >= 256) {
						fprintf(stderr, "oldcode:-1 code:%i\n", (int)(code));
						fprintf(stderr, "uncompress: corrupt input\n");
						abort_compress();
					}
					outbuf[outpos++] = (char_type)(finchar = (int)(oldcode = code));
					continue;
				}

				if (code == CLEAR && block_mode)
				{
					clear_tab_prefixof();
	    			free_ent = FIRST - 1;
					posbits = ((posbits-1) + ((n_bits<<3) -
								(posbits-1+(n_bits<<3))%(n_bits<<3)));
				    maxcode = MAXCODE(n_bits = INIT_BITS)-1;
					bitmask = (1<<n_bits)-1;
					goto resetbuf;
				}

				incode = code;
			    stackp = de_stack;

				if (code >= free_ent)	/* Special case for KwKwK string.	*/
				{
					if (code > free_ent)
					{
						char_type *p;

						posbits -= n_bits;
						p = &inbuf[posbits>>3];

						fprintf(stderr, "insize:%d posbits:%d inbuf:%02X %02X %02X %02X %02X (%d)\n", insize, posbits,
								p[-1],p[0],p[1],p[2],p[3], (posbits&07));
			    		fprintf(stderr, "uncompress: corrupt input\n");
						abort_compress();
					}

        	    	*--stackp = (char_type)finchar;
		    		code = oldcode;
				}

				while ((cmp_code_int)code >= (cmp_code_int)256)
				{ /* Generate output characters in reverse order */
			    	*--stackp = tab_suffixof(code);
			    	code = tab_prefixof(code);
				}

				*--stackp =	(char_type)(finchar = tab_suffixof(code));

			/* And put them out in forward order */

				{
					int i;

					if (outpos+(i = (de_stack-stackp)) >= OBUFSIZ)
					{
						do
						{
							if (i > OBUFSIZ-outpos) i = OBUFSIZ-outpos;

							if (i > 0)
							{
								memcpy(outbuf+outpos, stackp, i);
								outpos += i;
							}

							if (outpos >= OBUFSIZ)
							{
								if (write(fdout, outbuf, outpos) != outpos)
									write_error();

								outpos = 0;
							}
							stackp+= i;
						}
						while ((i = (de_stack-stackp)) > 0);
					}
					else
					{
						memcpy(outbuf+outpos, stackp, i);
						outpos += i;
					}
				}

				if ((code = free_ent) < maxmaxcode) /* Generate the new entry. */
				{
			    	tab_prefixof(code) = (unsigned short)oldcode;
			    	tab_suffixof(code) = (char_type)finchar;
	    			free_ent = code+1;
				} 

				oldcode = incode;	/* Remember previous code.	*/
			}

			bytes_in += rsize;
	    }
		while (rsize > 0);

		if (outpos > 0 && write(fdout, outbuf, outpos) != outpos)
			write_error();
	}

void
read_error()
	{
		fprintf(stderr, "\nread error on");
	    perror((ifname[0] != '\0') ? ifname : "stdin");
		abort_compress();
	}

void
write_error()
	{
		fprintf(stderr, "\nwrite error on");
	    perror(ofname ? ofname : "stdout");
		abort_compress();
	}

void
abort_compress()
	{
		if (remove_ofname)
	    	unlink(ofname);

		exit(1);
	}

void
prratio(stream, num, den)
	FILE		*stream;
	long int	 num;
	long int	 den;
	{
		int q;			/* Doesn't need to be long */

		if (den > 0)
		{
			if (num > 214748L) 
				q = (int)(num/(den/10000L));	/* 2147483647/10000 */
			else
				q = (int)(10000L*num/den);		/* Long calculations, though */
		}
		else
			q = 10000;

		if (q < 0)
		{
			putc('-', stream);
			q = -q;
		}

		fprintf(stream, "%d.%02d%%", q / 100, q % 100);
	}

void
about()
	{
		printf("Compress version: %s\n", version_id);
		printf("Compile options:\n        ");
#ifdef FAST
		printf("FAST, ");
#endif
#ifdef SIGNED_COMPARE_SLOW
		printf("SIGNED_COMPARE_SLOW, ");
#endif
#ifdef MAXSEG_64K
		printf("MAXSEG_64K, ");
#endif
#ifdef DOS
		printf("DOS, ");
#endif
#ifdef DEBUG
		printf("DEBUG, ");
#endif
#ifdef LSTAT
		printf("LSTAT, ");
#endif
		printf("\n        IBUFSIZ=%d, OBUFSIZ=%d, BITS=%d\n",
			IBUFSIZ, OBUFSIZ, BITS);

		printf("\n\
Author version 5.x (Modernization):\n\
Author version 4.2.4.x (Maintenance):\n\
     Mike Frysinger  (vapier@gmail.com)\n\
\n\
Author version 4.2 (Speed improvement & source cleanup):\n\
     Peter Jannesen  (peter@ncs.nl)\n\
\n\
Author version 4.1 (Added recursive directory compress):\n\
     Dave Mack  (csu@alembic.acs.com)\n\
\n\
Authors version 4.0 (World release in 1985):\n\
     Spencer W. Thomas, Jim McKie, Steve Davies,\n\
     Ken Turkowski, James A. Woods, Joe Orost\n");

		exit(0);
	}