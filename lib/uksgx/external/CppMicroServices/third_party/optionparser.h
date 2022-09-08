/*
 * The Lean Mean C++ Option Parser
 *
 * Copyright (C) 2012-2017 Matthias S. Benkmann
 *
 * The "Software" in the following 2 paragraphs refers to this file containing
 * the code to The Lean Mean C++ Option Parser.
 * The "Software" does NOT refer to any other files which you
 * may have received alongside this file (e.g. as part of a larger project that
 * incorporates The Lean Mean C++ Option Parser).
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software, to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the following
 * conditions:
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/*
 * NOTE: It is recommended that you read the processed HTML doxygen documentation
 * rather than this source. If you don't know doxygen, it's like javadoc for C++.
 * If you don't want to install doxygen you can find a copy of the processed
 * documentation at
 *
 * http://optionparser.sourceforge.net/
 *
 */

/**
 * @file
 *
 * @brief This is the only file required to use The Lean Mean C++ Option Parser.
 *        Just \#include it and you're set.
 *
 * The Lean Mean C++ Option Parser handles the program's command line arguments
 * (argc, argv).
 * It supports the short and long option formats of getopt(), getopt_long()
 * and getopt_long_only() but has a more convenient interface.
 *
 * @par Feedback:
 * Send questions, bug reports, feature requests etc. to: <tt><b>optionparser-feedback(a)lists.sourceforge.net</b></tt>
 *
 * @par Highlights:
 * <ul style="padding-left:1em;margin-left:0">
 * <li> It is a header-only library. Just <code>\#include "optionparser.h"</code> and you're set.
 * <li> It is freestanding. There are no dependencies whatsoever, not even the
 *      C or C++ standard library.
 * <li> It has a usage message formatter that supports column alignment and
 *      line wrapping. This aids localization because it adapts to
 *      translated strings that are shorter or longer (even if they contain
 *      Asian wide characters).
 * <li> Unlike getopt() and derivatives it doesn't force you to loop through
 *     options sequentially. Instead you can access options directly like this:
 *     <ul style="margin-top:.5em">
 *     <li> Test for presence of a switch in the argument vector:
 *      @code if ( options[QUIET] ) ... @endcode
 *     <li> Evaluate --enable-foo/--disable-foo pair where the last one used wins:
 *     @code if ( options[FOO].last()->type() == DISABLE ) ... @endcode
 *     <li> Cumulative option (-v verbose, -vv more verbose, -vvv even more verbose):
 *     @code int verbosity = options[VERBOSE].count(); @endcode
 *     <li> Iterate over all --file=&lt;fname> arguments:
 *     @code for (Option* opt = options[FILE]; opt; opt = opt->next())
 *   fname = opt->arg; ... @endcode
 *     <li> If you really want to, you can still process all arguments in order:
 *     @code
 *   for (int i = 0; i < p.optionsCount(); ++i) {
 *     Option& opt = buffer[i];
 *     switch(opt.index()) {
 *       case HELP:    ...
 *       case VERBOSE: ...
 *       case FILE:    fname = opt.arg; ...
 *       case UNKNOWN: ...
 *     @endcode
 *     </ul>
 * </ul> @n
 * Despite these features the code size remains tiny.
 * It is smaller than <a href="http://uclibc.org">uClibc</a>'s GNU getopt() and just a
 * couple 100 bytes larger than uClibc's SUSv3 getopt(). @n
 * (This does not include the usage formatter, of course. But you don't have to use that.)
 *
 * @par Download:
 * Tarball with examples and test programs:
 * <a style="font-size:larger;font-weight:bold" href="http://sourceforge.net/projects/optionparser/files/optionparser-1.7.tar.gz/download">optionparser-1.7.tar.gz</a> @n
 * Just the header (this is all you really need):
 * <a style="font-size:larger;font-weight:bold" href="http://optionparser.sourceforge.net/optionparser.h">optionparser.h</a>
 *
 * @par Changelog:
 * <b>Version 1.7:</b> Work on const-correctness. @n
 * <b>Version 1.6:</b> Fix for MSC compiler. @n
 * <b>Version 1.5:</b> Fixed 2 warnings about potentially uninitialized variables. @n
 *                     Added const version of Option::next(). @n
 * <b>Version 1.4:</b> Fixed 2 printUsage() bugs that messed up output with small COLUMNS values. @n
 * <b>Version 1.3:</b> Compatible with Microsoft Visual C++. @n
 * <b>Version 1.2:</b> Added @ref option::Option::namelen "Option::namelen" and removed the extraction
 *                     of short option characters into a special buffer. @n
 *                     Changed @ref option::Arg::Optional "Arg::Optional" to accept arguments if they are attached
 *                     rather than separate. This is what GNU getopt() does and how POSIX recommends
 *                     utilities should interpret their arguments.@n
 * <b>Version 1.1:</b> Optional mode with argument reordering as done by GNU getopt(), so that
 *                     options and non-options can be mixed. See
 *                     @ref option::Parser::parse() "Parser::parse()".
 *
 *
 * @par Example program:
 * (Note: @c option::* identifiers are links that take you to their documentation.)
 * @code
 * #error EXAMPLE SHORTENED FOR READABILITY. BETTER EXAMPLES ARE IN THE .TAR.GZ!
 * #include <iostream>
 * #include "optionparser.h"
 *
 * enum  optionIndex { UNKNOWN, HELP, PLUS };
 * const option::Descriptor usage[] =
 * {
 *  {UNKNOWN, 0,"" , ""    ,option::Arg::None, "USAGE: example [options]\n\n"
 *                                             "Options:" },
 *  {HELP,    0,"" , "help",option::Arg::None, "  --help  \tPrint usage and exit." },
 *  {PLUS,    0,"p", "plus",option::Arg::None, "  --plus, -p  \tIncrement count." },
 *  {UNKNOWN, 0,"" ,  ""   ,option::Arg::None, "\nExamples:\n"
 *                                             "  example --unknown -- --this_is_no_option\n"
 *                                             "  example -unk --plus -ppp file1 file2\n" },
 *  {0,0,0,0,0,0}
 * };
 *
 * int main(int argc, char* argv[])
 * {
 *   argc-=(argc>0); argv+=(argc>0); // skip program name argv[0] if present
 *   option::Stats  stats(usage, argc, argv);
 *   option::Option options[stats.options_max], buffer[stats.buffer_max];
 *   option::Parser parse(usage, argc, argv, options, buffer);
 *
 *   if (parse.error())
 *     return 1;
 *
 *   if (options[HELP] || argc == 0) {
 *     option::printUsage(std::cout, usage);
 *     return 0;
 *   }
 *
 *   std::cout << "--plus count: " <<
 *     options[PLUS].count() << "\n";
 *
 *   for (option::Option* opt = options[UNKNOWN]; opt; opt = opt->next())
 *     std::cout << "Unknown option: " << opt->name << "\n";
 *
 *   for (int i = 0; i < parse.nonOptionsCount(); ++i)
 *     std::cout << "Non-option #" << i << ": " << parse.nonOption(i) << "\n";
 * }
 * @endcode
 *
 * @par Option syntax:
 * @li The Lean Mean C++ Option Parser follows POSIX <code>getopt()</code> conventions and supports
 *     GNU-style <code>getopt_long()</code> long options as well as Perl-style single-minus
 *     long options (<code>getopt_long_only()</code>).
 * @li short options have the format @c -X where @c X is any character that fits in a char.
 * @li short options can be grouped, i.e. <code>-X -Y</code> is equivalent to @c -XY.
 * @li a short option may take an argument either separate (<code>-X foo</code>) or
 *     attached (@c -Xfoo). You can make the parser accept the additional format @c -X=foo by
 *     registering @c X as a long option (in addition to being a short option) and
 *     enabling single-minus long options.
 * @li an argument-taking short option may be grouped if it is the last in the group, e.g.
 *     @c -ABCXfoo or <code> -ABCX foo </code> (@c foo is the argument to the @c -X option).
 * @li a lone minus character @c '-' is not treated as an option. It is customarily used where
 *     a file name is expected to refer to stdin or stdout.
 * @li long options have the format @c --option-name.
 * @li the option-name of a long option can be anything and include any characters.
 *     Even @c = characters will work, but don't do that.
 * @li [optional] long options may be abbreviated as long as the abbreviation is unambiguous.
 *     You can set a minimum length for abbreviations.
 * @li [optional] long options may begin with a single minus. The double minus form is always
 *     accepted, too.
 * @li a long option may take an argument either separate (<code> --option arg </code>) or
 *     attached (<code> --option=arg </code>). In the attached form the equals sign is mandatory.
 * @li an empty string can be passed as an attached long option argument: <code> --option-name= </code>.
 *     Note the distinction between an empty string as argument and no argument at all.
 * @li an empty string is permitted as separate argument to both long and short options.
 * @li Arguments to both short and long options may start with a @c '-' character. E.g.
 *     <code> -X-X </code>, <code>-X -X</code> or <code> --long-X=-X </code>. If @c -X
 *     and @c --long-X take an argument, that argument will be @c "-X" in all 3 cases.
 * @li If using the built-in @ref option::Arg::Optional "Arg::Optional", optional arguments must
 *     be attached.
 * @li the special option @c -- (i.e. without a name) terminates the list of
 *     options. Everything that follows is a non-option argument, even if it starts with
 *     a @c '-' character. The @c -- itself will not appear in the parse results.
 * @li the first argument that doesn't start with @c '-' or @c '--' and does not belong to
 *     a preceding argument-taking option, will terminate the option list and is the
 *     first non-option argument. All following command line arguments are treated as
 *     non-option arguments, even if they start with @c '-' . @n
 *     NOTE: This behaviour is mandated by POSIX, but GNU getopt() only honours this if it is
 *     explicitly requested (e.g. by setting POSIXLY_CORRECT). @n
 *     You can enable the GNU behaviour by passing @c true as first argument to
 *     e.g. @ref option::Parser::parse() "Parser::parse()".
 * @li Arguments that look like options (i.e. @c '-' followed by at least 1 character) but
 *     aren't, are NOT treated as non-option arguments. They are treated as unknown options and
 *     are collected into a list of unknown options for error reporting. @n
 *     This means that in order to pass a first non-option
 *     argument beginning with the minus character it is required to use the
 *     @c -- special option, e.g.
 *     @code
 *     program -x -- --strange-filename
 *     @endcode
 *     In this example, @c --strange-filename is a non-option argument. If the @c --
 *     were omitted, it would be treated as an unknown option. @n
 *     See @ref option::Descriptor::longopt for information on how to collect unknown options.
 *
 */

#ifndef OPTIONPARSER_H_
#define OPTIONPARSER_H_

#ifdef _MSC_VER
#include <intrin.h>
#pragma intrinsic(_BitScanReverse)
#endif

