/*
*
*   Copyright (c) 2000-2001, Darren Hiebert
*
*   This source code is released for free distribution under the terms of the
*   GNU General Public License.
*
*   This module contains functions for applying regular expression matching.
*
*   The code for utlizing the Gnu regex package with regards to processing the
*   regex option and checking for regex matches was adapted from routines in
*   Gnu etags.
*/

/*
*   INCLUDE FILES
*/
#include "general.h"	/* must always come first */

#include <string.h>

#if defined (HAVE_REGCOMP) || defined (HAVE_RE_COMPILE_PATTERN)
# include <ctype.h>
# include <stddef.h>
# ifdef HAVE_SYS_TYPES_H
#  include <sys/types.h> /* declare off_t (not known to regex.h on FreeBSD) */
# endif
# include "regex.h"
#endif

#include "debug.h"
#include "entry.h"
#include "main.h"
#include "parse.h"
#include "read.h"

#ifdef HAVE_REGEX

/*
*   MACROS
*/

/* Back-references \0 through \9 */
#define BACK_REFERENCE_COUNT 10

#if defined (HAVE_REGCOMP) && !defined (REGCOMP_BROKEN)
# define POSIX_REGEX
#endif

#define REGEX_NAME "Regex"

/*
*   DATA DECLARATIONS
*/
#if defined (POSIX_REGEX)

struct sKind {
    boolean enabled;
    char letter;
    char* name;
};

enum pType { PTRN_TAG, PTRN_CALLBACK };

typedef struct {
    regex_t *pattern;
    enum pType type;
    union {
	struct {
	    char *name_pattern;
	    struct sKind kind;
	} tag;
	struct {
	    regexCallback function;
	} callback;
    } u;
} regexPattern;

#endif

typedef struct {
    regexPattern *patterns;
    unsigned int count;
} patternSet;

/*
*   DATA DEFINITIONS
*/

static boolean regexBroken = FALSE;

/* Array of pattern sets, indexed by language */
static patternSet* Sets = NULL;
static int SetUpper = -1;	/* upper language index in list */

/*
*   FUNCTION DEFINITIONS
*/

static void clearPatternSet (const langType language)
{
    if (language < SetUpper)
    {
	patternSet* const set = Sets + language;
	unsigned int i;
	for (i = 0  ;  i < set->count  ;  ++i)
	{
#if defined (POSIX_REGEX)
	    regfree (set->patterns [i].pattern);
#endif
	    eFree (set->patterns [i].pattern);
	    set->patterns [i].pattern = NULL;

	    if (set->patterns [i].type == PTRN_TAG)
	    {
		eFree (set->patterns [i].u.tag.name_pattern);
		set->patterns [i].u.tag.name_pattern = NULL;
	    }
	}
	if (set->patterns != NULL)
	    eFree (set->patterns);
	set->patterns = NULL;
	set->count = 0;
    }
}

/*
*   Regex psuedo-parser
*/

static void makeRegexTag (const vString* const name,
			  const struct sKind* const kind)
{
    if (kind->enabled)
    {
	tagEntryInfo e;
	Assert (name != NULL  &&  vStringLength (name) > 0);
	Assert (kind != NULL);
	initTagEntry (&e, vStringValue (name));
	e.kind     = kind->letter;
	e.kindName = kind->name;
	makeTagEntry (&e);
    }
}

/*
*   Regex pattern definition
*/

/* Take a string like "/blah/" and turn it into "blah", making sure
 * that the first and last characters are the same, and handling
 * quoted separator characters.  Actually, stops on the occurrence of
 * an unquoted separator.  Also turns "\t" into a Tab character.
 * Returns pointer to terminating separator.  Works in place.  Null
 * terminates name string.
 */
static char* scanSeparators (char* name)
{
    char sep = name [0];
    char *copyto = name;
    boolean quoted = FALSE;

    for (++name ; *name != '\0' ; ++name)
    {
	if (quoted)
	{
	    if (*name == sep)
		*copyto++ = sep;
	    else if (*name == 't')
		*copyto++ = '\t';
	    else
	    {
		/* Something else is quoted, so preserve the quote. */
		*copyto++ = '\\';
		*copyto++ = *name;
	    }
	    quoted = FALSE;
	}
	else if (*name == '\\')
	    quoted = TRUE;
	else if (*name == sep)
	{
	    break;
	}
	else
	    *copyto++ = *name;
    }
    *copyto = '\0';
    return name;
}

