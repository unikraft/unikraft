#include <uk/config.h>
#include <uk/libparam.h>

#define create_var(x,y) __STRINGCONCAT(x,y)
#ifdef UK_LIBPARAM_PREFIX
SECTIONS
{
	/**
	 * creates a variable name = LOADADDR(section_name);
	 */
	_SECTION_START(
			_LIB_PARAM_SECTION_NAME(UK_LIBPARAM_PREFIX,
						PARAM_SECTION_SUFFIX)
			  ) = LOADADDR(
				create_var(UK_LIBPARAM_PREFIX,__param_arg));

	create_var(UK_LIBPARAM_PREFIX,__param_arg) : {
		KEEP (*(create_var(UK_LIBPARAM_PREFIX,PARAM_SECTION_SUFFIX)))
	}
	/**
	 * creates a variable name = LOADADDR(section_name);
	 */
	_SECTION_STOP(
			_LIB_PARAM_SECTION_NAME(UK_LIBPARAM_PREFIX,
						PARAM_SECTION_SUFFIX)
			  ) = LOADADDR(
				create_var(UK_LIBPARAM_PREFIX,__param_arg)) +
			      SIZEOF(
				create_var(UK_LIBPARAM_PREFIX,__param_arg));
}
INSERT AFTER .rodata
#endif /* UK_LIBPARAM_PREFIX */
