#include <dev/apic.h>
#include <cpuid.h>
#include <msr.h>
#include <paging.h>
#include <nautilus.h>


#ifndef NAUT_CONFIG_DEBUG_APIC
#undef DEBUG_PRINT
#define DEBUG_PRINT(fmt, args...)
#endif

// TODO: FIX THIS
static struct apic_dev * hack = NULL;


int 
check_apic_avail (void)
{
    cpuid_ret_t cp;
    struct cpuid_feature_flags * flags;

    cp    = cpuid(CPUID_FEATURE_INFO);
    flags = (struct cpuid_feature_flags *)&cp.c;

    return flags->edx.apic;
}


static int 
apic_is_bsp (struct apic_dev * apic)
{
    uint64_t data;
    data = msr_read(IA32_APIC_BASE_MSR);
    return APIC_IS_BSP(data);
}


static int
apic_sw_enable (struct apic_dev * apic)
{
    uint32_t val;

    /* TODO: KCH: should we clear the LVT0 reg? */
    //apic_write(apic, APIC_REG_LVT0, 0); 
    val = apic_read(apic, APIC_REG_SPIV);
    apic_write(apic, APIC_REG_SPIV, val | APIC_SPIV_SW_ENABLE);
    return 0;
}


static int
apic_sw_disable (struct apic_dev * apic)
{
    uint32_t val;
    val = apic_read(apic, APIC_REG_SPIV);
    apic_write(apic, APIC_REG_SPIV, val & ~APIC_SPIV_SW_ENABLE);
    return 0;
}


static void
assign_spiv (uint8_t spiv_vec)
{
    /* TODO: fill in */

}


static void 
apic_enable (struct apic_dev * apic) 
{
    uint64_t data;
    data = msr_read(IA32_APIC_BASE_MSR);
    msr_write(IA32_APIC_BASE_MSR, data | APIC_GLOBAL_ENABLE);
    apic_sw_enable(apic);
}


/*
int 
apic_int_cmd (struct apic_dev * apic, 
              uint8_t vec,
              uint8_t dst)
{
    return 0;
}
*/


static ulong_t 
apic_get_base_addr (void) 
{
    uint64_t data;
    data = msr_read(IA32_APIC_BASE_MSR);

    // we're assuming PAE is on
    return (addr_t)(data & APIC_BASE_ADDR_MASK);
}


static void
apic_set_base_addr (struct apic_dev * apic, addr_t addr)
{
    uint64_t data;
    data = msr_read(IA32_APIC_BASE_MSR);
    
    msr_write(IA32_APIC_BASE_MSR, (addr & APIC_BASE_ADDR_MASK) | (data & 0xfff));
}


void 
apic_do_eoi (void)
{
    //apic_write(apic, APIC_REG_EOR, 0);
    apic_write(hack, APIC_REG_EOR, 0);
}


static uint32_t
apic_get_id (struct apic_dev * apic)
{
    return apic_read(apic, APIC_REG_ID) >> APIC_ID_SHIFT;
}


static inline uint8_t 
apic_get_version (struct apic_dev * apic)
{
    return APIC_VERSION(apic_read(apic, APIC_REG_LVR));
}


void 
apic_ipi (struct apic_dev * apic, 
          uint_t remote_id,
          uint_t vector)
{
    apic_write(apic, APIC_REG_ICR2, remote_id << APIC_ICR2_DST_SHIFT);
    apic_write(apic, APIC_REG_ICR, vector | APIC_ICR_LEVEL_ASSERT);
}


void
apic_self_ipi (struct apic_dev * apic, uint_t vector)
{
    apic_write(apic, APIC_IPI_SELF | APIC_ICR_TYPE_FIXED, vector);
}


void
apic_init (struct apic_dev * apic)
{

    if (!check_apic_avail()) {
        panic("no APIC found, dying\n");
    } 

    apic->base_addr = apic_get_base_addr();
    DEBUG_PRINT("apic base addr: %p\n", apic->base_addr);

    DEBUG_PRINT("Reserving APIC region\n");

    if (reserve_page(apic->base_addr) < 0) {
        DEBUG_PRINT("LAPIC mem region already reserved\n");
    }
    
    /* map in the lapic as uncacheable */
    create_page_mapping(apic->base_addr, 
                        apic->base_addr, 
                        PTE_PRESENT_BIT|PTE_WRITABLE_BIT|PTE_CACHE_DISABLE_BIT);

    apic->version   = apic_get_version(apic);
    apic->id        = apic_get_id(apic);

    DEBUG_PRINT("Found LAPIC (version=0x%x, id=0x%x)\n", apic->version, apic->id);

    if (apic->version < 0x10 || apic->version > 0x15) {
        panic("Unsupported APIC version (0x%1x)\n", (unsigned)apic->version);
    }

    /* TODO: these shouldn't be hard-coded! */
    apic_write(apic, APIC_REG_TPR, 0x20); // inhibit softint delivery
    apic_write(apic, APIC_REG_LVTT, 0x10000); // disable timer interrupts
    apic_write(apic, APIC_REG_LVTPC, 0x10000); // disable perf cntr interrupts
    apic_write(apic, APIC_REG_LVT0, 0x08700); // enable normal external interrupts
    apic_write(apic, APIC_REG_LVT1, 0x00400); // enable normal NMI processing
    apic_write(apic, APIC_REG_LVTERR, 0x10000); // disable error interrupts
    apic_write(apic, APIC_REG_SPIV, 0x010f); // may not need this (apic sets spur vector num to 15)

    apic_enable(apic);

    apic_write(apic, APIC_REG_LVT0, 0x08700);  // BAM BAM BAM
    apic_write(apic, APIC_REG_SPIV, 0x010f); 
    hack = apic;
}
