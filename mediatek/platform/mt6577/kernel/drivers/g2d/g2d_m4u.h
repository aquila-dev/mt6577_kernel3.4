#ifndef __G2D_M4U_H__
#define __G2D_M4U_H__

#if defined(MTK_M4U_SUPPORT)
int g2d_allocate_mva(unsigned int va, unsigned int *mva, unsigned int size);
int g2d_deallocate_mva(unsigned int va, unsigned int mva, unsigned int size);
#endif

#endif // __G2D_M4U_H__
