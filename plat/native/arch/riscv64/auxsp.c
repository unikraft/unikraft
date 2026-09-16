#include <uk/arch/types.h>
#include <uk/pcpuvar.h>

__uk_pcpuvar __uptr uk_plat_native_auxsp;

void uk_plat_native_set_auxsp(__uptr auxsp)
{
	uk_pcpuvar_current_set(uk_plat_native_auxsp, auxsp);
}