/** @brief The namespace of The Lean Mean C++ Option Parser. */
namespace option
{

#ifdef _MSC_VER
struct MSC_Builtin_CLZ
{
  static int builtin_clz(unsigned x)
  {
    unsigned long index;
    _BitScanReverse(&index, x);
    return 32-index; // int is always 32bit on Windows, even for target x64
  }
};
#define __builtin_clz(x) MSC_Builtin_CLZ::builtin_clz(x)
#endif

class Option;

/**
 * @brief Possible results when checking if an argument is valid for a certain option.
 *
 * In the case that no argument is provided for an option that takes an
 * optional argument, return codes @c ARG_OK and @c ARG_IGNORE are equivalent.
 */
enum ArgStatus
{
  //! The option does not take an argument.
  ARG_NONE,
  //! The argument is acceptable for the option.
  ARG_OK,
  //! The argument is not acceptable but that's non-fatal because the option's argument is optional.
  ARG_IGNORE,
  //! The argument is not acceptable and that's fatal.
  ARG_ILLEGAL
};

/**
 * @brief Signature of functions that check if an argument is valid for a certain type of option.
 *
 * Every Option has such a function assigned in its Descriptor.
 * @code
 * Descriptor usage[] = { {UNKNOWN, 0, "", "", Arg::None, ""}, ... };
 * @endcode
 *
 * A CheckArg function has the following signature:
 * @code ArgStatus CheckArg(const Option& option, bool msg); @endcode
 *
 * It is used to check if a potential argument would be acceptable for the option.
 * It will even be called if there is no argument. In that case @c option.arg will be @c NULL.
 *
 * If @c msg is @c true and the function determines that an argument is not acceptable and
 * that this is a fatal error, it should output a message to the user before
 * returning @ref ARG_ILLEGAL. If @c msg is @c false the function should remain silent (or you
 * will get duplicate messages).
 *
 * See @ref ArgStatus for the meaning of the return values.
 *
 * While you can provide your own functions,
 * often the following pre-defined checks (which never return @ref ARG_ILLEGAL) will suffice:
 *
 * @li @c Arg::None @copybrief Arg::None
 * @li @c Arg::Optional @copybrief Arg::Optional
 *
 */
typedef ArgStatus (*CheckArg)(const Option& option, bool msg);

/**
 * @brief Describes an option, its help text (usage) and how it should be parsed.
 *
 * The main input when constructing an option::Parser is an array of Descriptors.

 * @par Example:
 * @code
 * enum OptionIndex {CREATE, ...};
 * enum OptionType {DISABLE, ENABLE, OTHER};
 *
 * const option::Descriptor usage[] = {
 *   { CREATE,                                            // index
 *     OTHER,                                             // type
 *     "c",                                               // shortopt
 *     "create",                                          // longopt
 *     Arg::None,                                         // check_arg
 *     "--create  Tells the program to create something." // help
 *   }
 *   , ...
 * };
 * @endcode
 */
struct Descriptor
{
  /**
   * @brief Index of this option's linked list in the array filled in by the parser.
   *
   * Command line options whose Descriptors have the same index will end up in the same
   * linked list in the order in which they appear on the command line. If you have
   * multiple long option aliases that refer to the same option, give their descriptors
   * the same @c index.
   *
   * If you have options that mean exactly opposite things
   * (e.g. @c --enable-foo and @c --disable-foo ), you should also give them the same
   * @c index, but distinguish them through different values for @ref type.
   * That way they end up in the same list and you can just take the last element of the
   * list and use its type. This way you get the usual behaviour where switches later
   * on the command line override earlier ones without having to code it manually.
   *
   * @par Tip:
   * Use an enum rather than plain ints for better readability, as shown in the example
   * at Descriptor.
   */
  const unsigned index;

  /**
   * @brief Used to distinguish between options with the same @ref index.
   * See @ref index for details.
   *
   * It is recommended that you use an enum rather than a plain int to make your
   * code more readable.
   */
  const int type;

  /**
   * @brief Each char in this string will be accepted as a short option character.
   *
   * The string must not include the minus character @c '-' or you'll get undefined
   * behaviour.
   *
   * If this Descriptor should not have short option characters, use the empty
   * string "". NULL is not permitted here!
   *
   * See @ref longopt for more information.
   */
  const char* const shortopt;

  /**
   * @brief The long option name (without the leading @c -- ).
   *
   * If this Descriptor should not have a long option name, use the empty
   * string "". NULL is not permitted here!
   *
   * While @ref shortopt allows multiple short option characters, each
   * Descriptor can have only a single long option name. If you have multiple
   * long option names referring to the same option use separate Descriptors
   * that have the same @ref index and @ref type. You may repeat
   * short option characters in such an alias Descriptor but there's no need to.
   *
   * @par Dummy Descriptors:
   * You can use dummy Descriptors with an
   * empty string for both @ref shortopt and @ref longopt to add text to
   * the usage that is not related to a specific option. See @ref help.
   * The first dummy Descriptor will be used for unknown options (see below).
   *
   * @par Unknown Option Descriptor:
   * The first dummy Descriptor in the list of Descriptors,
   * whose @ref shortopt and @ref longopt are both the empty string, will be used
   * as the Descriptor for unknown options. An unknown option is a string in
   * the argument vector that is not a lone minus @c '-' but starts with a minus
   * character and does not match any Descriptor's @ref shortopt or @ref longopt. @n
   * Note that the dummy descriptor's @ref check_arg function @e will be called and
   * its return value will be evaluated as usual. I.e. if it returns @ref ARG_ILLEGAL
   * the parsing will be aborted with <code>Parser::error()==true</code>. @n
   * if @c check_arg does not return @ref ARG_ILLEGAL the descriptor's
   * @ref index @e will be used to pick the linked list into which
   * to put the unknown option. @n
   * If there is no dummy descriptor, unknown options will be dropped silently.
   *
   */
  const char* const longopt;

  /**
   * @brief For each option that matches @ref shortopt or @ref longopt this function
   * will be called to check a potential argument to the option.
   *
   * This function will be called even if there is no potential argument. In that case
   * it will be passed @c NULL as @c arg parameter. Do not confuse this with the empty
   * string.
   *
   * See @ref CheckArg for more information.
   */
  const CheckArg check_arg;

  /**
   * @brief The usage text associated with the options in this Descriptor.
   *
   * You can use option::printUsage() to format your usage message based on
   * the @c help texts. You can use dummy Descriptors where
   * @ref shortopt and @ref longopt are both the empty string to add text to
   * the usage that is not related to a specific option.
   *
   * See option::printUsage() for special formatting characters you can use in
   * @c help to get a column layout.
   *
   * @attention
   * Must be UTF-8-encoded. If your compiler supports C++11 you can use the "u8"
   * prefix to make sure string literals are properly encoded.
   */
  const char* help;
};

/**
 * @brief A parsed option from the command line together with its argument if it has one.
 *
 * The Parser chains all parsed options with the same Descriptor::index together
 * to form a linked list. This allows you to easily implement all of the common ways
 * of handling repeated options and enable/disable pairs.
 *
 * @li Test for presence of a switch in the argument vector:
 *      @code if ( options[QUIET] ) ... @endcode
 * @li Evaluate --enable-foo/--disable-foo pair where the last one used wins:
 *     @code if ( options[FOO].last()->type() == DISABLE ) ... @endcode
 * @li Cumulative option (-v verbose, -vv more verbose, -vvv even more verbose):
 *     @code int verbosity = options[VERBOSE].count(); @endcode
 * @li Iterate over all --file=&lt;fname> arguments:
 *     @code for (Option* opt = options[FILE]; opt; opt = opt->next())
 *   fname = opt->arg; ... @endcode
 */
class Option
{
  Option* next_;
  Option* prev_;
public:
  /**
   * @brief Pointer to this Option's Descriptor.
   *
   * Remember that the first dummy descriptor (see @ref Descriptor::longopt) is used
   * for unknown options.
   *
   * @attention
   * @c desc==NULL signals that this Option is unused. This is the default state of
   * elements in the result array. You don't need to test @c desc explicitly. You
   * can simply write something like this:
   * @code
   * if (options[CREATE])
   * {
   *   ...
   * }
   * @endcode
   * This works because of <code> operator const Option*() </code>.
   */
  const Descriptor* desc;

  /**
   * @brief The name of the option as used on the command line.
   *
   * The main purpose of this string is to be presented to the user in messages.
   *
   * In the case of a long option, this is the actual @c argv pointer, i.e. the first
   * character is a '-'. In the case of a short option this points to the option
   * character within the @c argv string.
   *
   * Note that in the case of a short option group or an attached option argument, this
   * string will contain additional characters following the actual name. Use @ref namelen
   * to filter out the actual option name only.
   *
   */
  const char* name;

  /**
   * @brief Pointer to this Option's argument (if any).
   *
   * NULL if this option has no argument. Do not confuse this with the empty string which
   * is a valid argument.
   */
  const char* arg;

  /**
   * @brief The length of the option @ref name.
   *
   * Because @ref name points into the actual @c argv string, the option name may be
   * followed by more characters (e.g. other short options in the same short option group).
   * This value is the number of bytes (not characters!) that are part of the actual name.
   *
   * For a short option, this length is always 1. For a long option this length is always
   * at least 2 if single minus long options are permitted and at least 3 if they are disabled.
   *
   * @note
   * In the pathological case of a minus within a short option group (e.g. @c -xf-z), this
   * length is incorrect, because this case will be misinterpreted as a long option and the
   * name will therefore extend to the string's 0-terminator or a following '=" character
   * if there is one. This is irrelevant for most uses of @ref name and @c namelen. If you
   * really need to distinguish the case of a long and a short option, compare @ref name to
   * the @c argv pointers. A long option's @c name is always identical to one of them,
   * whereas a short option's is never.
   */
  int namelen;

  /**
   * @brief Returns Descriptor::type of this Option's Descriptor, or 0 if this Option
   * is invalid (unused).
   *
   * Because this method (and last(), too) can be used even on unused Options with desc==0, you can (provided
   * you arrange your types properly) switch on type() without testing validity first.
   * @code
   * enum OptionType { UNUSED=0, DISABLED=0, ENABLED=1 };
   * enum OptionIndex { FOO };
   * const Descriptor usage[] = {
   *   { FOO, ENABLED,  "", "enable-foo",  Arg::None, 0 },
   *   { FOO, DISABLED, "", "disable-foo", Arg::None, 0 },
   *   { 0, 0, 0, 0, 0, 0 } };
   * ...
   * switch(options[FOO].last()->type()) // no validity check required!
   * {
   *   case ENABLED: ...
   *   case DISABLED: ...  // UNUSED==DISABLED !
   * }
   * @endcode
   */
  int type() const
  {
    return desc == 0 ? 0 : desc->type;
  }

  /**
   * @brief Returns Descriptor::index of this Option's Descriptor, or -1 if this Option
   * is invalid (unused).
   */
  int index() const
  {
    return desc == 0 ? -1 : static_cast<int>( desc->index );
  }

  /**
   * @brief Returns the number of times this Option (or others with the same Descriptor::index)
   * occurs in the argument vector.
   *
   * This corresponds to the number of elements in the linked list this Option is part of.
   * It doesn't matter on which element you call count(). The return value is always the same.
   *
   * Use this to implement cumulative options, such as -v, -vv, -vvv for
   * different verbosity levels.
   *
   * Returns 0 when called for an unused/invalid option.
   */
  int count() const
  {
    int c = (desc == 0 ? 0 : 1);
    const Option* p = first();
    while (!p->isLast())
    {
      ++c;
      p = p->next_;
    };
    return c;
  }

  /**
   * @brief Returns true iff this is the first element of the linked list.
   *
   * The first element in the linked list is the first option on the command line
   * that has the respective Descriptor::index value.
   *
   * Returns true for an unused/invalid option.
   */
  bool isFirst() const
  {
    return isTagged(prev_);
  }

  /**
   * @brief Returns true iff this is the last element of the linked list.
   *
   * The last element in the linked list is the last option on the command line
   * that has the respective Descriptor::index value.
   *
   * Returns true for an unused/invalid option.
   */
  bool isLast() const
  {
    return isTagged(next_);
  }

  /**
   * @brief Returns a pointer to the first element of the linked list.
   *
   * Use this when you want the first occurrence of an option on the command line to
   * take precedence. Note that this is not the way most programs handle options.
   * You should probably be using last() instead.
   *
   * @note
   * This method may be called on an unused/invalid option and will return a pointer to the
   * option itself.
   */
  Option* first()
  {
    Option* p = this;
    while (!p->isFirst())
      p = p->prev_;
    return p;
  }

  /**
  * const version of Option::first().
  */
  const Option* first() const
  {
    return const_cast<Option*>(this)->first();
  }

