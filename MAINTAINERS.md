We use the same syntax to describe this maintainer list as the Linux kernel.
For your convenience, this is a copy of the description for the section entries:

	M: Mail patches to: FullName <address@domain>
	R: Designated reviewer: FullName <address@domain>
	   These reviewers should be CCed on patches.
	L: Mailing list that is relevant to this area
	W: Web-page with status/info
	B: URI for where to file bugs. A web-page with detailed bug
	   filing info, a direct bug tracker link, or a mailto: URI.
	C: URI for chat protocol, server and channel where developers
	   usually hang out, for example irc://server/channel.
	Q: Patchwork web based patch tracking system site
	T: SCM tree type and location.
	   Type is one of: git, hg, quilt, stgit, topgit
	S: Status, one of the following:
	   Supported:	Someone is actually paid to look after this.
	   Maintained:	Someone actually looks after it.
	   Odd Fixes:	It has a maintainer but they don't have time to do
			much other than throw the odd patch in. See below..
	   Orphan:	No current maintainer [but maybe you could take the
			role as you write your new code].
	   Obsolete:	Old code. Something tagged obsolete generally means
			it has been replaced by a better system and you
			should be using that.
	F: Files and directories with wildcard patterns.
	   A trailing slash includes all files and subdirectory files.
	   F:	lib/net/		all files in and below lib/net
	   F:	lib/net/*		all files in lib/net, but not below
	   F:	*/net/*		all files in "any top level directory"/net
	   One pattern per line.  Multiple F: lines acceptable.
	N: Files and directories with regex patterns.
	   N:	[^a-z]tegra	all files whose path contains the word tegra
	   One pattern per line.  Multiple N: lines acceptable.
	   scripts/get_maintainer.pl has different behavior for files that
	   match F: pattern and matches of N: patterns.  By default,
	   get_maintainer will not look at git log history when an F: pattern
	   match occurs.  When an N: match occurs, git log history is used
	   to also notify the people that have git commit signatures.
	X: Files and directories that are NOT maintained, same rules as F:
	   Files exclusions are tested before file matches.
	   Can be useful for excluding a specific subdirectory, for instance:
	   F:	net/
	   X:	net/ipv6/
	   matches all files in and below net excluding net/ipv6/
	K: Keyword perl extended regex pattern to match content in a
	   patch or file.  For instance:
	   K: of_get_profile
	      matches patches or files that contain "of_get_profile"
	   K: \b(printk|pr_(info|err))\b
	      matches patches or files that contain one or more of the words
	      printk, pr_info or pr_err
	   One regex pattern per line.  Multiple K: lines acceptable.


Maintainers List
================

Try to look for the most precise areas first. In case nothing fits use 
`UNIKRAFT GENERAL`:

	XEN PLATFORM (X86_64, ARM32)
	M:	Simon Kuenzer <simon.kuenzer@neclab.eu>
	M:	Costin Lupu <costin.lupu@cs.pub.ro>
	M:	Felipe Huici <felipe.huici@neclab.eu>
	L:	minios-devel@lists.xen.org
	F: plat/xen/*

	LINUX USERSPACE PLATFORM (X86_64, ARM32)
	M:	Simon Kuenzer <simon.kuenzer@neclab.eu>
	M:	Sharan Santhanam <sharan.santhanam@neclab.eu>
	L:	minios-devel@lists.xen.org
	F: plat/linuxu/*

	KVM PLATFORM (X86)
	M:	Simon Kuenzer <simon.kuenzer@neclab.eu>
	M:	Felipe Huici <felipe.huici@neclab.eu>
	M:	Sharan Santhanam <sharan.santhanam@neclab.eu>
	L:	minios-devel@lists.xen.org
	F: plat/linuxu/*

	LIBFDT
	M:	Simon Kuenzer <simon.kuenzer@neclab.eu>
	L:	minios-devel@lists.xen.org
	F: lib/fdt/*

	LIBNOLIBC
	M:	Simon Kuenzer <simon.kuenzer@neclab.eu>
	L:	minios-devel@lists.xen.org
	F: lib/nolibc/*

	LIBUKALLOC
	M:	Simon Kuenzer <simon.kuenzer@neclab.eu>
	L:	minios-devel@lists.xen.org
	F: lib/ukalloc/*

	LIBUKALLOCBBUDDY
	M:	Simon Kuenzer <simon.kuenzer@neclab.eu>
	L:	minios-devel@lists.xen.org
	F: lib/ukallocbbuddy/*

	LIBUKSCHED
	M:	Costin Lupu <costin.lupu@cs.pub.ro>
	M:	Simon Kuenzer <simon.kuenzer@neclab.eu>
	L:	minios-devel@lists.xen.org
	F: lib/uksched/*

	LIBUKSCHEDCOOP
	M:	Costin Lupu <costin.lupu@cs.pub.ro>
	M:	Simon Kuenzer <simon.kuenzer@neclab.eu>
	L:	minios-devel@lists.xen.org
	F: lib/ukschedcoop/*

	LIBUKARGPARSE
	M:	Simon Kuenzer <simon.kuenzer@neclab.eu>
	L:	minios-devel@lists.xen.org
	F: lib/ukargparse/*

	LIBUKBOOT
	M:	Simon Kuenzer <simon.kuenzer@neclab.eu>
	L:	minios-devel@lists.xen.org
	F: lib/ukboot/*

	LIBUKDEBUG
	M:	Simon Kuenzer <simon.kuenzer@neclab.eu>
	L:	minios-devel@lists.xen.org
	F: lib/ukdebug/*

	LIBUKLIBPARAM
	M: Sharan Santhanam <sharan.santhanam@neclab.eu>
	L:	minios-devel@lists.xen.org
	F: lib/uklibparam/*

	UNIKRAFT GENERAL
	M:	Simon Kuenzer <simon.kuenzer@neclab.eu>
	M:	Sharan Santhanam <sharan.santhanam@neclab.eu>
	M:	Felipe Huici <felipe.huici@neclab.eu>
	L:	minios-devel@lists.xen.org
