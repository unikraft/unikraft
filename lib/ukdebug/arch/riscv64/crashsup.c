#include <uk/crash.h>
#include <uk/essentials.h>
#include <uk/lcpu.h>

extern __bool _uk_crash_explicit;

void uk_crash_populate_descr(struct uk_lcpu_except_err_ctx *ctx,
			     struct uk_crash_descr *descr)
{
	if (_uk_crash_explicit) {
		descr->reason = UK_CRASH_REASON_EXPLICIT;
		descr->uk_err = 0;
		return;
	}

	if (uk_lcpu_riscv64_except_err_ctx_get_eid(ctx) ==
	    UK_LCPU_RISCV64_EXCEPT_ID_PAGE_FAULT) {
		descr->reason = UK_CRASH_REASON_PAGE_FAULT;
		descr->uk_err = uk_lcpu_except_err_ctx_get_handler_err(ctx);
		descr->arg1 = uk_lcpu_except_err_ctx_get_fault_addr(ctx);
		descr->arg2 = uk_lcpu_riscv64_except_err_ctx_get_scause(ctx);
		descr->arg3 = 0;
		return;
	}

	descr->reason = UK_CRASH_REASON_UNHANDLED_TRAP;
	descr->uk_err = 0;
	descr->arg1 = uk_lcpu_riscv64_except_err_ctx_get_eid(ctx);
	descr->arg2 = (__u64)uk_lcpu_except_err_ctx_get_str(ctx);
	descr->arg3 = uk_lcpu_riscv64_except_err_ctx_get_scause(ctx);
}