  /**
   * @brief Returns a pointer to the last element of the linked list.
   *
   * Use this when you want the last occurrence of an option on the command line to
   * take precedence. This is the most common way of handling conflicting options.
   *
   * @note
   * This method may be called on an unused/invalid option and will return a pointer to the
   * option itself.
   *
   * @par Tip:
   * If you have options with opposite meanings (e.g. @c --enable-foo and @c --disable-foo), you
   * can assign them the same Descriptor::index to get them into the same list. Distinguish them by
   * Descriptor::type and all you have to do is check <code> last()->type() </code> to get
   * the state listed last on the command line.
   */
  Option* last()
  {
    return first()->prevwrap();
  }

  /**
  * const version of Option::last().
  */
  const Option* last() const
  {
    return first()->prevwrap();
  }

  /**
   * @brief Returns a pointer to the previous element of the linked list or NULL if
   * called on first().
   *
   * If called on first() this method returns NULL. Otherwise it will return the
   * option with the same Descriptor::index that precedes this option on the command
   * line.
   */
  Option* prev()
  {
    return isFirst() ? 0 : prev_;
  }

  /**
   * @brief Returns a pointer to the previous element of the linked list with wrap-around from
   * first() to last().
   *
   * If called on first() this method returns last(). Otherwise it will return the
   * option with the same Descriptor::index that precedes this option on the command
   * line.
   */
  Option* prevwrap()
  {
    return untag(prev_);
  }

  /**
  * const version of Option::prevwrap().
  */
  const Option* prevwrap() const
  {
    return untag(prev_);
  }

  /**
   * @brief Returns a pointer to the next element of the linked list or NULL if called
   * on last().
   *
   * If called on last() this method returns NULL. Otherwise it will return the
   * option with the same Descriptor::index that follows this option on the command
   * line.
   */
  Option* next()
  {
    return isLast() ? 0 : next_;
  }

  /**
  * const version of Option::next().
  */
  const Option* next() const
  {
    return isLast() ? 0 : next_;
  }

  /**
   * @brief Returns a pointer to the next element of the linked list with wrap-around from
   * last() to first().
   *
   * If called on last() this method returns first(). Otherwise it will return the
   * option with the same Descriptor::index that follows this option on the command
   * line.
   */
  Option* nextwrap()
  {
    return untag(next_);
  }

  /**
   * @brief Makes @c new_last the new last() by chaining it into the list after last().
   *
   * It doesn't matter which element you call append() on. The new element will always
   * be appended to last().
   *
   * @attention
   * @c new_last must not yet be part of a list, or that list will become corrupted, because
   * this method does not unchain @c new_last from an existing list.
   */
  void append(Option* new_last)
  {
    Option* p = last();
    Option* f = first();
    p->next_ = new_last;
    new_last->prev_ = p;
    new_last->next_ = tag(f);
    f->prev_ = tag(new_last);
  }

  /**
   * @brief Casts from Option to const Option* but only if this Option is valid.
   *
   * If this Option is valid (i.e. @c desc!=NULL), returns this.
   * Otherwise returns NULL. This allows testing an Option directly
   * in an if-clause to see if it is used:
   * @code
   * if (options[CREATE])
   * {
   *   ...
   * }
   * @endcode
   * It also allows you to write loops like this:
   * @code for (Option* opt = options[FILE]; opt; opt = opt->next())
   *   fname = opt->arg; ... @endcode
   */
  operator const Option*() const
  {
    return desc ? this : 0;
  }

  /**
   * @brief Casts from Option to Option* but only if this Option is valid.
   *
   * If this Option is valid (i.e. @c desc!=NULL), returns this.
   * Otherwise returns NULL. This allows testing an Option directly
   * in an if-clause to see if it is used:
   * @code
   * if (options[CREATE])
   * {
   *   ...
   * }
   * @endcode
   * It also allows you to write loops like this:
   * @code for (Option* opt = options[FILE]; opt; opt = opt->next())
   *   fname = opt->arg; ... @endcode
   */
  operator Option*()
  {
    return desc ? this : 0;
  }

  /**
   * @brief Creates a new Option that is a one-element linked list and has NULL
   * @ref desc, @ref name, @ref arg and @ref namelen.
   */
  Option() :
      desc(0), name(0), arg(0), namelen(0)
  {
    prev_ = tag(this);
    next_ = tag(this);
  }

  /**
   * @brief Creates a new Option that is a one-element linked list and has the given
   * values for @ref desc, @ref name and @ref arg.
   *
   * If @c name_ points at a character other than '-' it will be assumed to refer to a
   * short option and @ref namelen will be set to 1. Otherwise the length will extend to
   * the first '=' character or the string's 0-terminator.
   */
  Option(const Descriptor* desc_, const char* name_, const char* arg_)
  {
    init(desc_, name_, arg_);
  }

  /**
   * @brief Makes @c *this a copy of @c orig except for the linked list pointers.
   *
   * After this operation @c *this will be a one-element linked list.
   */
  void operator=(const Option& orig)
  {
    init(orig.desc, orig.name, orig.arg);
  }

  /**
   * @brief Makes @c *this a copy of @c orig except for the linked list pointers.
   *
   * After this operation @c *this will be a one-element linked list.
   */
  Option(const Option& orig)
  {
    init(orig.desc, orig.name, orig.arg);
  }

private:
  /**
   * @internal
   * @brief Sets the fields of this Option to the given values (extracting @c name if necessary).
   *
   * If @c name_ points at a character other than '-' it will be assumed to refer to a
   * short option and @ref namelen will be set to 1. Otherwise the length will extend to
   * the first '=' character or the string's 0-terminator.
   */
  void init(const Descriptor* desc_, const char* name_, const char* arg_)
  {
    desc = desc_;
    name = name_;
    arg = arg_;
    prev_ = tag(this);
    next_ = tag(this);
    namelen = 0;
    if (name == 0)
      return;
    namelen = 1;
    if (name[0] != '-')
      return;
    while (name[namelen] != 0 && name[namelen] != '=')
      ++namelen;
  }

  static Option* tag(Option* ptr)
  {
    return reinterpret_cast<Option*>( reinterpret_cast<unsigned long long>( ptr ) | 1 );
  }

  static Option* untag(Option* ptr)
  {
    return reinterpret_cast<Option*>( reinterpret_cast<unsigned long long>( ptr ) & ~1ull);
  }

  static bool isTagged(Option* ptr)
  {
    return (reinterpret_cast<unsigned long long>( ptr ) & 1);
  }
};

/**
 * @brief Functions for checking the validity of option arguments.
 *
 * @copydetails CheckArg
 *
 * The following example code
 * can serve as starting place for writing your own more complex CheckArg functions:
 * @code
 * struct Arg: public option::Arg
 * {
 *   static void printError(const char* msg1, const option::Option& opt, const char* msg2)
 *   {
 *     fprintf(stderr, "ERROR: %s", msg1);
 *     fwrite(opt.name, opt.namelen, 1, stderr);
 *     fprintf(stderr, "%s", msg2);
 *   }
 *
 *   static option::ArgStatus Unknown(const option::Option& option, bool msg)
 *   {
 *     if (msg) printError("Unknown option '", option, "'\n");
 *     return option::ARG_ILLEGAL;
 *   }
 *
 *   static option::ArgStatus Required(const option::Option& option, bool msg)
 *   {
 *     if (option.arg != 0)
 *       return option::ARG_OK;
 *
 *     if (msg) printError("Option '", option, "' requires an argument\n");
 *     return option::ARG_ILLEGAL;
 *   }
 *
 *   static option::ArgStatus NonEmpty(const option::Option& option, bool msg)
 *   {
 *     if (option.arg != 0 && option.arg[0] != 0)
 *       return option::ARG_OK;
 *
 *     if (msg) printError("Option '", option, "' requires a non-empty argument\n");
 *     return option::ARG_ILLEGAL;
 *   }
 *
 *   static option::ArgStatus Numeric(const option::Option& option, bool msg)
 *   {
 *     char* endptr = 0;
 *     if (option.arg != 0 && strtol(option.arg, &endptr, 10)){};
 *     if (endptr != option.arg && *endptr == 0)
 *       return option::ARG_OK;
 *
 *     if (msg) printError("Option '", option, "' requires a numeric argument\n");
 *     return option::ARG_ILLEGAL;
 *   }
 * };
 * @endcode
 */
struct Arg
{
  //! @brief For options that don't take an argument: Returns ARG_NONE.
  static ArgStatus None(const Option&, bool)
  {
    return ARG_NONE;
  }

  //! @brief Returns ARG_OK if the argument is attached and ARG_IGNORE otherwise.
  static ArgStatus Optional(const Option& option, bool)
  {
    if (option.arg && option.name[option.namelen] != 0)
      return ARG_OK;
    else
      return ARG_IGNORE;
  }
};

/**
 * @brief Determines the minimum lengths of the buffer and options arrays used for Parser.
 *
 * Because Parser doesn't use dynamic memory its output arrays have to be pre-allocated.
 * If you don't want to use fixed size arrays (which may turn out too small, causing
 * command line arguments to be dropped), you can use Stats to determine the correct sizes.
 * Stats work cumulative. You can first pass in your default options and then the real
 * options and afterwards the counts will reflect the union.
 */
struct Stats
{
  /**
   * @brief Number of elements needed for a @c buffer[] array to be used for
   * @ref Parser::parse() "parsing" the same argument vectors that were fed
   * into this Stats object.
   *
   * @note
   * This number is always 1 greater than the actual number needed, to give
   * you a sentinel element.
   */
  unsigned buffer_max;

  /**
   * @brief Number of elements needed for an @c options[] array to be used for
   * @ref Parser::parse() "parsing" the same argument vectors that were fed
   * into this Stats object.
   *
   * @note
   * @li This number is always 1 greater than the actual number needed, to give
   * you a sentinel element.
   * @li This number depends only on the @c usage, not the argument vectors, because
   * the @c options array needs exactly one slot for each possible Descriptor::index.
   */
  unsigned options_max;

  /**
   * @brief Creates a Stats object with counts set to 1 (for the sentinel element).
   */
  Stats() :
      buffer_max(1), options_max(1) // 1 more than necessary as sentinel
  {
  }

  /**
   * @brief Creates a new Stats object and immediately updates it for the
   * given @c usage and argument vector. You may pass 0 for @c argc and/or @c argv,
   * if you just want to update @ref options_max.
   *
   * @note
   * The calls to Stats methods must match the later calls to Parser methods.
   * See Parser::parse() for the meaning of the arguments.
   */
  Stats(bool gnu, const Descriptor usage[], int argc, const char** argv, int min_abbr_len = 0, //
        bool single_minus_longopt = false) :
      buffer_max(1), options_max(1) // 1 more than necessary as sentinel
  {
    add(gnu, usage, argc, argv, min_abbr_len, single_minus_longopt);
  }

  //! @brief Stats(...) with non-const argv.
  Stats(bool gnu, const Descriptor usage[], int argc, char** argv, int min_abbr_len = 0, //
        bool single_minus_longopt = false) :
      buffer_max(1), options_max(1) // 1 more than necessary as sentinel
  {
    add(gnu, usage, argc, const_cast<const char**>( argv ), min_abbr_len, single_minus_longopt);
  }

  //! @brief POSIX Stats(...) (gnu==false).
  Stats(const Descriptor usage[], int argc, const char** argv, int min_abbr_len = 0, //
        bool single_minus_longopt = false) :
      buffer_max(1), options_max(1) // 1 more than necessary as sentinel
  {
    add(false, usage, argc, argv, min_abbr_len, single_minus_longopt);
  }

  //! @brief POSIX Stats(...) (gnu==false) with non-const argv.
  Stats(const Descriptor usage[], int argc, char** argv, int min_abbr_len = 0, //
        bool single_minus_longopt = false) :
      buffer_max(1), options_max(1) // 1 more than necessary as sentinel
  {
    add(false, usage, argc, const_cast<const char**>( argv ), min_abbr_len, single_minus_longopt);
  }

