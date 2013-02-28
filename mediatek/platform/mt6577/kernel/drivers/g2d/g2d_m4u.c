#if defined(MTK_M4U_SUPPORT)

#include <linux/module.h> 
#include <asm/io.h>
#include <mach/m4u.h>
#include "g2d_drv.h"

M4U_EXPORT_FUNCTION_STRUCT _m4u_g2d_func = {0};
EXPORT_SYMBOL(_m4u_g2d_func);

unsigned int _m4u_g2d_init(void)
{
    M4U_PORT_STRUCT m4u_port;

    if (!_m4u_g2d_func.isInit)
    {
        G2D_ERR("m4u is not initialized\n");
        return -1;
    }

    m4u_port.ePortID    = M4U_PORT_G2D_R;
    m4u_port.Virtuality = 1;
    m4u_port.Security   = 0;
    m4u_port.Distance   = 1;
    m4u_port.Direction  = 0;

    _m4u_g2d_func.m4u_config_port(&m4u_port);

    m4u_port.ePortID    = M4U_PORT_G2D_W;
    m4u_port.Virtuality = 1;
    m4u_port.Security   = 0;
    m4u_port.Distance   = 1;
    m4u_port.Direction  = 0;

    _m4u_g2d_func.m4u_config_port(&m4u_port);

    return 0;
}
EXPORT_SYMBOL(_m4u_g2d_init);

int g2d_allocate_mva(unsigned int va, unsigned int *mva, unsigned int size)
{
    int err = 0;

    if (!_m4u_g2d_func.isInit)
    {
        G2D_ERR("m4u is not initialized\n");
        return -1;
    }

    G2D_DBG("try to allocate mva with 0x%x (%d)\n", va, size);

    err = _m4u_g2d_func.m4u_alloc_mva(M4U_CLNTMOD_G2D, va, size, mva);
    if (err != 0)
    {
        G2D_ERR("failed to allocate mva\n");
        return -1;
    }

    err = _m4u_g2d_func.m4u_insert_tlb_range(M4U_CLNTMOD_G2D, *mva, *mva + size - 1, RT_RANGE_HIGH_PRIORITY, 1);
    if (err != 0)
    {
        G2D_ERR("failed to insert m4u tlb\n");
        _m4u_g2d_func.m4u_dealloc_mva(M4U_CLNTMOD_G2D, va, size, *mva);
        return -1;
    }

    return 0;
}

int g2d_deallocate_mva(unsigned int va, unsigned int mva, unsigned int size)
{
    if (!_m4u_g2d_func.isInit)
    {
        G2D_ERR("m4u is not initialized\n");
        return -1;
    }

    G2D_DBG("try to deallocate mva with 0x%x (%d)\n", va, size);

    _m4u_g2d_func.m4u_invalid_tlb_range(M4U_CLNTMOD_G2D, mva, mva + size - 1);
    _m4u_g2d_func.m4u_dealloc_mva(M4U_CLNTMOD_G2D, va, size, mva);
    return 0;
}

#endif