/* Parse `regexp', in form "/regex/name/[k,Kind/]flags" (where the separator
 * character is whatever the first character of `regexp' is), by breaking it
 * up into null terminated strings, removing the separators, and expanding
 * '\t' into tabs. When complete, `regexp' points to the line matching
 * pattern, a pointer to the name matching pattern is written to `name', a
 * pointer to the kinds is written to `kinds' (possibly NULL), and a pointer
 * to the trailing flags is written to `flags'. If the pattern is not in the
 * correct format, a false value is returned.
 */
static boolean parseTagRegex (char* const regexp, char** const name,
			      char** const kinds, char** const flags)
{
    boolean result = FALSE;
    const int separator = (unsigned char) regexp [0];

    *name = scanSeparators (regexp);
    if (*regexp == '\0')
	error (WARNING, "empty regexp");
    else if (**name != separator)
	error (WARNING, "%s: incomplete regexp", regexp);
    else
    {
	char* const third = scanSeparators (*name);
	if (**name == '\0')
	    error (WARNING, "%s: regexp missing name pattern", regexp);
	if ((*name) [strlen (*name) - 1] == '\\')
	    error (WARNING, "error in name pattern: \"%s\"", *name);
	if (*third != separator)
	    error (WARNING, "%s: regexp missing final separator", regexp);
	else
	{
	    char* const fourth = scanSeparators (third);
	    if (*fourth == separator)
	    {
		*kinds = third;
		scanSeparators (fourth);
		*flags = fourth;
	    }
	    else
	    {
		*flags = third;
		*kinds = NULL;
	    }
	    result = TRUE;
	}
    }
    return result;
}

static void addCompiledTagPattern (const langType language,
				   regex_t* const pattern, char* const name,
				   const char kind, char* const kindName)
{
    patternSet* set;
    regexPattern *ptrn;
    if (language > SetUpper)
    {
	int i;
	Sets = xRealloc (Sets, (language + 1), patternSet);
	for (i = SetUpper + 1  ;  i <= language  ;  ++i)
	{
	    Sets [i].patterns = NULL;
	    Sets [i].count = 0;
	}
	SetUpper = language;
    }
    set = Sets + language;
    set->patterns = xRealloc (set->patterns, (set->count + 1), regexPattern);
    ptrn = &set->patterns [set->count];
    set->count += 1;

    ptrn->pattern = pattern;
    ptrn->type    = PTRN_TAG;
    ptrn->u.tag.name_pattern = name;
    ptrn->u.tag.kind.enabled = TRUE;
    ptrn->u.tag.kind.letter  = kind;
    ptrn->u.tag.kind.name    = kindName;
}

static void addCompiledCallbackPattern (const langType language,
					regex_t* const pattern,
					const regexCallback callback)
{
    patternSet* set;
    regexPattern *ptrn;
    if (language > SetUpper)
    {
	int i;
	Sets = xRealloc (Sets, (language + 1), patternSet);
	for (i = SetUpper + 1  ;  i <= language  ;  ++i)
	{
	    Sets [i].patterns = NULL;
	    Sets [i].count = 0;
	}
	SetUpper = language;
    }
    set = Sets + language;
    set->patterns = xRealloc (set->patterns, (set->count + 1), regexPattern);
    ptrn = &set->patterns [set->count];
    set->count += 1;

    ptrn->pattern = pattern;
    ptrn->type    = PTRN_CALLBACK;
    ptrn->u.callback.function = callback;
}

#if defined (POSIX_REGEX)

static regex_t* compileRegex (const char* const regexp, const char* const flags)
{
    int cflags = REG_EXTENDED | REG_NEWLINE;
    regex_t *result = NULL;
    int errcode;
    int i;
    for (i = 0  ; flags != NULL  &&  flags [i] != '\0'  ;  ++i)
    {
	switch ((int) flags [i])
	{
	    case 'b': cflags &= ~REG_EXTENDED; break;
	    case 'e': cflags |= REG_EXTENDED;  break;
	    case 'i': cflags |= REG_ICASE;     break;
	    default: error (WARNING, "unknown regex flag: '%c'", *flags); break;
	}
    }
    result = xMalloc (1, regex_t);
    errcode = regcomp (result, regexp, cflags);
    if (errcode != 0)
    {
	char errmsg[256];
	regerror (errcode, result, errmsg, 256);
	error (WARNING, "%s", errmsg);
	regfree (result);
	eFree (result);
	result = NULL;
    }
    return result;
}

#endif

static void parseKinds (const char* const kinds,
			char* const kind, char** const kindName)
{
    *kind = '\0';
    *kindName = NULL;
    if (kinds == NULL)
    {
	*kind = 'r';
	*kindName = eStrdup ("regex");
    }
    else if (kinds [0] != '\0')
    {
	const char* k = kinds;
	if (k [1] == ','  ||  k [1] == '\0')
	    *kind = *k++;
	if (*k == ',')
	    ++k;
	if (*k != '\0')
	    *kindName = eStrdup (k);
    }
}