  /**
   * @brief Updates this Stats object for the
   * given @c usage and argument vector. You may pass 0 for @c argc and/or @c argv,
   * if you just want to update @ref options_max.
   *
   * @note
   * The calls to Stats methods must match the later calls to Parser methods.
   * See Parser::parse() for the meaning of the arguments.
   */
  void add(bool gnu, const Descriptor usage[], int argc, const char** argv, int min_abbr_len = 0, //
           bool single_minus_longopt = false);

  //! @brief add() with non-const argv.
  void add(bool gnu, const Descriptor usage[], int argc, char** argv, int min_abbr_len = 0, //
           bool single_minus_longopt = false)
  {
    add(gnu, usage, argc, const_cast<const char**>( argv ), min_abbr_len, single_minus_longopt);
  }

  //! @brief POSIX add() (gnu==false).
  void add(const Descriptor usage[], int argc, const char** argv, int min_abbr_len = 0, //
           bool single_minus_longopt = false)
  {
    add(false, usage, argc, argv, min_abbr_len, single_minus_longopt);
  }

  //! @brief POSIX add() (gnu==false) with non-const argv.
  void add(const Descriptor usage[], int argc, char** argv, int min_abbr_len = 0, //
           bool single_minus_longopt = false)
  {
    add(false, usage, argc, const_cast<const char**>( argv ), min_abbr_len, single_minus_longopt);
  }
private:
  class CountOptionsAction;
};

/**
 * @brief Checks argument vectors for validity and parses them into data
 * structures that are easier to work with.
 *
 * @par Example:
 * @code
 * int main(int argc, char* argv[])
 * {
 *   argc-=(argc>0); argv+=(argc>0); // skip program name argv[0] if present
 *   option::Stats  stats(usage, argc, argv);
 *   option::Option options[stats.options_max], buffer[stats.buffer_max];
 *   option::Parser parse(usage, argc, argv, options, buffer);
 *
 *   if (parse.error())
 *     return 1;
 *
 *   if (options[HELP])
 *   ...
 * @endcode
 */
class Parser
{
  int op_count; //!< @internal @brief see optionsCount()
  int nonop_count; //!< @internal @brief see nonOptionsCount()
  const char** nonop_args; //!< @internal @brief see nonOptions()
  bool err; //!< @internal @brief see error()
public:

  /**
   * @brief Creates a new Parser.
   */
  Parser() :
      op_count(0), nonop_count(0), nonop_args(0), err(false)
  {
  }

  /**
   * @brief Creates a new Parser and immediately parses the given argument vector.
   * @copydetails parse()
   */
  Parser(bool gnu, const Descriptor usage[], int argc, const char** argv, Option options[], Option buffer[],
         int min_abbr_len = 0, bool single_minus_longopt = false, int bufmax = -1) :
      op_count(0), nonop_count(0), nonop_args(0), err(false)
  {
    parse(gnu, usage, argc, argv, options, buffer, min_abbr_len, single_minus_longopt, bufmax);
  }

  //! @brief Parser(...) with non-const argv.
  Parser(bool gnu, const Descriptor usage[], int argc, char** argv, Option options[], Option buffer[],
         int min_abbr_len = 0, bool single_minus_longopt = false, int bufmax = -1) :
      op_count(0), nonop_count(0), nonop_args(0), err(false)
  {
    parse(gnu, usage, argc, const_cast<const char**>( argv ), options, buffer, min_abbr_len, single_minus_longopt, bufmax);
  }

  //! @brief POSIX Parser(...) (gnu==false).
  Parser(const Descriptor usage[], int argc, const char** argv, Option options[], Option buffer[], int min_abbr_len = 0,
         bool single_minus_longopt = false, int bufmax = -1) :
      op_count(0), nonop_count(0), nonop_args(0), err(false)
  {
    parse(false, usage, argc, argv, options, buffer, min_abbr_len, single_minus_longopt, bufmax);
  }

  //! @brief POSIX Parser(...) (gnu==false) with non-const argv.
  Parser(const Descriptor usage[], int argc, char** argv, Option options[], Option buffer[], int min_abbr_len = 0,
         bool single_minus_longopt = false, int bufmax = -1) :
      op_count(0), nonop_count(0), nonop_args(0), err(false)
  {
    parse(false, usage, argc, const_cast<const char**>( argv ), options, buffer, min_abbr_len, single_minus_longopt, bufmax);
  }

  /**
   * @brief Parses the given argument vector.
   *
   * @param gnu if true, parse() will not stop at the first non-option argument. Instead it will
   *            reorder arguments so that all non-options are at the end. This is the default behaviour
   *            of GNU getopt() but is not conforming to POSIX. @n
   *            Note, that once the argument vector has been reordered, the @c gnu flag will have
   *            no further effect on this argument vector. So it is enough to pass @c gnu==true when
   *            creating Stats.
   * @param usage Array of Descriptor objects that describe the options to support. The last entry
   *              of this array must have 0 in all fields.
   * @param argc The number of elements from @c argv that are to be parsed. If you pass -1, the number
   *             will be determined automatically. In that case the @c argv list must end with a NULL
   *             pointer.
   * @param argv The arguments to be parsed. If you pass -1 as @c argc the last pointer in the @c argv
   *             list must be NULL to mark the end.
   * @param options Each entry is the first element of a linked list of Options. Each new option
   *                that is parsed will be appended to the list specified by that Option's
   *                Descriptor::index. If an entry is not yet used (i.e. the Option is invalid),
   *                it will be replaced rather than appended to. @n
   *                The minimum length of this array is the greatest Descriptor::index value that
   *                occurs in @c usage @e PLUS ONE.
   * @param buffer Each argument that is successfully parsed (including unknown arguments, if they
   *        have a Descriptor whose CheckArg does not return @ref ARG_ILLEGAL) will be stored in this
   *        array. parse() scans the array for the first invalid entry and begins writing at that
   *        index. You can pass @c bufmax to limit the number of options stored.
   * @param min_abbr_len Passing a value <code> min_abbr_len > 0 </code> enables abbreviated long
   *               options. The parser will match a prefix of a long option as if it was
   *               the full long option (e.g. @c --foob=10 will be interpreted as if it was
   *               @c --foobar=10 ), as long as the prefix has at least @c min_abbr_len characters
   *               (not counting the @c -- ) and is unambiguous.
   *               @n Be careful if combining @c min_abbr_len=1 with @c single_minus_longopt=true
   *               because the ambiguity check does not consider short options and abbreviated
   *               single minus long options will take precedence over short options.
   * @param single_minus_longopt Passing @c true for this option allows long options to begin with
   *               a single minus. The double minus form will still be recognized. Note that
   *               single minus long options take precedence over short options and short option
   *               groups. E.g. @c -file would be interpreted as @c --file and not as
   *               <code> -f -i -l -e </code> (assuming a long option named @c "file" exists).
   * @param bufmax The greatest index in the @c buffer[] array that parse() will write to is
   *               @c bufmax-1. If there are more options, they will be processed (in particular
   *               their CheckArg will be called) but not stored. @n
   *               If you used Stats::buffer_max to dimension this array, you can pass
   *               -1 (or not pass @c bufmax at all) which tells parse() that the buffer is
   *               "large enough".
   * @attention
   * Remember that @c options and @c buffer store Option @e objects, not pointers. Therefore it
   * is not possible for the same object to be in both arrays. For those options that are found in
   * both @c buffer[] and @c options[] the respective objects are independent copies. And only the
   * objects in @c options[] are properly linked via Option::next() and Option::prev().
   * You can iterate over @c buffer[] to
   * process all options in the order they appear in the argument vector, but if you want access to
   * the other Options with the same Descriptor::index, then you @e must access the linked list via
   * @c options[]. You can get the linked list in options from a buffer object via something like
   * @c options[buffer[i].index()].
   */
  void parse(bool gnu, const Descriptor usage[], int argc, const char** argv, Option options[], Option buffer[],
             int min_abbr_len = 0, bool single_minus_longopt = false, int bufmax = -1);

  //! @brief parse() with non-const argv.
  void parse(bool gnu, const Descriptor usage[], int argc, char** argv, Option options[], Option buffer[],
             int min_abbr_len = 0, bool single_minus_longopt = false, int bufmax = -1)
  {
    parse(gnu, usage, argc, const_cast<const char**>( argv ), options, buffer, min_abbr_len, single_minus_longopt, bufmax);
  }

  //! @brief POSIX parse() (gnu==false).
  void parse(const Descriptor usage[], int argc, const char** argv, Option options[], Option buffer[],
             int min_abbr_len = 0, bool single_minus_longopt = false, int bufmax = -1)
  {
    parse(false, usage, argc, argv, options, buffer, min_abbr_len, single_minus_longopt, bufmax);
  }

  //! @brief POSIX parse() (gnu==false) with non-const argv.
  void parse(const Descriptor usage[], int argc, char** argv, Option options[], Option buffer[], int min_abbr_len = 0,
             bool single_minus_longopt = false, int bufmax = -1)
  {
    parse(false, usage, argc, const_cast<const char**>( argv ), options, buffer, min_abbr_len, single_minus_longopt, bufmax);
  }

  /**
   * @brief Returns the number of valid Option objects in @c buffer[].
   *
   * @note
   * @li The returned value always reflects the number of Options in the buffer[] array used for
   * the most recent call to parse().
   * @li The count (and the buffer[]) includes unknown options if they are collected
   * (see Descriptor::longopt).
   */
  int optionsCount()
  {
    return op_count;
  }

  /**
   * @brief Returns the number of non-option arguments that remained at the end of the
   * most recent parse() that actually encountered non-option arguments.
   *
   * @note
   * A parse() that does not encounter non-option arguments will leave this value
   * as well as nonOptions() undisturbed. This means you can feed the Parser a
   * default argument vector that contains non-option arguments (e.g. a default filename).
   * Then you feed it the actual arguments from the user. If the user has supplied at
   * least one non-option argument, all of the non-option arguments from the default
   * disappear and are replaced by the user's non-option arguments. However, if the
   * user does not supply any non-option arguments the defaults will still be in
   * effect.
   */
  int nonOptionsCount()
  {
    return nonop_count;
  }

  /**
   * @brief Returns a pointer to an array of non-option arguments (only valid
   * if <code>nonOptionsCount() >0 </code>).
   *
   * @note
   * @li parse() does not copy arguments, so this pointer points into the actual argument
   * vector as passed to parse().
   * @li As explained at nonOptionsCount() this pointer is only changed by parse() calls
   * that actually encounter non-option arguments. A parse() call that encounters only
   * options, will not change nonOptions().
   */
  const char** nonOptions()
  {
    return nonop_args;
  }

  /**
   * @brief Returns <b><code>nonOptions()[i]</code></b> (@e without checking if i is in range!).
   */
  const char* nonOption(int i)
  {
    return nonOptions()[i];
  }

  /**
   * @brief Returns @c true if an unrecoverable error occurred while parsing options.
   *
   * An illegal argument to an option (i.e. CheckArg returns @ref ARG_ILLEGAL) is an
   * unrecoverable error that aborts the parse. Unknown options are only an error if
   * their CheckArg function returns @ref ARG_ILLEGAL. Otherwise they are collected.
   * In that case if you want to exit the program if either an illegal argument
   * or an unknown option has been passed, use code like this
   *
   * @code
   * if (parser.error() || options[UNKNOWN])
   *   exit(1);
   * @endcode
   *
   */
  bool error()
  {
    return err;
  }

private:
  friend struct Stats;
  class StoreOptionAction;
  struct Action;

  /**
   * @internal
   * @brief This is the core function that does all the parsing.
   * @retval false iff an unrecoverable error occurred.
   */
  static bool workhorse(bool gnu, const Descriptor usage[], int numargs, const char** args, Action& action,
                        bool single_minus_longopt, bool print_errors, int min_abbr_len);

