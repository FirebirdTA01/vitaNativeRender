#pragma once

#include <psp2/kernel/sysmem.h>
#include <psp2/types.h>
#include <psp2/gxm.h>
#include "commonUtils.h"

#define ALIGN(x, a) (((x) + ((a) - 1)) & ~((a) - 1)) // Align a value to a given alignment

void* gpuAllocMap(size_t size, SceKernelMemBlockType type, SceGxmMemoryAttribFlags gpuAttribs, SceUID* uid);
void gpuFreeUnmap(SceUID uid);

void* gpuVertexUsseAllocMap(size_t size, unsigned int* usseOffset, SceUID* uid);
void gpuVertexUsseFreeUnmap(SceUID uid);

void* gpuFragmentUsseAllocMap(size_t size, unsigned int* usseOffset, SceUID* uid);
void gpuFragmentUsseFreeUnmap(SceUID uid);