static void printRegexKindOption (const regexPattern *pat, unsigned int i)
{
    const struct sKind *const kind = &pat [i].u.tag.kind;
    Assert (pat [i].type == PTRN_TAG);
    printf ("          %c  %s (regex %d)%s\n",
	    kind->letter != '\0' ? kind->letter : '?',
	    kind->name != NULL ? kind->name : "Regex pattern",
	    i + 1, kind->enabled ? "" : " [off]");
}

static void processLanguageRegex (const langType language,
				  const char* const parameter)
{
    if (parameter == NULL  ||  parameter [0] == '\0')
	clearPatternSet (language);
    else if (parameter [0] != '@')
	addLanguageRegex (language, parameter);
    else if (! doesFileExist (parameter + 1))
	error (WARNING, "cannot open regex file");
    else
    {
	const char* regexfile = parameter + 1;
	FILE* const fp = fopen (regexfile, "r");
	if (fp == NULL)
	    error (WARNING | PERROR, regexfile);
	else
	{
	    vString* const regex = vStringNew ();
	    while (readLine (regex, fp))
		addLanguageRegex (language, vStringValue (regex));
	    fclose (fp);
	    vStringDelete (regex);
	}
    }
}

/*
*   Regex pattern matching
*/

#if defined (POSIX_REGEX)

static vString* substitute (const char* const in, const char* out,
			    const int nmatch, const regmatch_t* const pmatch)
{
    vString* result = vStringNew ();
    const char* p;
    for (p = out  ;  *p != '\0'  ;  p++)
    {
	if (*p == '\\'  &&  isdigit ((int) *++p))
	{
	    const int dig = *p - '0';
	    if (0 < dig  &&  dig < nmatch  &&  pmatch [dig].rm_so != -1)
	    {
		const int diglen = pmatch [dig].rm_eo - pmatch [dig].rm_so;
		vStringNCatS (result, in + pmatch [dig].rm_so, diglen);
	    }
	}
	else if (*p != '\n'  &&  *p != '\r')
	    vStringPut (result, *p);
    }
    vStringTerminate (result);
    return result;
}

static void matchTagPattern (const vString* const line,
			     const regexPattern* const patbuf,
			     const regmatch_t* const pmatch)
{
    vString *const name = substitute (vStringValue (line),
	    patbuf->u.tag.name_pattern, BACK_REFERENCE_COUNT, pmatch);
    vStringStripLeading (name);
    vStringStripTrailing (name);
    if (vStringLength (name) > 0)
	makeRegexTag (name, &patbuf->u.tag.kind);
    else
	error (WARNING, "%s:%ld: null expansion of name pattern \"%s\"",
	    getInputFileName (), getInputLineNumber (),
	    patbuf->u.tag.name_pattern);
    vStringDelete (name);
}

static void matchCallbackPattern (const vString* const line,
				  const regexPattern* const patbuf,
				  const regmatch_t* const pmatch)
{
    regexMatch matches [BACK_REFERENCE_COUNT];
    unsigned int count = 0;
    int i;
    for (i = 0  ;  i < BACK_REFERENCE_COUNT  &&  pmatch [i].rm_so != -1  ;  ++i)
    {
	matches [i].start  = pmatch [i].rm_so;
	matches [i].length = pmatch [i].rm_eo - pmatch [i].rm_so;
	++count;
    }
    patbuf->u.callback.function (vStringValue (line), matches, count);
}

static void matchRegexPattern (const vString* const line,
			       const regexPattern* const patbuf)
{
    regmatch_t pmatch [BACK_REFERENCE_COUNT];
    const int match = regexec (patbuf->pattern, vStringValue (line),
			       BACK_REFERENCE_COUNT, pmatch, 0);
    if (match == 0)
    {
	if (patbuf->type == PTRN_TAG)
	    matchTagPattern (line, patbuf, pmatch);
	else if (patbuf->type == PTRN_CALLBACK)
	    matchCallbackPattern (line, patbuf, pmatch);
    }
}

#endif

/* PUBLIC INTERFACE */

/* Match against all patterns for specified language. */
extern void matchRegex (const vString* const line, const langType language)
{
    if (language != LANG_IGNORE  &&  language <= SetUpper  &&
	Sets [language].count > 0)
    {
	const patternSet* const set = Sets + language;
	unsigned int i;
	for (i = 0  ;  i < set->count  ;  ++i)
	    matchRegexPattern (line, set->patterns + i);
    }
}

extern void findRegexTags (void)
{
    /* merely read all lines of the file */
    while (fileReadLine () != NULL)
	;
}