  /**
   * @internal
   * @brief Returns true iff @c st1 is a prefix of @c st2 and
   * in case @c st2 is longer than @c st1, then
   * the first additional character is '='.
   *
   * @par Examples:
   * @code
   * streq("foo", "foo=bar") == true
   * streq("foo", "foobar")  == false
   * streq("foo", "foo")     == true
   * streq("foo=bar", "foo") == false
   * @endcode
   */
  static bool streq(const char* st1, const char* st2)
  {
    while (*st1 != 0)
      if (*st1++ != *st2++)
        return false;
    return (*st2 == 0 || *st2 == '=');
  }

  /**
   * @internal
   * @brief Like streq() but handles abbreviations.
   *
   * Returns true iff @c st1 and @c st2 have a common
   * prefix with the following properties:
   * @li (if min > 0) its length is at least @c min characters or the same length as @c st1 (whichever is smaller).
   * @li (if min <= 0) its length is the same as that of @c st1
   * @li within @c st2 the character following the common prefix is either '=' or end-of-string.
   *
   * Examples:
   * @code
   * streqabbr("foo", "foo=bar",<anything>) == true
   * streqabbr("foo", "fo=bar" , 2) == true
   * streqabbr("foo", "fo"     , 2) == true
   * streqabbr("foo", "fo"     , 0) == false
   * streqabbr("foo", "f=bar"  , 2) == false
   * streqabbr("foo", "f"      , 2) == false
   * streqabbr("fo" , "foo=bar",<anything>)  == false
   * streqabbr("foo", "foobar" ,<anything>)  == false
   * streqabbr("foo", "fobar"  ,<anything>)  == false
   * streqabbr("foo", "foo"    ,<anything>)  == true
   * @endcode
   */
  static bool streqabbr(const char* st1, const char* st2, long long min)
  {
    const char* st1start = st1;
    while (*st1 != 0 && (*st1 == *st2))
    {
      ++st1;
      ++st2;
    }

    return (*st1 == 0 || (min > 0 && (st1 - st1start) >= min)) && (*st2 == 0 || *st2 == '=');
  }

  /**
   * @internal
   * @brief Returns true iff character @c ch is contained in the string @c st.
   *
   * Returns @c true for @c ch==0 .
   */
  static bool instr(char ch, const char* st)
  {
    while (*st != 0 && *st != ch)
      ++st;
    return *st == ch;
  }

  /**
   * @internal
   * @brief Rotates <code>args[-count],...,args[-1],args[0]</code> to become
   *        <code>args[0],args[-count],...,args[-1]</code>.
   */
  static void shift(const char** args, int count)
  {
    for (int i = 0; i > -count; --i)
    {
      const char* temp = args[i];
      args[i] = args[i - 1];
      args[i - 1] = temp;
    }
  }
};

/**
 * @internal
 * @brief Interface for actions Parser::workhorse() should perform for each Option it
 * parses.
 */
struct Parser::Action
{

  virtual ~Action() {}

  /**
   * @brief Called by Parser::workhorse() for each Option that has been successfully
   * parsed (including unknown
   * options if they have a Descriptor whose Descriptor::check_arg does not return
   * @ref ARG_ILLEGAL.
   *
   * Returns @c false iff a fatal error has occured and the parse should be aborted.
   */
  virtual bool perform(Option&)
  {
    return true;
  }

  /**
   * @brief Called by Parser::workhorse() after finishing the parse.
   * @param numargs the number of non-option arguments remaining
   * @param args pointer to the first remaining non-option argument (if numargs > 0).
   *
   * @return
   * @c false iff a fatal error has occurred.
   */
  virtual bool finished(int numargs, const char** args)
  {
    (void) numargs;
    (void) args;
    return true;
  }
};

/**
 * @internal
 * @brief An Action to pass to Parser::workhorse() that will increment a counter for
 * each parsed Option.
 */
class Stats::CountOptionsAction: public Parser::Action
{
  unsigned* buffer_max;
public:
  /**
   * Creates a new CountOptionsAction that will increase @c *buffer_max_ for each
   * parsed Option.
   */
  CountOptionsAction(unsigned* buffer_max_) :
      buffer_max(buffer_max_)
  {
  }

  bool perform(Option&)
  {
    if (*buffer_max == 0x7fffffff)
      return false; // overflow protection: don't accept number of options that doesn't fit signed int
    ++*buffer_max;
    return true;
  }
};

/**
 * @internal
 * @brief An Action to pass to Parser::workhorse() that will store each parsed Option in
 * appropriate arrays (see Parser::parse()).
 */
class Parser::StoreOptionAction: public Parser::Action
{
  Parser& parser;
  Option* options;
  Option* buffer;
  int bufmax; //! Number of slots in @c buffer. @c -1 means "large enough".
public:
  /**
   * @brief Creates a new StoreOption action.
   * @param parser_ the parser whose op_count should be updated.
   * @param options_ each Option @c o is chained into the linked list @c options_[o.desc->index]
   * @param buffer_ each Option is appended to this array as long as there's a free slot.
   * @param bufmax_ number of slots in @c buffer_. @c -1 means "large enough".
   */
  StoreOptionAction(Parser& parser_, Option options_[], Option buffer_[], int bufmax_) :
      parser(parser_), options(options_), buffer(buffer_), bufmax(bufmax_)
  {
    // find first empty slot in buffer (if any)
    int bufidx = 0;
    while ((bufmax < 0 || bufidx < bufmax) && buffer[bufidx])
      ++bufidx;

    // set parser's optionCount
    parser.op_count = bufidx;
  }

  bool perform(Option& option)
  {
    if (bufmax < 0 || parser.op_count < bufmax)
    {
      if (parser.op_count == 0x7fffffff)
        return false; // overflow protection: don't accept number of options that doesn't fit signed int

      buffer[parser.op_count] = option;
      int idx = buffer[parser.op_count].desc->index;
      if (options[idx])
        options[idx].append(buffer[parser.op_count]);
      else
        options[idx] = buffer[parser.op_count];
      ++parser.op_count;
    }
    return true; // NOTE: an option that is discarded because of a full buffer is not fatal
  }

  bool finished(int numargs, const char** args)
  {
    // only overwrite non-option argument list if there's at least 1
    // new non-option argument. Otherwise we keep the old list. This
    // makes it easy to use default non-option arguments.
    if (numargs > 0)
    {
      parser.nonop_count = numargs;
      parser.nonop_args = args;
    }

    return true;
  }
};

inline void Parser::parse(bool gnu, const Descriptor usage[], int argc, const char** argv, Option options[],
                          Option buffer[], int min_abbr_len, bool single_minus_longopt, int bufmax)
{
  StoreOptionAction action(*this, options, buffer, bufmax);
  err = !workhorse(gnu, usage, argc, argv, action, single_minus_longopt, true, min_abbr_len);
}

inline void Stats::add(bool gnu, const Descriptor usage[], int argc, const char** argv, int min_abbr_len,
                       bool single_minus_longopt)
{
  // determine size of options array. This is the greatest index used in the usage + 1
  int i = 0;
  while (usage[i].shortopt != 0)
  {
    if (usage[i].index + 1 >= options_max)
      options_max = (usage[i].index + 1) + 1; // 1 more than necessary as sentinel

    ++i;
  }

  CountOptionsAction action(&buffer_max);
  Parser::workhorse(gnu, usage, argc, argv, action, single_minus_longopt, false, min_abbr_len);
}

inline bool Parser::workhorse(bool gnu, const Descriptor usage[], int numargs, const char** args, Action& action,
                              bool single_minus_longopt, bool print_errors, int min_abbr_len)
{
  // protect against NULL pointer
  if (args == 0)
    numargs = 0;

  int nonops = 0;

  while (numargs != 0 && *args != 0)
  {
    const char* param = *args; // param can be --long-option, -srto or non-option argument

    // in POSIX mode the first non-option argument terminates the option list
    // a lone minus character is a non-option argument
    if (param[0] != '-' || param[1] == 0)
    {
      if (gnu)
      {
        ++nonops;
        ++args;
        if (numargs > 0)
          --numargs;
        continue;
      }
      else
        break;
    }

    // -- terminates the option list. The -- itself is skipped.
    if (param[1] == '-' && param[2] == 0)
    {
      shift(args, nonops);
      ++args;
      if (numargs > 0)
        --numargs;
      break;
    }

    bool handle_short_options;
    const char* longopt_name;
    if (param[1] == '-') // if --long-option
    {
      handle_short_options = false;
      longopt_name = param + 2;
    }
    else
    {
      handle_short_options = true;
      longopt_name = param + 1; //for testing a potential -long-option
    }

    bool try_single_minus_longopt = single_minus_longopt;
    bool have_more_args = (numargs > 1 || numargs < 0); // is referencing argv[1] valid?

    do // loop over short options in group, for long options the body is executed only once
    {
      int idx = 0;

      const char* optarg = 0;

      /******************** long option **********************/
      if (handle_short_options == false || try_single_minus_longopt)
      {
        idx = 0;
        while (usage[idx].longopt != 0 && !streq(usage[idx].longopt, longopt_name))
          ++idx;

        if (usage[idx].longopt == 0 && min_abbr_len > 0) // if we should try to match abbreviated long options
        {
          int i1 = 0;
          while (usage[i1].longopt != 0 && !streqabbr(usage[i1].longopt, longopt_name, min_abbr_len))
            ++i1;
          if (usage[i1].longopt != 0)
          { // now test if the match is unambiguous by checking for another match
            int i2 = i1 + 1;
            while (usage[i2].longopt != 0 && !streqabbr(usage[i2].longopt, longopt_name, min_abbr_len))
              ++i2;

            if (usage[i2].longopt == 0) // if there was no second match it's unambiguous, so accept i1 as idx
              idx = i1;
          }
        }

        // if we found something, disable handle_short_options (only relevant if single_minus_longopt)
        if (usage[idx].longopt != 0)
          handle_short_options = false;

        try_single_minus_longopt = false; // prevent looking for longopt in the middle of shortopt group

        optarg = longopt_name;
        while (*optarg != 0 && *optarg != '=')
          ++optarg;
        if (*optarg == '=') // attached argument
          ++optarg;
        else
          // possibly detached argument
          optarg = (have_more_args ? args[1] : 0);
      }

      /************************ short option ***********************************/
      if (handle_short_options)
      {
        if (*++param == 0) // point at the 1st/next option character
          break; // end of short option group

        idx = 0;
        while (usage[idx].shortopt != 0 && !instr(*param, usage[idx].shortopt))
          ++idx;

        if (param[1] == 0) // if the potential argument is separate
          optarg = (have_more_args ? args[1] : 0);
        else
          // if the potential argument is attached
          optarg = param + 1;
      }

      const Descriptor* descriptor = &usage[idx];

      if (descriptor->shortopt == 0) /**************  unknown option ********************/
      {
        // look for dummy entry (shortopt == "" and longopt == "") to use as Descriptor for unknown options
        idx = 0;
        while (usage[idx].shortopt != 0 && (usage[idx].shortopt[0] != 0 || usage[idx].longopt[0] != 0))
          ++idx;
        descriptor = (usage[idx].shortopt == 0 ? 0 : &usage[idx]);
      }

      if (descriptor != 0)
      {
        Option option(descriptor, param, optarg);
        switch (descriptor->check_arg(option, print_errors))
        {
          case ARG_ILLEGAL:
            return false; // fatal
          case ARG_OK:
            // skip one element of the argument vector, if it's a separated argument
            if (optarg != 0 && have_more_args && optarg == args[1])
            {
              shift(args, nonops);
              if (numargs > 0)
                --numargs;
              ++args;
            }

            // No further short options are possible after an argument
            handle_short_options = false;

            break;
          case ARG_IGNORE:
          case ARG_NONE:
            option.arg = 0;
            break;
        }

        if (!action.perform(option))
          return false;
      }

    } while (handle_short_options);

    shift(args, nonops);
    ++args;
    if (numargs > 0)
      --numargs;

  } // while

  if (numargs > 0 && *args == 0) // It's a bug in the caller if numargs is greater than the actual number
    numargs = 0; // of arguments, but as a service to the user we fix this if we spot it.

  if (numargs < 0) // if we don't know the number of remaining non-option arguments
  { // we need to count them
    numargs = 0;
    while (args[numargs] != 0)
      ++numargs;
  }

  return action.finished(numargs + nonops, args - nonops);
}

/**
 * @internal
 * @brief The implementation of option::printUsage().
 */
struct PrintUsageImplementation
{
  /**
   * @internal
   * @brief Interface for Functors that write (part of) a string somewhere.
   */
  struct IStringWriter
  {
    virtual ~IStringWriter() {}
    /**
     * @brief Writes the given number of chars beginning at the given pointer somewhere.
     */
    virtual void operator()(const char*, int)
    {
    }
  };

