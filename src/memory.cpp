#include "memory.h"

void* gpuAllocMap(size_t size, SceKernelMemBlockType type, SceGxmMemoryAttribFlags gpuAttribs, SceUID* uid)
{
	int err = SCE_OK;
	SceUID memUid;
	void* memAddr = nullptr;

	sceClibPrintf("Allocating GPU memory of size 0x%08X\n", size);
	sceClibPrintf("Memory type: %d\n", type);

	if (type == SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW)
	{
		size = ALIGN(size, 256 * 1024);
	}
	else
	{
		size = ALIGN(size, 4 * 1024);
	}

	memUid = sceKernelAllocMemBlock("gpumem", type, size, NULL);
	if (memUid >= SCE_OK)
	{
		err = sceKernelGetMemBlockBase(memUid, &memAddr);
		if (err == SCE_OK)
		{
			err = sceGxmMapMemory(memAddr, size, gpuAttribs);
			if (err == SCE_OK)
			{
				sceClibPrintf("Allocated GPU memory at address %p with UID 0x%08X\n", (void*)memAddr, memUid);
				*uid = memUid;
				return memAddr;
			}
			else
			{
				sceClibPrintf("ERROR: sceGxmMapMemory failed: 0x%08X\n", err);
			}
		}
		else
		{
			sceClibPrintf("ERROR: sceKernelGetMemBlockBase failed: 0x%08X\n", err);
		}
	}
	else
	{
		sceClibPrintf("ERROR: sceKernelAllocMemBlock failed: 0x%08X\n", memUid);
	}

	return nullptr;
}

void gpuFreeUnmap(SceUID uid)
{
	void* addr;

	if (sceKernelGetMemBlockBase(uid, &addr) < 0)
	{
		sceClibPrintf("WARNING: gpuFreeUnmap UID 0x%08X called but sceKernelGetMemBlockBase found nothing to free!\n", uid);
		return;
	}

	sceGxmUnmapMemory(addr);
	sceKernelFreeMemBlock(uid);
}

void* gpuVertexUsseAllocMap(size_t size, unsigned int* usseOffset, SceUID* uid)
{
	int err = SCE_OK;
	SceUID memUid;
	void* memAddr = nullptr;

	sceClibPrintf("Allocating Vertex USSE memory of size 0x%08X\n", size);

	size = ALIGN(size, 4 * 1024);

	memUid = sceKernelAllocMemBlock("vertex_usse", SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE, size, NULL);
	if (memUid >= SCE_OK)
	{
		err = sceKernelGetMemBlockBase(memUid, &memAddr);
		if (err == SCE_OK)
		{
			err = sceGxmMapVertexUsseMemory(memAddr, size, usseOffset);
			if (err == SCE_OK)
			{
				sceClibPrintf("Allocated Vertex USSE memory at address %p with UID 0x%08X\n", (void*)memAddr, memUid);
				*uid = memUid;
				return memAddr;
			}
			else
			{
				sceClibPrintf("ERROR: sceGxmMapVertexUsseMemory failed: 0x%08X\n", err);
			}
		}
		else
		{
			sceClibPrintf("ERROR: sceKernelGetMemBlockBase failed: 0x%08X\n", err);
		}
	}
	else
	{
		sceClibPrintf("ERROR: sceKernelAllocMemBlock failed: 0x%08X\n", memUid);
	}

	sceClibPrintf("ERROR: VertexUsseAllocMap failed to map aligned memory at address %p", (void*)memAddr);

	return nullptr;
}

void gpuVertexUsseFreeUnmap(SceUID uid)
{
	void* addr;

	if (sceKernelGetMemBlockBase(uid, &addr) < 0)
	{
		sceClibPrintf("WARNING: gpuVertexUsseFreeUnmap UID 0x%08X called but sceKernelGetMemBlockBase found nothing to free!\n", uid);
		return;
	}

	sceGxmUnmapVertexUsseMemory(addr);
	sceKernelFreeMemBlock(uid);
}

void* gpuFragmentUsseAllocMap(size_t size, unsigned int* usseOffset, SceUID* uid)
{
	int err = SCE_OK;
	SceUID memUid;
	void* memAddr = nullptr;

	sceClibPrintf("Allocating Fragment USSE memory of size 0x%08X\n", size);

	size = ALIGN(size, 4 * 1024);

	memUid = sceKernelAllocMemBlock("fragment_usse", SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE, size, NULL);
	if (memUid >= SCE_OK)
	{
		err = sceKernelGetMemBlockBase(memUid, &memAddr);
		if (err == SCE_OK)
		{
			err = sceGxmMapFragmentUsseMemory(memAddr, size, usseOffset);
			if (err == SCE_OK)
			{
				sceClibPrintf("Allocated Fragment USSE memory at address %p with UID 0x%08X\n", (void*)memAddr, memUid);
				*uid = memUid;
				return memAddr;
			}
			else
			{
				sceClibPrintf("ERROR: sceGxmMapFragmentUsseMemory failed: 0x%08X\n", err);
			}
		}
		else
		{
			sceClibPrintf("ERROR: sceKernelGetMemBlockBase failed: 0x%08X\n", err);
		}
	}
	else
	{
		sceClibPrintf("ERROR: sceKernelAllocMemBlock failed: 0x%08X\n", memUid);
	}

	sceClibPrintf("ERROR: FragmentUsseAllocMap failed to map aligned memory at address %p", (void*)memAddr);

	return nullptr;
}

void gpuFragmentUsseFreeUnmap(SceUID uid)
{
	void* addr;

	if (sceKernelGetMemBlockBase(uid, &addr) < 0)
	{
		sceClibPrintf("WARNING: gpuFragmentUsseFreeUnmap UID 0x%08X called but sceKernelGetMemBlockBase found nothing to free!\n", uid);
		return;
	}

	sceGxmUnmapFragmentUsseMemory(addr);
	sceKernelFreeMemBlock(uid);
}