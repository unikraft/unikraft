#include <uk/event.h>

#include <uk/arch/traps.h>
#include <uk/arch/crash.h>

static int ukarch_invalid_op_handler(void *data) {
	struct ukarch_trap_ctx *ctx = data;
	struct uk_crash_description description;

	/* We only want to handle invalid operation exceptions caused by
	 * explicit UK_CRASH calls. This allows applications to also handle
	 * invalid operation exceptions.
	 */
	if (ukplat_explicit_crash) {
		description.reason = UKARCH_CRASH_REASON_EXPLICIT;
		UK_CRASH_EX(ctx->regs, &description);
	}

	return UK_EVENT_NOT_HANDLED;
}

UK_EVENT_HANDLER_PRIO(UKARCH_TRAP_INVALID_OP,
	ukarch_invalid_op_handler,
	UK_PRIO_EARLIEST);