  /**
   * @internal
   * @brief Encapsulates a function with signature <code>func(string, size)</code> where
   * string can be initialized with a const char* and size with an int.
   */
  template<typename Function>
  struct FunctionWriter: public IStringWriter
  {
    Function* write;

    virtual void operator()(const char* str, int size)
    {
      (*write)(str, size);
    }

    FunctionWriter(Function* w) :
        write(w)
    {
    }
  };

  /**
   * @internal
   * @brief Encapsulates a reference to an object with a <code>write(string, size)</code>
   * method like that of @c std::ostream.
   */
  template<typename OStream>
  struct OStreamWriter: public IStringWriter
  {
    OStream& ostream;

    virtual void operator()(const char* str, int size)
    {
      ostream.write(str, size);
    }

    OStreamWriter(OStream& o) :
        ostream(o)
    {
    }
  };

  /**
   * @internal
   * @brief Like OStreamWriter but encapsulates a @c const reference, which is
   * typically a temporary object of a user class.
   */
  template<typename Temporary>
  struct TemporaryWriter: public IStringWriter
  {
    const Temporary& userstream;

    virtual void operator()(const char* str, int size)
    {
      userstream.write(str, size);
    }

    TemporaryWriter(const Temporary& u) :
        userstream(u)
    {
    }
  };

  /**
   * @internal
   * @brief Encapsulates a function with the signature <code>func(fd, string, size)</code> (the
   * signature of the @c write() system call)
   * where fd can be initialized from an int, string from a const char* and size from an int.
   */
  template<typename Syscall>
  struct SyscallWriter: public IStringWriter
  {
    Syscall* write;
    int fd;

    virtual void operator()(const char* str, int size)
    {
      (*write)(fd, str, size);
    }

    SyscallWriter(Syscall* w, int f) :
        write(w), fd(f)
    {
    }
  };

  /**
   * @internal
   * @brief Encapsulates a function with the same signature as @c std::fwrite().
   */
  template<typename Function, typename Stream>
  struct StreamWriter: public IStringWriter
  {
    Function* fwrite;
    Stream* stream;

    virtual void operator()(const char* str, int size)
    {
      (*fwrite)(str, size, 1, stream);
    }

    StreamWriter(Function* w, Stream* s) :
        fwrite(w), stream(s)
    {
    }
  };

  /**
   * @internal
   * @brief Sets <code> i1 = max(i1, i2) </code>
   */
  static void upmax(int& i1, int i2)
  {
    i1 = (i1 >= i2 ? i1 : i2);
  }

  /**
   * @internal
   * @brief Moves the "cursor" to column @c want_x assuming it is currently at column @c x
   * and sets @c x=want_x .
   * If <code> x > want_x </code>, a line break is output before indenting.
   *
   * @param write Spaces and possibly a line break are written via this functor to get
   *        the desired indentation @c want_x .
   * @param[in,out] x the current indentation. Set to @c want_x by this method.
   * @param want_x the desired indentation.
   */
  static void indent(IStringWriter& write, int& x, int want_x)
  {
    int indent = want_x - x;
    if (indent < 0)
    {
      write("\n", 1);
      indent = want_x;
    }

    if (indent > 0)
    {
      char space = ' ';
      for (int i = 0; i < indent; ++i)
        write(&space, 1);
      x = want_x;
    }
  }

  /**
   * @brief Returns true if ch is the unicode code point of a wide character.
   *
   * @note
   * The following character ranges are treated as wide
   * @code
   * 1100..115F
   * 2329..232A  (just 2 characters!)
   * 2E80..A4C6  except for 303F
   * A960..A97C
   * AC00..D7FB
   * F900..FAFF
   * FE10..FE6B
   * FF01..FF60
   * FFE0..FFE6
   * 1B000......
   * @endcode
   */
  static bool isWideChar(unsigned ch)
  {
    if (ch == 0x303F)
      return false;

    return ((0x1100 <= ch && ch <= 0x115F) || (0x2329 <= ch && ch <= 0x232A) || (0x2E80 <= ch && ch <= 0xA4C6)
        || (0xA960 <= ch && ch <= 0xA97C) || (0xAC00 <= ch && ch <= 0xD7FB) || (0xF900 <= ch && ch <= 0xFAFF)
        || (0xFE10 <= ch && ch <= 0xFE6B) || (0xFF01 <= ch && ch <= 0xFF60) || (0xFFE0 <= ch && ch <= 0xFFE6)
        || (0x1B000 <= ch));
  }

  /**
   * @internal
   * @brief Splits a @c Descriptor[] array into tables, rows, lines and columns and
   * iterates over these components.
   *
   * The top-level organizational unit is the @e table.
   * A table begins at a Descriptor with @c help!=NULL and extends up to
   * a Descriptor with @c help==NULL.
   *
   * A table consists of @e rows. Due to line-wrapping and explicit breaks
   * a row may take multiple lines on screen. Rows within the table are separated
   * by \\n. They never cross Descriptor boundaries. This means a row ends either
   * at \\n or the 0 at the end of the help string.
   *
   * A row consists of columns/cells. Columns/cells within a row are separated by \\t.
   * Line breaks within a cell are marked by \\v.
   *
   * Rows in the same table need not have the same number of columns/cells. The
   * extreme case are interjections, which are rows that contain neither \\t nor \\v.
   * These are NOT treated specially by LinePartIterator, but they are treated
   * specially by printUsage().
   *
   * LinePartIterator iterates through the usage at 3 levels: table, row and part.
   * Tables and rows are as described above. A @e part is a line within a cell.
   * LinePartIterator iterates through 1st parts of all cells, then through the 2nd
   * parts of all cells (if any),... @n
   * Example: The row <code> "1 \v 3 \t 2 \v 4" </code> has 2 cells/columns and 4 parts.
   * The parts will be returned in the order 1, 2, 3, 4.
   *
   * It is possible that some cells have fewer parts than others. In this case
   * LinePartIterator will "fill up" these cells with 0-length parts. IOW, LinePartIterator
   * always returns the same number of parts for each column. Note that this is different
   * from the way rows and columns are handled. LinePartIterator does @e not guarantee that
   * the same number of columns will be returned for each row.
   *
   */
  class LinePartIterator
  {
    const Descriptor* tablestart; //!< The 1st descriptor of the current table.
    const Descriptor* rowdesc; //!< The Descriptor that contains the current row.
    const char* rowstart; //!< Ptr to 1st character of current row within rowdesc->help.
    const char* ptr; //!< Ptr to current part within the current row.
    int col; //!< Index of current column.
    int len; //!< Length of the current part (that ptr points at) in BYTES
    int screenlen; //!< Length of the current part in screen columns (taking narrow/wide chars into account).
    int max_line_in_block; //!< Greatest index of a line within the block. This is the number of \\v within the cell with the most \\vs.
    int line_in_block; //!< Line index within the current cell of the current part.
    int target_line_in_block; //!< Line index of the parts we should return to the user on this iteration.
    bool hit_target_line; //!< Flag whether we encountered a part with line index target_line_in_block in the current cell.

    /**
     * @brief Determines the byte and character lengths of the part at @ref ptr and
     * stores them in @ref len and @ref screenlen respectively.
     */
    void update_length()
    {
      screenlen = 0;
      for (len = 0; ptr[len] != 0 && ptr[len] != '\v' && ptr[len] != '\t' && ptr[len] != '\n'; ++len)
      {
        ++screenlen;
        unsigned ch = static_cast<unsigned char>( ptr[len] );
        if (ch > 0xC1) // everything <= 0xC1 (yes, even 0xC1 itself) is not a valid UTF-8 start byte
        {
          // int __builtin_clz (unsigned int x)
          // Returns the number of leading 0-bits in x, starting at the most significant bit
          unsigned mask = static_cast<unsigned>( -1 ) >> __builtin_clz(ch ^ 0xff);
          ch = ch & mask; // mask out length bits, we don't verify their correctness
          while ((static_cast<unsigned char>( ptr[len + 1] ) ^ 0x80) <= 0x3F) // while next byte is continuation byte
          {
            ch = (ch << 6) ^ static_cast<unsigned char>( ptr[len + 1] ) ^ 0x80; // add continuation to char code
            ++len;
          }
          // ch is the decoded unicode code point
          if (ch >= 0x1100 && isWideChar(ch)) // the test for 0x1100 is here to avoid the function call in the Latin case
            ++screenlen;
        }
      }
    }

  public:
    //! @brief Creates an iterator for @c usage.
    LinePartIterator(const Descriptor usage[]) :
        tablestart(usage), rowdesc(0), rowstart(0), ptr(0), col(-1), len(0), max_line_in_block(0), line_in_block(0),
        target_line_in_block(0), hit_target_line(true)
    {
    }

    /**
     * @brief Moves iteration to the next table (if any). Has to be called once on a new
     * LinePartIterator to move to the 1st table.
     * @retval false if moving to next table failed because no further table exists.
     */
    bool nextTable()
    {
      // If this is NOT the first time nextTable() is called after the constructor,
      // then skip to the next table break (i.e. a Descriptor with help == 0)
      if (rowdesc != 0)
      {
        while (tablestart->help != 0 && tablestart->shortopt != 0)
          ++tablestart;
      }

      // Find the next table after the break (if any)
      while (tablestart->help == 0 && tablestart->shortopt != 0)
        ++tablestart;

      restartTable();
      return rowstart != 0;
    }

    /**
     * @brief Reset iteration to the beginning of the current table.
     */
    void restartTable()
    {
      rowdesc = tablestart;
      rowstart = tablestart->help;
      ptr = 0;
    }

    /**
     * @brief Moves iteration to the next row (if any). Has to be called once after each call to
     * @ref nextTable() to move to the 1st row of the table.
     * @retval false if moving to next row failed because no further row exists.
     */
    bool nextRow()
    {
      if (ptr == 0)
      {
        restartRow();
        return rowstart != 0;
      }

      while (*ptr != 0 && *ptr != '\n')
        ++ptr;

      if (*ptr == 0)
      {
        if ((rowdesc + 1)->help == 0) // table break
          return false;

        ++rowdesc;
        rowstart = rowdesc->help;
      }
      else // if (*ptr == '\n')
      {
        rowstart = ptr + 1;
      }

      restartRow();
      return true;
    }

    /**
     * @brief Reset iteration to the beginning of the current row.
     */
    void restartRow()
    {
      ptr = rowstart;
      col = -1;
      len = 0;
      screenlen = 0;
      max_line_in_block = 0;
      line_in_block = 0;
      target_line_in_block = 0;
      hit_target_line = true;
    }

    /**
     * @brief Moves iteration to the next part (if any). Has to be called once after each call to
     * @ref nextRow() to move to the 1st part of the row.
     * @retval false if moving to next part failed because no further part exists.
     *
     * See @ref LinePartIterator for details about the iteration.
     */
    bool next()
    {
      if (ptr == 0)
        return false;

      if (col == -1)
      {
        col = 0;
        update_length();
        return true;
      }

      ptr += len;
      while (true)
      {
        switch (*ptr)
        {
          case '\v':
            upmax(max_line_in_block, ++line_in_block);
            ++ptr;
            break;
          case '\t':
            if (!hit_target_line) // if previous column did not have the targetline
            { // then "insert" a 0-length part
              update_length();
              hit_target_line = true;
              return true;
            }

            hit_target_line = false;
            line_in_block = 0;
            ++col;
            ++ptr;
            break;
          case 0:
          case '\n':
            if (!hit_target_line) // if previous column did not have the targetline
            { // then "insert" a 0-length part
              update_length();
              hit_target_line = true;
              return true;
            }

            if (++target_line_in_block > max_line_in_block)
            {
              update_length();
              return false;
            }

            hit_target_line = false;
            line_in_block = 0;
            col = 0;
            ptr = rowstart;
            continue;
          default:
            ++ptr;
            continue;
        } // switch

        if (line_in_block == target_line_in_block)
        {
          update_length();
          hit_target_line = true;
          return true;
        }
      } // while
    }

