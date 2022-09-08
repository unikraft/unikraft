#include "DEFS.h"

ENTRY(_bcmp)
	RETGUARD_SETUP(_bcmp, r11)
	xorl	%eax,%eax		/* clear return value */
	cld				/* set compare direction forward */

	movq	%rdx,%rcx		/* compare by words */
	shrq	$3,%rcx
	repe
	cmpsq
	jne	L1

	movq	%rdx,%rcx		/* compare remainder by bytes */
	andq	$7,%rcx
	repe
	cmpsb
	je	L2

L1:	incl	%eax
L2:	RETGUARD_CHECK(_bcmp, r11)
	ret
END_WEAK(_bcmp)