#endif /* HAVE_REGEX */

extern void addTagRegex (const langType __unused__ language,
			 const char* const __unused__ regex,
			 const char* const __unused__ name,
			 const char* const __unused__ kinds,
			 const char* const __unused__ flags)
{
#ifdef HAVE_REGEX
    Assert (regex != NULL);
    Assert (name != NULL);
    if (! regexBroken)
    {
	regex_t* const cp = compileRegex (regex, flags);
	if (cp != NULL)
	{
	    char kind;
	    char* kindName;
	    parseKinds (kinds, &kind, &kindName);
	    addCompiledTagPattern (language, cp, eStrdup (name),
				    kind, kindName);
	}
    }
#endif
}

extern void addCallbackRegex (const langType __unused__ language,
			      const char* const __unused__ regex,
			      const char* const __unused__ flags,
			      const regexCallback __unused__ callback)
{
#ifdef HAVE_REGEX
    Assert (regex != NULL);
    if (! regexBroken)
    {
	regex_t* const cp = compileRegex (regex, flags);
	if (cp != NULL)
	    addCompiledCallbackPattern (language, cp, callback);
    }
#endif
}

extern void addLanguageRegex (const langType __unused__ language,
			      const char* const __unused__ regex)
{
#ifdef HAVE_REGEX
    if (! regexBroken)
    {
	char *const regex_pat = eStrdup (regex);
	char *name, *kinds, *flags;
	if (parseTagRegex (regex_pat, &name, &kinds, &flags))
	{
	    addTagRegex (language, regex_pat, name, kinds, flags);
	    eFree (regex_pat);
	}
    }
#endif
}

/*
*   Regex option parsing
*/

extern boolean processRegexOption (const char *const option,
				   const char *const __unused__ parameter)
{
    boolean handled = FALSE;
    const char* const dash = strchr (option, '-');
    if (dash != NULL  &&  strncmp (option, "regex", dash - option) == 0)
    {
#ifdef HAVE_REGEX
	langType language;
	language = getNamedLanguage (dash + 1);
	if (language == LANG_IGNORE)
	    error (WARNING, "unknown language in --%s option", option);
	else
	    processLanguageRegex (language, parameter);
#else
	error (WARNING, "regex support not available; required for --%s option",
	   option);
#endif
	handled = TRUE;
    }
    return handled;
}

extern void disableRegexKinds (const langType __unused__ language)
{
#ifdef HAVE_REGEX
    if (language <= SetUpper  &&  Sets [language].count > 0)
    {
	patternSet* const set = Sets + language;
	unsigned int i;
	for (i = 0  ;  i < set->count  ;  ++i)
	    if (set->patterns [i].type == PTRN_TAG)
		set->patterns [i].u.tag.kind.enabled = FALSE;
    }
#endif
}

extern boolean enableRegexKind (const langType __unused__ language,
				const int __unused__ kind,
				const boolean __unused__ mode)
{
    boolean result = FALSE;
#ifdef HAVE_REGEX
    if (language <= SetUpper  &&  Sets [language].count > 0)
    {
	patternSet* const set = Sets + language;
	unsigned int i;
	for (i = 0  ;  i < set->count  ;  ++i)
	    if (set->patterns [i].type == PTRN_TAG &&
		set->patterns [i].u.tag.kind.letter == kind)
	    {
		set->patterns [i].u.tag.kind.enabled = mode;
		result = TRUE;
	    }
    }
#endif
    return result;
}

extern void printRegexKindOptions (const langType __unused__ language)
{
#ifdef HAVE_REGEX
    if (language <= SetUpper  &&  Sets [language].count > 0)
    {
	patternSet* const set = Sets + language;
	unsigned int i;
	for (i = 0  ;  i < set->count  ;  ++i)
	    if (set->patterns [i].type == PTRN_TAG)
		printRegexKindOption (set->patterns, i);
    }
#endif
}

extern void freeRegexResources (void)
{
#ifdef HAVE_REGEX
    int i;
    for (i = 0  ;  i <= SetUpper  ;  ++i)
	clearPatternSet (i);
    if (Sets != NULL)
	eFree (Sets);
    Sets = NULL;
    SetUpper = -1;
#endif
}

/* Check for broken regcomp() on Cygwin */
extern void checkRegex (void)
{
#if defined (HAVE_REGEX) && defined (CHECK_REGCOMP)
    regex_t patbuf;
    int errcode;
    if (regcomp (&patbuf, "/hello/", 0) != 0)
    {
	error (WARNING, "Disabling broken regex");
	regexBroken = TRUE;
    }
#endif
}

/* vi:set tabstop=8 shiftwidth=4: */