    /**
     * @brief Returns the index (counting from 0) of the column in which
     * the part pointed to by @ref data() is located.
     */
    int column()
    {
      return col;
    }

    /**
     * @brief Returns the index (counting from 0) of the line within the current column
     * this part belongs to.
     */
    int line()
    {
      return target_line_in_block; // NOT line_in_block !!! It would be wrong if !hit_target_line
    }

    /**
     * @brief Returns the length of the part pointed to by @ref data() in raw chars (not UTF-8 characters).
     */
    int length()
    {
      return len;
    }

    /**
     * @brief Returns the width in screen columns of the part pointed to by @ref data().
     * Takes multi-byte UTF-8 sequences and wide characters into account.
     */
    int screenLength()
    {
      return screenlen;
    }

    /**
     * @brief Returns the current part of the iteration.
     */
    const char* data()
    {
      return ptr;
    }
  };

  /**
   * @internal
   * @brief Takes input and line wraps it, writing out one line at a time so that
   * it can be interleaved with output from other columns.
   *
   * The LineWrapper is used to handle the last column of each table as well as interjections.
   * The LineWrapper is called once for each line of output. If the data given to it fits
   * into the designated width of the last column it is simply written out. If there
   * is too much data, an appropriate split point is located and only the data up to this
   * split point is written out. The rest of the data is queued for the next line.
   * That way the last column can be line wrapped and interleaved with data from
   * other columns. The following example makes this clearer:
   * @code
   * Column 1,1    Column 2,1     This is a long text
   * Column 1,2    Column 2,2     that does not fit into
   *                              a single line.
   * @endcode
   *
   * The difficulty in producing this output is that the whole string
   * "This is a long text that does not fit into a single line" is the
   * 1st and only part of column 3. In order to produce the above
   * output the string must be output piecemeal, interleaved with
   * the data from the other columns.
   */
  class LineWrapper
  {
    static const int bufmask = 15; //!< Must be a power of 2 minus 1.
    /**
     * @brief Ring buffer for length component of pair (data, length).
     */
    int lenbuf[bufmask + 1];
    /**
     * @brief Ring buffer for data component of pair (data, length).
     */
    const char* datbuf[bufmask + 1];
    /**
     * @brief The indentation of the column to which the LineBuffer outputs. LineBuffer
     * assumes that the indentation has already been written when @ref process()
     * is called, so this value is only used when a buffer flush requires writing
     * additional lines of output.
     */
    int x;
    /**
     * @brief The width of the column to line wrap.
     */
    int width;
    int head; //!< @brief index for next write
    int tail; //!< @brief index for next read - 1 (i.e. increment tail BEFORE read)

    /**
     * @brief Multiple methods of LineWrapper may decide to flush part of the buffer to
     * free up space. The contract of process() says that only 1 line is output. So
     * this variable is used to track whether something has output a line. It is
     * reset at the beginning of process() and checked at the end to decide if
     * output has already occurred or is still needed.
     */
    bool wrote_something;

    bool buf_empty()
    {
      return ((tail + 1) & bufmask) == head;
    }

    bool buf_full()
    {
      return tail == head;
    }

    void buf_store(const char* data, int len)
    {
      lenbuf[head] = len;
      datbuf[head] = data;
      head = (head + 1) & bufmask;
    }

    //! @brief Call BEFORE reading ...buf[tail].
    void buf_next()
    {
      tail = (tail + 1) & bufmask;
    }

    /**
     * @brief Writes (data,len) into the ring buffer. If the buffer is full, a single line
     * is flushed out of the buffer into @c write.
     */
    void output(IStringWriter& write, const char* data, int len)
    {
      if (buf_full())
        write_one_line(write);

      buf_store(data, len);
    }

    /**
     * @brief Writes a single line of output from the buffer to @c write.
     */
    void write_one_line(IStringWriter& write)
    {
      if (wrote_something) // if we already wrote something, we need to start a new line
      {
        write("\n", 1);
        int _ = 0;
        indent(write, _, x);
      }

      if (!buf_empty())
      {
        buf_next();
        write(datbuf[tail], lenbuf[tail]);
      }

      wrote_something = true;
    }
  public:

    /**
     * @brief Writes out all remaining data from the LineWrapper using @c write.
     * Unlike @ref process() this method indents all lines including the first and
     * will output a \\n at the end (but only if something has been written).
     */
    void flush(IStringWriter& write)
    {
      if (buf_empty())
        return;
      int _ = 0;
      indent(write, _, x);
      wrote_something = false;
      while (!buf_empty())
        write_one_line(write);
      write("\n", 1);
    }

    /**
     * @brief Process, wrap and output the next piece of data.
     *
     * process() will output at least one line of output. This is not necessarily
     * the @c data passed in. It may be data queued from a prior call to process().
     * If the internal buffer is full, more than 1 line will be output.
     *
     * process() assumes that the a proper amount of indentation has already been
     * output. It won't write any further indentation before the 1st line. If
     * more than 1 line is written due to buffer constraints, the lines following
     * the first will be indented by this method, though.
     *
     * No \\n is written by this method after the last line that is written.
     *
     * @param write where to write the data.
     * @param data the new chunk of data to write.
     * @param len the length of the chunk of data to write.
     */
    void process(IStringWriter& write, const char* data, int len)
    {
      wrote_something = false;

      while (len > 0)
      {
        if (len <= width) // quick test that works because utf8width <= len (all wide chars have at least 2 bytes)
        {
          output(write, data, len);
          len = 0;
        }
        else // if (len > width)  it's possible (but not guaranteed) that utf8len > width
        {
          int utf8width = 0;
          int maxi = 0;
          while (maxi < len && utf8width < width)
          {
            int charbytes = 1;
            unsigned ch = static_cast<unsigned char>( data[maxi] );
            if (ch > 0xC1) // everything <= 0xC1 (yes, even 0xC1 itself) is not a valid UTF-8 start byte
            {
              // int __builtin_clz (unsigned int x)
              // Returns the number of leading 0-bits in x, starting at the most significant bit
              unsigned mask = static_cast<unsigned>( -1 ) >> __builtin_clz(ch ^ 0xff);
              ch = ch & mask; // mask out length bits, we don't verify their correctness
              while ((maxi + charbytes < len) && //
                  ((static_cast<unsigned char>( data[maxi + charbytes] ) ^ 0x80) <= 0x3F)) // while next byte is continuation byte
              {
                ch = (ch << 6) ^ static_cast<unsigned char>( data[maxi + charbytes] ) ^ 0x80; // add continuation to char code
                ++charbytes;
              }
              // ch is the decoded unicode code point
              if (ch >= 0x1100 && isWideChar(ch)) // the test for 0x1100 is here to avoid the function call in the Latin case
              {
                if (utf8width + 2 > width)
                  break;
                ++utf8width;
              }
            }
            ++utf8width;
            maxi += charbytes;
          }

          // data[maxi-1] is the last byte of the UTF-8 sequence of the last character that fits
          // onto the 1st line. If maxi == len, all characters fit on the line.

          if (maxi == len)
          {
            output(write, data, len);
            len = 0;
          }
          else // if (maxi < len)  at least 1 character (data[maxi] that is) doesn't fit on the line
          {
            int i;
            for (i = maxi; i >= 0; --i)
              if (data[i] == ' ')
                break;

            if (i >= 0)
            {
              output(write, data, i);
              data += i + 1;
              len -= i + 1;
            }
            else // did not find a space to split at => split before data[maxi]
            { // data[maxi] is always the beginning of a character, never a continuation byte
              output(write, data, maxi);
              data += maxi;
              len -= maxi;
            }
          }
        }
      }
      if (!wrote_something) // if we didn't already write something to make space in the buffer
        write_one_line(write); // write at most one line of actual output
    }

    /**
     * @brief Constructs a LineWrapper that wraps its output to fit into
     * screen columns @c x1 (incl.) to @c x2 (excl.).
     *
     * @c x1 gives the indentation LineWrapper uses if it needs to indent.
     */
    LineWrapper(int x1, int x2) :
        x(x1), width(x2 - x1), head(0), tail(bufmask)
    {
      if (width < 2) // because of wide characters we need at least width 2 or the code breaks
        width = 2;
    }
  };

  /**
   * @internal
   * @brief This is the implementation that is shared between all printUsage() templates.
   * Because all printUsage() templates share this implementation, there is no template bloat.
   */
  static void printUsage(IStringWriter& write, const Descriptor usage[], int width = 80, //
                         int last_column_min_percent = 50, int last_column_own_line_max_percent = 75)
  {
    if (width < 1) // protect against nonsense values
      width = 80;

    if (width > 10000) // protect against overflow in the following computation
      width = 10000;

    int last_column_min_width = ((width * last_column_min_percent) + 50) / 100;
    int last_column_own_line_max_width = ((width * last_column_own_line_max_percent) + 50) / 100;
    if (last_column_own_line_max_width == 0)
      last_column_own_line_max_width = 1;

    LinePartIterator part(usage);
    while (part.nextTable())
    {

      /***************** Determine column widths *******************************/

      const int maxcolumns = 8; // 8 columns are enough for everyone
      int col_width[maxcolumns];
      int lastcolumn;
      int leftwidth;
      int overlong_column_threshold = 10000;
      do
      {
        lastcolumn = 0;
        for (int i = 0; i < maxcolumns; ++i)
          col_width[i] = 0;

        part.restartTable();
        while (part.nextRow())
        {
          while (part.next())
          {
            if (part.column() < maxcolumns)
            {
              upmax(lastcolumn, part.column());
              if (part.screenLength() < overlong_column_threshold)
                // We don't let rows that don't use table separators (\t or \v) influence
                // the width of column 0. This allows the user to interject section headers
                // or explanatory paragraphs that do not participate in the table layout.
                if (part.column() > 0 || part.line() > 0 || part.data()[part.length()] == '\t'
                    || part.data()[part.length()] == '\v')
                  upmax(col_width[part.column()], part.screenLength());
            }
          }
        }

        /*
         * If the last column doesn't fit on the same
         * line as the other columns, we can fix that by starting it on its own line.
         * However we can't do this for any of the columns 0..lastcolumn-1.
         * If their sum exceeds the maximum width we try to fix this by iteratively
         * ignoring the widest line parts in the width determination until
         * we arrive at a series of column widths that fit into one line.
         * The result is a layout where everything is nicely formatted
         * except for a few overlong fragments.
         * */

        leftwidth = 0;
        overlong_column_threshold = 0;
        for (int i = 0; i < lastcolumn; ++i)
        {
          leftwidth += col_width[i];
          upmax(overlong_column_threshold, col_width[i]);
        }

      } while (leftwidth > width);

      /**************** Determine tab stops and last column handling **********************/

      int tabstop[maxcolumns];
      tabstop[0] = 0;
      for (int i = 1; i < maxcolumns; ++i)
        tabstop[i] = tabstop[i - 1] + col_width[i - 1];

      int rightwidth = width - tabstop[lastcolumn];
      bool print_last_column_on_own_line = false;
      if (rightwidth < last_column_min_width &&  // if we don't have the minimum requested width for the last column
            ( col_width[lastcolumn] == 0 ||      // and all last columns are > overlong_column_threshold
              rightwidth < col_width[lastcolumn] // or there is at least one last column that requires more than the space available
            )
          )
      {
        print_last_column_on_own_line = true;
        rightwidth = last_column_own_line_max_width;
      }

      // If lastcolumn == 0 we must disable print_last_column_on_own_line because
      // otherwise 2 copies of the last (and only) column would be output.
      // Actually this is just defensive programming. It is currently not
      // possible that lastcolumn==0 and print_last_column_on_own_line==true
      // at the same time, because lastcolumn==0 => tabstop[lastcolumn] == 0 =>
      // rightwidth==width => rightwidth>=last_column_min_width  (unless someone passes
      // a bullshit value >100 for last_column_min_percent) => the above if condition
      // is false => print_last_column_on_own_line==false
      if (lastcolumn == 0)
        print_last_column_on_own_line = false;

      LineWrapper lastColumnLineWrapper(width - rightwidth, width);
      LineWrapper interjectionLineWrapper(0, width);

      part.restartTable();

      /***************** Print out all rows of the table *************************************/

      while (part.nextRow())
      {
        int x = -1;
        while (part.next())
        {
          if (part.column() > lastcolumn)
            continue; // drop excess columns (can happen if lastcolumn == maxcolumns-1)

          if (part.column() == 0)
          {
            if (x >= 0)
              write("\n", 1);
            x = 0;
          }

          indent(write, x, tabstop[part.column()]);

          if ((part.column() < lastcolumn)
              && (part.column() > 0 || part.line() > 0 || part.data()[part.length()] == '\t'
                  || part.data()[part.length()] == '\v'))
          {
            write(part.data(), part.length());
            x += part.screenLength();
          }
          else // either part.column() == lastcolumn or we are in the special case of
               // an interjection that doesn't contain \v or \t
          {
            // NOTE: This code block is not necessarily executed for
            // each line, because some rows may have fewer columns.

            LineWrapper& lineWrapper = (part.column() == 0) ? interjectionLineWrapper : lastColumnLineWrapper;

            if (!print_last_column_on_own_line || part.column() != lastcolumn)
              lineWrapper.process(write, part.data(), part.length());
          }
        } // while

        if (print_last_column_on_own_line)
        {
          part.restartRow();
          while (part.next())
          {
            if (part.column() == lastcolumn)
            {
              write("\n", 1);
              int _ = 0;
              indent(write, _, width - rightwidth);
              lastColumnLineWrapper.process(write, part.data(), part.length());
            }
          }
        }

        write("\n", 1);
        lastColumnLineWrapper.flush(write);
        interjectionLineWrapper.flush(write);
      }
    }
  }

}
;

/**
 * @brief Outputs a nicely formatted usage string with support for multi-column formatting
 * and line-wrapping.
 *
 * printUsage() takes the @c help texts of a Descriptor[] array and formats them into
 * a usage message, wrapping lines to achieve the desired output width.
 *
 * <b>Table formatting:</b>
 *
 * Aside from plain strings which are simply line-wrapped, the usage may contain tables. Tables
 * are used to align elements in the output.
 *
 * @code
 * // Without a table. The explanatory texts are not aligned.
 * -c, --create  |Creates something.
 * -k, --kill  |Destroys something.
 *
 * // With table formatting. The explanatory texts are aligned.
 * -c, --create  |Creates something.
 * -k, --kill    |Destroys something.
 * @endcode
 *
 * Table formatting removes the need to pad help texts manually with spaces to achieve
 * alignment. To create a table, simply insert \\t (tab) characters to separate the cells
 * within a row.
 *
 * @code
 * const option::Descriptor usage[] = {
 * {..., "-c, --create  \tCreates something." },
 * {..., "-k, --kill  \tDestroys something." }, ...
 * @endcode
 *
 * Note that you must include the minimum amount of space desired between cells yourself.
 * Table formatting will insert further spaces as needed to achieve alignment.
 *
 * You can insert line breaks within cells by using \\v (vertical tab).
 *
 * @code
 * const option::Descriptor usage[] = {
 * {..., "-c,\v--create  \tCreates\vsomething." },
 * {..., "-k,\v--kill  \tDestroys\vsomething." }, ...
 *
 * // results in
 *
 * -c,       Creates
 * --create  something.
 * -k,       Destroys
 * --kill    something.
 * @endcode
 *
 * You can mix lines that do not use \\t or \\v with those that do. The plain
 * lines will not mess up the table layout. Alignment of the table columns will
 * be maintained even across these interjections.
 *
 * @code
 * const option::Descriptor usage[] = {
 * {..., "-c, --create  \tCreates something." },
 * {..., "----------------------------------" },
 * {..., "-k, --kill  \tDestroys something." }, ...
 *
 * // results in
 *
 * -c, --create  Creates something.
 * ----------------------------------
 * -k, --kill    Destroys something.
 * @endcode
 *
 * You can have multiple tables within the same usage whose columns are
 * aligned independently. Simply insert a dummy Descriptor with @c help==0.
 *
 * @code
 * const option::Descriptor usage[] = {
 * {..., "Long options:" },
 * {..., "--very-long-option  \tDoes something long." },
 * {..., "--ultra-super-mega-long-option  \tTakes forever to complete." },
 * {..., 0 }, // ---------- table break -----------
 * {..., "Short options:" },
 * {..., "-s  \tShort." },
 * {..., "-q  \tQuick." }, ...
 *
 * // results in
 *
 * Long options:
 * --very-long-option              Does something long.
 * --ultra-super-mega-long-option  Takes forever to complete.
 * Short options:
 * -s  Short.
 * -q  Quick.
 *
 * // Without the table break it would be
 *
 * Long options:
 * --very-long-option              Does something long.
 * --ultra-super-mega-long-option  Takes forever to complete.
 * Short options:
 * -s                              Short.
 * -q                              Quick.
 * @endcode
 *
 * <b>Output methods:</b>
 *
 * Because TheLeanMeanC++Option parser is freestanding, you have to provide the means for
 * output in the first argument(s) to printUsage(). Because printUsage() is implemented as
 * a set of template functions, you have great flexibility in your choice of output
 * method. The following example demonstrates typical uses. Anything that's similar enough
 * will work.
 *
 * @code
 * #include <unistd.h>  // write()
 * #include <iostream>  // cout
 * #include <sstream>   // ostringstream
 * #include <cstdio>    // fwrite()
 * using namespace std;
 *
 * void my_write(const char* str, int size) {
 *   fwrite(str, size, 1, stdout);
 * }
 *
 * struct MyWriter {
 *   void write(const char* buf, size_t size) const {
 *      fwrite(str, size, 1, stdout);
 *   }
 * };
 *
 * struct MyWriteFunctor {
 *   void operator()(const char* buf, size_t size) {
 *      fwrite(str, size, 1, stdout);
 *   }
 * };
 * ...
 * printUsage(my_write, usage);    // custom write function
 * printUsage(MyWriter(), usage);  // temporary of a custom class
 * MyWriter writer;
 * printUsage(writer, usage);      // custom class object
 * MyWriteFunctor wfunctor;
 * printUsage(&wfunctor, usage);   // custom functor
 * printUsage(write, 1, usage);    // write() to file descriptor 1
 * printUsage(cout, usage);        // an ostream&
 * printUsage(fwrite, stdout, usage);  // fwrite() to stdout
 * ostringstream sstr;
 * printUsage(sstr, usage);        // an ostringstream&
 *
 * @endcode
 *
 * @par Notes:
 * @li the @c write() method of a class that is to be passed as a temporary
 *     as @c MyWriter() is in the example, must be a @c const method, because
 *     temporary objects are passed as const reference. This only applies to
 *     temporary objects that are created and destroyed in the same statement.
 *     If you create an object like @c writer in the example, this restriction
 *     does not apply.
 * @li a functor like @c MyWriteFunctor in the example must be passed as a pointer.
 *     This differs from the way functors are passed to e.g. the STL algorithms.
 * @li All printUsage() templates are tiny wrappers around a shared non-template implementation.
 *     So there's no penalty for using different versions in the same program.
 * @li printUsage() always interprets Descriptor::help as UTF-8 and always produces UTF-8-encoded
 *     output. If your system uses a different charset, you must do your own conversion. You
 *     may also need to change the font of the console to see non-ASCII characters properly.
 *     This is particularly true for Windows.
 * @li @b Security @b warning: Do not insert untrusted strings (such as user-supplied arguments)
 *     into the usage. printUsage() has no protection against malicious UTF-8 sequences.
 *
 * @param prn The output method to use. See the examples above.
 * @param usage the Descriptor[] array whose @c help texts will be formatted.
 * @param width the maximum number of characters per output line. Note that this number is
 *        in actual characters, not bytes. printUsage() supports UTF-8 in @c help and will
 *        count multi-byte UTF-8 sequences properly. Asian wide characters are counted
 *        as 2 characters.
 * @param last_column_min_percent (0-100) The minimum percentage of @c width that should be available
 *        for the last column (which typically contains the textual explanation of an option).
 *        If less space is available, the last column will be printed on its own line, indented
 *        according to @c last_column_own_line_max_percent.
 * @param last_column_own_line_max_percent (0-100) If the last column is printed on its own line due to
 *        less than @c last_column_min_percent of the width being available, then only
 *        @c last_column_own_line_max_percent of the extra line(s) will be used for the
 *        last column's text. This ensures an indentation. See example below.
 *
 * @code
 * // width=20, last_column_min_percent=50 (i.e. last col. min. width=10)
 * --3456789 1234567890
 *           1234567890
 *
 * // width=20, last_column_min_percent=75 (i.e. last col. min. width=15)
 * // last_column_own_line_max_percent=75
 * --3456789
 *      123456789012345
 *      67890
 *
 * // width=20, last_column_min_percent=75 (i.e. last col. min. width=15)
 * // last_column_own_line_max_percent=33 (i.e. max. 5)
 * --3456789
 *                12345
 *                67890
 *                12345
 *                67890
 * @endcode
 */
template<typename OStream>
void printUsage(OStream& prn, const Descriptor usage[], int width = 80, int last_column_min_percent = 50,
                int last_column_own_line_max_percent = 75)
{
  PrintUsageImplementation::OStreamWriter<OStream> write(prn);
  PrintUsageImplementation::printUsage(write, usage, width, last_column_min_percent, last_column_own_line_max_percent);
}

template<typename Function>
void printUsage(Function* prn, const Descriptor usage[], int width = 80, int last_column_min_percent = 50,
                int last_column_own_line_max_percent = 75)
{
  PrintUsageImplementation::FunctionWriter<Function> write(prn);
  PrintUsageImplementation::printUsage(write, usage, width, last_column_min_percent, last_column_own_line_max_percent);
}

template<typename Temporary>
void printUsage(const Temporary& prn, const Descriptor usage[], int width = 80, int last_column_min_percent = 50,
                int last_column_own_line_max_percent = 75)
{
  PrintUsageImplementation::TemporaryWriter<Temporary> write(prn);
  PrintUsageImplementation::printUsage(write, usage, width, last_column_min_percent, last_column_own_line_max_percent);
}

template<typename Syscall>
void printUsage(Syscall* prn, int fd, const Descriptor usage[], int width = 80, int last_column_min_percent = 50,
                int last_column_own_line_max_percent = 75)
{
  PrintUsageImplementation::SyscallWriter<Syscall> write(prn, fd);
  PrintUsageImplementation::printUsage(write, usage, width, last_column_min_percent, last_column_own_line_max_percent);
}

template<typename Function, typename Stream>
void printUsage(Function* prn, Stream* stream, const Descriptor usage[], int width = 80, int last_column_min_percent =
                    50,
                int last_column_own_line_max_percent = 75)
{
  PrintUsageImplementation::StreamWriter<Function, Stream> write(prn, stream);
  PrintUsageImplementation::printUsage(write, usage, width, last_column_min_percent, last_column_own_line_max_percent);
}

}
// namespace option

#endif /* OPTIONPARSER_H_ */
