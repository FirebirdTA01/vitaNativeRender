#include <psp2/gxm.h>
#include <psp2/kernel/clib.h>
#include <psp2/display.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/ctrl.h>
#include <vector>
#include <stdlib.h> // For malloc, free
#include <string.h> // For memcpy

#include "commonUtils.h"
#include "matrix.h"
#include "camera.h"

#define DISPLAY_WIDTH 960 // Default display width in pixels
#define DISPLAY_HEIGHT 544 // Default display height in pixels
#define DISPLAY_STRIDE 1024
#define DISPLAY_MAX_BUFFER_COUNT 3 // Maximum amount of display buffers to use //not used yet
#define DISPLAY_BUFFER_COUNT 2
#define MAX_PENDING_SWAPS (DISPLAY_BUFFER_COUNT - 1)

#define DISPLAY_COLOR_FORMAT SCE_GXM_COLOR_FORMAT_A8B8G8R8
#define DISPLAY_PIXEL_FORMAT SCE_DISPLAY_PIXELFORMAT_A8B8G8R8

static SceGxmMultisampleMode gxmMultisampleMode; //this is set later in initGxm

//not used yet
#define MAX_IDX_NUMBER 0xC000 // Maximum allowed number of indices per draw call for glDrawArrays

#define ANALOG_THRESHOLD 20 //not used yet

#define ALIGN(x, a) (((x) + ((a) - 1)) & ~((a) - 1)) // Align a value to a given alignment
#define abs(x) (((x) < 0) ? -(x) : (x))

static uint32_t gxmParamBufSize = SCE_GXM_DEFAULT_PARAMETER_BUFFER_SIZE; // Param buffer size for sceGxm
static uint32_t gxmVdmBufSize = SCE_GXM_DEFAULT_VDM_RING_BUFFER_SIZE; // VDM ring buffer size for sceGxm
static uint32_t gxmVertexBufSize = SCE_GXM_DEFAULT_VERTEX_RING_BUFFER_SIZE; // Vertex ring buffer size for sceGxm
static uint32_t gxmFragmentBufSize = SCE_GXM_DEFAULT_FRAGMENT_RING_BUFFER_SIZE; // Fragment ring buffer size for sceGxm
static uint32_t gxmFragmentUsseBufSize = SCE_GXM_DEFAULT_FRAGMENT_USSE_RING_BUFFER_SIZE; // Fragment ring buffer size for sceGxm

static void* vdmRingBufferAddr = NULL; // Address of VDM ring buffer
static void* vertexRingBufferAddr = NULL; // Address of vertex ring buffer
static void* fragmentRingBufferAddr = NULL; // Address of fragment ring buffer
static void* fragmentUsseRingBufferAddr = NULL; // Address of fragment USSE ring buffer
unsigned int fragmentUsseOffset;

static SceUID vdmRingBufferUID;
static SceUID vertexRingBufferUID;
static SceUID fragmentRingBufferUID;
static SceUID fragmentUsseRingBufferUID;

static SceGxmContext* gxmContext = NULL; // Graphics context
static SceGxmRenderTarget* gxmRenderTarget = NULL; // Graphics render target

static SceGxmColorSurface gxmColorSurfaces[DISPLAY_BUFFER_COUNT]; // Color surfaces
static void* gxmColorSurfacesAddr[DISPLAY_BUFFER_COUNT]; // Address of color surface
static SceGxmSyncObject* gxmSyncObjs[DISPLAY_BUFFER_COUNT]; // Sync objects for display buffers
static SceUID gxmColorSurfaceUIDs[DISPLAY_BUFFER_COUNT];

static SceGxmDepthStencilSurface gxmDepthStencilSurface; // Depth stencil surface
static void* gxmDepthStencilSurfaceAddr;
static SceUID gxmDepthStencilSurfaceUID;

static void* gxmShaderPatcherBufferAddr = NULL; // Address of shader patcher buffer
static void* gxmShaderPatcherVertexUsseAddr = NULL;
static void* gxmShaderPatcherFragmentUsseAddr = NULL;

static SceGxmShaderPatcher* gxmShaderPatcher = NULL; // Shader patcher
static SceUID gxmShaderPatcherBufferUID;
static SceUID gxmShaderPatcherVertexUsseUID;
static SceUID gxmShaderPatcherFragmentUsseUID;

unsigned int gxmFrontBufferIndex = DISPLAY_BUFFER_COUNT - 1;
unsigned int gxmBackBufferIndex = 0;

/*
*  Shader Stuff
*/

//alternative to linking against compiled shaders (these need recompiled if they are to be actually used, not up to date)
/*
static unsigned int size_clear_v = 252;
static unsigned char clear_v[] __attribute__((aligned(16))) = {
	0x47, 0x58, 0x50, 0x00, 0x01, 0x05, 0x50, 0x03, 0xf9, 0x00, 0x00, 0x00, 0x90, 0xae, 0x7f, 0xcc,
	0x93, 0x06, 0x6c, 0x2c, 0x04, 0x00, 0x19, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xb8, 0x00, 0x00, 0x00, 0x70, 0x00, 0x00, 0x00,
	0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x04, 0x00, 0x00, 0x00,
	0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x74, 0x00, 0x00, 0x00, 0x70, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x78, 0x00, 0x00, 0x00, 0xc0, 0x3d, 0x03, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x6c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x5c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x54, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x4c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x04,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x44, 0xfa, 0x01, 0x00, 0x04, 0x90, 0x85, 0x11, 0xa5, 0x08,
	0x01, 0x00, 0x56, 0x90, 0x81, 0x11, 0x83, 0x08, 0x00, 0x00, 0x20, 0xa0, 0x00, 0x50, 0x27, 0xfb,
	0x10, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x70, 0x6f, 0x73, 0x69, 0x74, 0x69, 0x6f, 0x6e, 0x00, 0x00, 0x00, 0x00
};

static unsigned int size_clear_f = 236;
static unsigned char clear_f[] __attribute__((aligned(16))) = {
	0x47, 0x58, 0x50, 0x00, 0x01, 0x05, 0x50, 0x03, 0xe9, 0x00, 0x00, 0x00, 0xcd, 0xaa, 0xa1, 0x03,
	0x11, 0xcc, 0xd2, 0x9a, 0x01, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xa4, 0x00, 0x00, 0x00, 0x70, 0x00, 0x00, 0x00,
	0x02, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00,
	0x74, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x68, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x5c, 0x00, 0x00, 0x00, 0xc0, 0x3d, 0x03, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 0x00, 0x00, 0x00,
	0x01, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x04, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x44, 0xfa, 0x02, 0x80, 0x19, 0xf0,
	0x7e, 0x0d, 0x80, 0x40, 0x0e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x10, 0x00, 0x00, 0x00,
	0x01, 0xe4, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x75, 0x5f, 0x63, 0x6c,
	0x65, 0x61, 0x72, 0x43, 0x6f, 0x6c, 0x6f, 0x72, 0x00, 0x00, 0x00, 0x00
};*/

extern unsigned char _binary_clear_v_gxp_start;
extern unsigned char _binary_clear_f_gxp_start;
extern unsigned char _binary_disable_color_buffer_v_gxp_start;
extern unsigned char _binary_disable_color_buffer_f_gxp_start;
extern unsigned char _binary_cube_v_gxp_start;
extern unsigned char _binary_cube_f_gxp_start;
extern unsigned char _binary_basic_v_gxp_start;
extern unsigned char _binary_basic_f_gxp_start;

static const SceGxmProgram* const gxmProgClearVertexGxp = (SceGxmProgram*)&_binary_clear_v_gxp_start;
static const SceGxmProgram* const gxmProgClearFragmentGxp = (SceGxmProgram*)&_binary_clear_f_gxp_start;
static const SceGxmProgram* const gxmProgBasicVertexGxp = (SceGxmProgram*)&_binary_basic_v_gxp_start;
static const SceGxmProgram* const gxmProgBasicFragmentGxp = (SceGxmProgram*)&_binary_basic_f_gxp_start;

static SceGxmShaderPatcherId gxmClearVertexProgramID;
static SceGxmShaderPatcherId gxmClearFragmentProgramID;
static const SceGxmProgramParameter* gxmClearVertexProgram_positionParam;
static const SceGxmProgramParameter* gxmClearFragmentProgram_u_clearColorParam;
static SceGxmVertexProgram* gxmClearVertexProgramPatched;
static SceGxmFragmentProgram* gxmClearFragmentProgramPatched;

static struct ClearVertex* clearVerticesData;
static unsigned short* clearIndicesData;
static SceUID clearVerticesUID;
static SceUID clearIndicesUID;

static SceGxmShaderPatcherId gxmBasicVertexProgramID;
static SceGxmShaderPatcherId gxmBasicFragmentProgramID;
static const SceGxmProgramParameter* gxmBasicVertexProgram_positionParam;
static const SceGxmProgramParameter* gxmBasicVertexProgram_colorParam;
static const SceGxmProgramParameter* gxmBasicVertexProgram_u_modelMatrixParam;
static const SceGxmProgramParameter* gxmBasicVertexProgram_u_viewMatrixParam;
static const SceGxmProgramParameter* gxmBasicVertexProgram_u_projectionMatrixParam;
static SceGxmVertexProgram* gxmBasicVertexProgramPatched;
static SceGxmFragmentProgram* gxmBasicFragmentProgramPatched;

Color clearColor(0.0f, 0.1f, 0.15f, 1.0f); // Clear screen color

/*	Data structure to pass through the display queue.  This structure is
	serialized during sceGxmDisplayQueueAddEntry, and is used to pass
	arbitrary data to the display callback function, called from an internal
	thread once the back buffer is ready to be displayed.

	In this example, we only need to pass the base address of the buffer.
*/
struct DisplayQueueCallbackData
{
	void* addr;
};

static void displayQueueCallback(const void* callbackData)
{
	const struct DisplayQueueCallbackData* cbData = (DisplayQueueCallbackData*)callbackData;

	//populate sceDisplay framebuffer params
	SceDisplayFrameBuf displayFB;
	sceClibMemset(&displayFB, 0, sizeof(SceDisplayFrameBuf));
	displayFB.size = sizeof(SceDisplayFrameBuf);
	displayFB.base = cbData->addr;
	displayFB.pitch = DISPLAY_STRIDE;
	displayFB.pixelformat = SCE_DISPLAY_PIXELFORMAT_A8B8G8R8;
	displayFB.width = DISPLAY_WIDTH;
	displayFB.height = DISPLAY_HEIGHT;

	sceDisplaySetFrameBuf(&displayFB, SCE_DISPLAY_SETBUF_NEXTFRAME);

	sceDisplayWaitVblankStart();
}

static void* patcherHostAllocCallback(void* obj, uint32_t size)
{
	sceClibPrintf("PatcherHostAllocCallback attempting to allocate uint32_t %u size memory\n", size);
	sceClibPrintf("PatcherHostAllocCallback received obj for allocation at address: %p\n", obj);
	//return poolMalloc(size);
	return malloc(size);
}

static void patcherHostFreeCallback(void* obj, void* ptr)
{
	//poolFree(ptr);
	return free(ptr);
}


static void* gpuAllocMap(size_t size, SceKernelMemBlockType type, SceGxmMemoryAttribFlags gpuAttribs, SceUID* uid)
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


// Flags available for sceGxmVshInitialize
enum {
	GXM_FLAG_DEFAULT = 0x00,
	GXM_FLAG_SYSAPP = 0x0A,
	GXM_FLAG_TEXFORMAT_EXT = 0x10
} sceGxmVshInitializeFlags;

void initGxm(int width, int height, SceGxmMultisampleMode msaaMode)
{
	sceClibPrintf("Initializing GXM\n");
	int err = 0;

	//double check that framebuffer size is valid
	int maxWidth = width, maxHeight = height;
	sceDisplayGetMaximumFrameBufResolution(&maxWidth, &maxHeight);
	if (width > maxWidth || height > maxHeight)
	{
		width = maxWidth;
		height = maxHeight;
	}

	gxmMultisampleMode = msaaMode;

	//Initialize the initialize params
	SceGxmInitializeParams gxmInitParams;
	sceClibMemset(&gxmInitParams, 0, sizeof(SceGxmInitializeParams));

	gxmInitParams.flags = GXM_FLAG_DEFAULT;
	gxmInitParams.displayQueueMaxPendingCount = DISPLAY_BUFFER_COUNT - 1;
	gxmInitParams.displayQueueCallback = displayQueueCallback;
	gxmInitParams.displayQueueCallbackDataSize = sizeof(struct DisplayQueueCallbackData);
	gxmInitParams.parameterBufferSize = gxmParamBufSize;

	//Initialize GXM
	//err = sceGxmInitialize(&gxmInitParams);
	err = sceGxmVshInitialize(&gxmInitParams);
	sceClibPrintf("sceGxmInitialize(): 0x%08X\n", err);
}

void initGxmContext()
{
	sceClibPrintf("Initializing GXM context...\n");

	//allocate ring buffers
	vdmRingBufferAddr = gpuAllocMap(gxmVdmBufSize, SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW,
		SCE_GXM_MEMORY_ATTRIB_READ, &vdmRingBufferUID);

	vertexRingBufferAddr = gpuAllocMap(gxmVertexBufSize, SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW,
		SCE_GXM_MEMORY_ATTRIB_READ, &vertexRingBufferUID);

	fragmentRingBufferAddr = gpuAllocMap(gxmFragmentBufSize, SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW,
		SCE_GXM_MEMORY_ATTRIB_READ, &fragmentRingBufferUID);

	fragmentUsseRingBufferAddr = gpuFragmentUsseAllocMap(gxmFragmentUsseBufSize, &fragmentUsseOffset, &fragmentUsseRingBufferUID);

	//set up the context params
	SceGxmContextParams contextParams;
	sceClibMemset(&contextParams, 0, sizeof(SceGxmContextParams));
	contextParams.hostMem = malloc(SCE_GXM_MINIMUM_CONTEXT_HOST_MEM_SIZE);
	contextParams.hostMemSize = SCE_GXM_MINIMUM_CONTEXT_HOST_MEM_SIZE;
	contextParams.vdmRingBufferMem = vdmRingBufferAddr;
	contextParams.vdmRingBufferMemSize = gxmVdmBufSize;
	contextParams.vertexRingBufferMem = vertexRingBufferAddr;
	contextParams.vertexRingBufferMemSize = gxmVertexBufSize;
	contextParams.fragmentRingBufferMem = fragmentRingBufferAddr;
	contextParams.fragmentRingBufferMemSize = gxmFragmentBufSize;
	contextParams.fragmentUsseRingBufferMem = fragmentUsseRingBufferAddr;
	contextParams.fragmentUsseRingBufferMemSize = gxmFragmentUsseBufSize;
	contextParams.fragmentUsseRingBufferOffset = fragmentUsseOffset;

	//print context params
	sceClibPrintf("contextParams.hostMem: 0x%08X\n", contextParams.hostMem);
	sceClibPrintf("contextParams.hostMemSize: 0x%08X\n", contextParams.hostMemSize);
	sceClibPrintf("contextParams.vdmRingBufferMem: 0x%08X\n", contextParams.vdmRingBufferMem);
	sceClibPrintf("contextParams.vdmRingBufferMemSize: 0x%08X\n", contextParams.vdmRingBufferMemSize);
	sceClibPrintf("contextParams.vertexRingBufferMem: 0x%08X\n", contextParams.vertexRingBufferMem);
	sceClibPrintf("contextParams.vertexRingBufferMemSize: 0x%08X\n", contextParams.vertexRingBufferMemSize);
	sceClibPrintf("contextParams.fragmentRingBufferMem: 0x%08X\n", contextParams.fragmentRingBufferMem);
	sceClibPrintf("contextParams.fragmentRingBufferMemSize: 0x%08X\n", contextParams.fragmentRingBufferMemSize);
	sceClibPrintf("contextParams.fragmentUsseRingBufferMem: 0x%08X\n", contextParams.fragmentUsseRingBufferMem);
	sceClibPrintf("contextParams.fragmentUsseRingBufferMemSize: 0x%08X\n", contextParams.fragmentUsseRingBufferMemSize);
	sceClibPrintf("contextParams.fragmentUsseRingBufferOffset: 0x%08X\n", contextParams.fragmentUsseRingBufferOffset);

	//create the context
	int ret = sceGxmCreateContext(&contextParams, &gxmContext);
	sceClibPrintf("sceGxmCreateContext(): 0x%08X\n", ret);
}

void createRenderTarget()
{
	sceClibPrintf("Creating render target...\n");
	//set up parameters
	SceGxmRenderTargetParams renderTargetParams;
	sceClibMemset(&renderTargetParams, 0, sizeof(SceGxmRenderTargetParams));
	renderTargetParams.flags = 0;
	renderTargetParams.width = DISPLAY_WIDTH;
	renderTargetParams.height = DISPLAY_HEIGHT;
	renderTargetParams.scenesPerFrame = 1;
	renderTargetParams.multisampleMode = gxmMultisampleMode;
	renderTargetParams.multisampleLocations = 0;
	renderTargetParams.driverMemBlock = -1;

	//print render target params
	sceClibPrintf("renderTargetParams.flags: 0x%08X\n", renderTargetParams.flags);
	sceClibPrintf("renderTargetParams.width: %u\n", renderTargetParams.width);
	sceClibPrintf("renderTargetParams.height: %u\n", renderTargetParams.height);
	sceClibPrintf("renderTargetParams.scenesPerFrame: %u\n", renderTargetParams.scenesPerFrame);
	sceClibPrintf("renderTargetParams.multisampleMode: 0x%08X\n", renderTargetParams.multisampleMode);
	sceClibPrintf("renderTargetParams.multisampleLocations: 0x%08X\n", renderTargetParams.multisampleLocations);
	sceClibPrintf("renderTargetParams.driverMemBlock: 0x%08X\n", renderTargetParams.driverMemBlock);

	//create the render target
	int ret = sceGxmCreateRenderTarget(&renderTargetParams, &gxmRenderTarget);
	sceClibPrintf("sceGxmCreateRenderTarget(): 0x%08X\n", ret);
}

void initDisplayColorSurfaces()
{
	sceClibPrintf("Initializing display color surfaces...\n");
	for (int i = 0; i < DISPLAY_BUFFER_COUNT; i++)
	{
		//allocate memory for display buffers
		//VRAM should be 4KB aligned
		gxmColorSurfacesAddr[i] = gpuAllocMap(ALIGN(4 * DISPLAY_STRIDE * DISPLAY_HEIGHT, 1 * 1024 * 1024),
			SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW, SCE_GXM_MEMORY_ATTRIB_RW, &gxmColorSurfaceUIDs[i]);
		//memset the buffer to black
		sceClibMemset(gxmColorSurfacesAddr[i], 0, DISPLAY_STRIDE * DISPLAY_HEIGHT);

		//initialize allocated color surface
		int err = sceGxmColorSurfaceInit(&gxmColorSurfaces[i],
			SCE_GXM_COLOR_FORMAT_A8B8G8R8,
			SCE_GXM_COLOR_SURFACE_LINEAR,
			(gxmMultisampleMode == SCE_GXM_MULTISAMPLE_NONE) ? SCE_GXM_COLOR_SURFACE_SCALE_NONE : SCE_GXM_COLOR_SURFACE_SCALE_MSAA_DOWNSCALE,
			SCE_GXM_OUTPUT_REGISTER_SIZE_32BIT,
			DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_STRIDE,
			gxmColorSurfacesAddr[i]);

		sceClibPrintf("sceGxmColorSurfaceInit(): 0x%08X\n", err);

		//create sync objects for the new color surface
		err = sceGxmSyncObjectCreate(&gxmSyncObjs[i]);
		sceClibPrintf("sceGxmSyncObjectCreate(): 0x%08X\n", err);
	}
}

void initDepthStencilSurfaces()
{
	sceClibPrintf("Initializing depth stencil surfaces...\n");
	//Calculate sizes for depth and stencil buffers
	uint32_t alignedWidth = ALIGN(DISPLAY_WIDTH, SCE_GXM_TILE_SIZEX);
	uint32_t alignedHeight = ALIGN(DISPLAY_HEIGHT, SCE_GXM_TILE_SIZEY);
	uint32_t sampleCount = 1;
	if (gxmMultisampleMode != SCE_GXM_MULTISAMPLE_NONE)
	{
		if (gxmMultisampleMode == SCE_GXM_MULTISAMPLE_4X)
			sampleCount = 4;
		else if (gxmMultisampleMode == SCE_GXM_MULTISAMPLE_2X)
			sampleCount = 2;
		else
		{
			//assert(false && "Invalid multisample mode");
		}
	}
	uint32_t depthStencilSamples = alignedWidth * alignedHeight * sampleCount;

	//allocate memory for depth and stencil buffers
	gxmDepthStencilSurfaceAddr = gpuAllocMap(4 * depthStencilSamples, SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW,
		SCE_GXM_MEMORY_ATTRIB_RW, &gxmDepthStencilSurfaceUID);

	/*Depth-Stencil Formats
	The depth-stencil format you choose will have an impact on rendering quality and memory consumption.

	SCE_GXM_DEPTH_STENCIL_FORMAT_DF32M_S8: This format uses a 32-bit floating-point depth value (DF32) and an 8-bit stencil value (S8).
	This will offer very high precision for depth buffering but will also consume more memory compared to other formats.
	It's useful when you need highly accurate depth sorting, for example, in scenes with a high degree of geometric complexity.

	SCE_GXM_DEPTH_STENCIL_FORMAT_S8D24: This format uses an 8-bit stencil value (S8) and a 24-bit fixed-point depth value (D24).
	While not as precise as DF32M, it is generally more than sufficient for most applications and consumes less memory.

	Tiling Options

	SCE_GXM_DEPTH_STENCIL_SURFACE_TILED: When memory is accessed in a pattern more suited to a 2D texture than to a 1D array,
	tiling can offer performance improvements. The PS Vita's GPU can read a "tile" of texture data into cache, which can make
	subsequent reads of nearby texture data faster. This is generally the best choice for most 3D graphics applications.

	SCE_GXM_DEPTH_STENCIL_SURFACE_LINEAR: In a linear layout, data is stored in a straightforward, row-by-row manner. This might be
	easier to manipulate if you are doing CPU-side operations but is generally slower for GPU-side texture sampling compared to tiled formats.

	When to Use Different Combinations

	High-Precision Depth, Tiled (DF32M_S8, TILED): Choose this when you have complex geometry that requires highly accurate depth calculations and you want the best GPU performance.

	Low-Precision Depth, Tiled (S8D24, TILED): This will be the go-to for most applications. It offers a good trade-off between memory usage and performance.

	High-Precision Depth, Linear (DF32M_S8, LINEAR): This could be useful if you have specific needs for CPU-side manipulation of the depth buffer, and you require high precision.

	Low-Precision Depth, Linear (S8D24, LINEAR): Similarly, if you need to read or write depth/stencil values on the CPU and do not need the highest precision, this option would be suitable.
	*/

	//initialize depth and stencil surfaces
	int err = sceGxmDepthStencilSurfaceInit(&gxmDepthStencilSurface,
		SCE_GXM_DEPTH_STENCIL_FORMAT_S8D24,
		SCE_GXM_DEPTH_STENCIL_SURFACE_TILED,
		gxmMultisampleMode == SCE_GXM_MULTISAMPLE_4X ? alignedWidth * 2 : alignedWidth,
		gxmDepthStencilSurfaceAddr,
		NULL);
	sceClibPrintf("sceGxmDepthStencilSurfaceInit(): 0x%08X\n", err);
}

void initShaderPatcher()
{
	sceClibPrintf("Initializing shader patcher...\n");
	int err = 0;
	/*
	Creating a shader patcher for the PSVita involves setting the size for various buffers that the patcher uses
	: the shader patcher buffer, shader patcher vertex USSE size, and shader patcher fragment USSE size.

	These sizes can vary depending on the complexity of the shaders you're using and the resources you have
	available on the PSVita.

		Shader Patcher Buffer : This buffer stores the compiled shaders.A typical size could be around
		64KB to 512KB depending on your needs.If you are working with very complex shaders or a large
		number of them, you might need to allocate more memory.

		Shader Patcher Vertex USSE Size : The Vertex USSE(Unified Shader Subsystem) size determines how
		much memory is allocated for vertex shaders.This is usually smaller than the patcher buffer and
		can be around 16KB to 64KB.

		Shader Patcher Fragment USSE Size : This is similar to the Vertex USSE but for fragment shaders.
		Fragment shaders usually perform more calculations per pixel compared to vertex shaders, so you
		might need a larger buffer here.Typical sizes could be around 16KB to 64KB.
	*/

	//constants for shader patcher buffers
	const unsigned int patcherBufferSize = 512 * 1024;
	const unsigned int patcherVertexUsseSize = 64 * 1024;
	const unsigned int patcherFragmentUsseSize = 64 * 1024;

	//allocate memory for buffers and USSE code
	gxmShaderPatcherBufferAddr = gpuAllocMap(patcherBufferSize, SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW,
		SCE_GXM_MEMORY_ATTRIB_RW, &gxmShaderPatcherBufferUID);
	unsigned int patcherVertexUsseOffset;
	gxmShaderPatcherVertexUsseAddr = gpuVertexUsseAllocMap(patcherVertexUsseSize, &patcherVertexUsseOffset, &gxmShaderPatcherVertexUsseUID);
	unsigned int patcherFragmentUsseOffset;
	gxmShaderPatcherFragmentUsseAddr = gpuFragmentUsseAllocMap(patcherFragmentUsseSize, &patcherFragmentUsseOffset, &gxmShaderPatcherFragmentUsseUID);

	sceClibPrintf("gxmShaderPatcherBufferAddr: 0x%08X\n", gxmShaderPatcherBufferAddr);
	sceClibPrintf("gxmShaderPatcherVertexUsseAddr: 0x%08X\n", gxmShaderPatcherVertexUsseAddr);
	sceClibPrintf("gxmShaderPatcherFragmentUsseAddr: 0x%08X\n", gxmShaderPatcherFragmentUsseAddr);

	//setup parameters
	SceGxmShaderPatcherParams patcherParams;
	sceClibMemset(&patcherParams, 0, sizeof(SceGxmShaderPatcherParams));
	patcherParams.userData = NULL;
	patcherParams.hostAllocCallback = &patcherHostAllocCallback;
	patcherParams.hostFreeCallback = &patcherHostFreeCallback;
	patcherParams.bufferAllocCallback = NULL;
	patcherParams.bufferFreeCallback = NULL;
	patcherParams.bufferMem = gxmShaderPatcherBufferAddr;
	patcherParams.bufferMemSize = patcherBufferSize;
	patcherParams.vertexUsseAllocCallback = NULL;
	patcherParams.vertexUsseFreeCallback = NULL;
	patcherParams.vertexUsseMem = gxmShaderPatcherVertexUsseAddr;
	patcherParams.vertexUsseMemSize = patcherVertexUsseSize;
	patcherParams.vertexUsseOffset = patcherVertexUsseOffset;
	patcherParams.fragmentUsseAllocCallback = NULL;
	patcherParams.fragmentUsseFreeCallback = NULL;
	patcherParams.fragmentUsseMem = gxmShaderPatcherFragmentUsseAddr;
	patcherParams.fragmentUsseMemSize = patcherFragmentUsseSize;
	patcherParams.fragmentUsseOffset = patcherFragmentUsseOffset;

	//print all the patcher params
	sceClibPrintf("patcherParams.userData: 0x%08X\n", patcherParams.userData);
	sceClibPrintf("patcherParams.hostAllocCallback: 0x%08X\n", patcherParams.hostAllocCallback);
	sceClibPrintf("patcherParams.hostFreeCallback: 0x%08X\n", patcherParams.hostFreeCallback);
	sceClibPrintf("patcherParams.bufferAllocCallback: 0x%08X\n", patcherParams.bufferAllocCallback);
	sceClibPrintf("patcherParams.bufferFreeCallback: 0x%08X\n", patcherParams.bufferFreeCallback);
	sceClibPrintf("patcherParams.bufferMem: 0x%08X\n", patcherParams.bufferMem);
	sceClibPrintf("patcherParams.bufferMemSize: 0x%08X\n", patcherParams.bufferMemSize);
	sceClibPrintf("patcherParams.vertexUsseAllocCallback: 0x%08X\n", patcherParams.vertexUsseAllocCallback);
	sceClibPrintf("patcherParams.vertexUsseFreeCallback: 0x%08X\n", patcherParams.vertexUsseFreeCallback);
	sceClibPrintf("patcherParams.vertexUsseMem: 0x%08X\n", patcherParams.vertexUsseMem);
	sceClibPrintf("patcherParams.vertexUsseMemSize: 0x%08X\n", patcherParams.vertexUsseMemSize);
	sceClibPrintf("patcherParams.vertexUsseOffset: 0x%08X\n", patcherParams.vertexUsseOffset);
	sceClibPrintf("patcherParams.fragmentUsseAllocCallback: 0x%08X\n", patcherParams.fragmentUsseAllocCallback);
	sceClibPrintf("patcherParams.fragmentUsseFreeCallback: 0x%08X\n", patcherParams.fragmentUsseFreeCallback);
	sceClibPrintf("patcherParams.fragmentUsseMem: 0x%08X\n", patcherParams.fragmentUsseMem);
	sceClibPrintf("patcherParams.fragmentUsseMemSize: 0x%08X\n", patcherParams.fragmentUsseMemSize);
	sceClibPrintf("patcherParams.fragmentUsseOffset: 0x%08X\n", patcherParams.fragmentUsseOffset);

	//Create the shader patcher instance
	err = sceGxmShaderPatcherCreate(&patcherParams, &gxmShaderPatcher);
	sceClibPrintf("sceGxmShaderPatcherCreate(): 0x%08X\n", err);
}

bool findGxmShaderAttributeByName(const SceGxmProgram* program, const char* name, const SceGxmProgramParameter** outParamId)
{
	*outParamId = sceGxmProgramFindParameterByName(program, name);
	if (*outParamId == NULL)
	{
		sceClibPrintf("sceGxmProgramFindParameterByName(%p, \"%s\") returned NULL \n", program, name);
	}
	else
	{
		//ensure its an attribute and not a uniform
		if (sceGxmProgramParameterGetCategory(*outParamId) == SCE_GXM_PARAMETER_CATEGORY_ATTRIBUTE)
		{
			sceClibPrintf("sceGxmProgramFindParameterByName(%p, \"%s\") returned attribute at address: %p\n", program, name, *outParamId);
			return true;
		}
		else
		{
			sceClibPrintf("WARNING: sceGxmProgramParameterGetCategory(%p) != SCE_GXM_PARAMETER_CATEGORY_ATTRIBUTE \n", *outParamId);
			return false;
		}
	}

	return false;
}

bool findGxmShaderUniformByName(const SceGxmProgram* program, const char* name, const SceGxmProgramParameter** outParamId)
{
	*outParamId = sceGxmProgramFindParameterByName(program, name);
	if (*outParamId == NULL)
	{
		sceClibPrintf("sceGxmProgramFindParameterByName(%p, \"%s\") returned NULL \n", program, name);
	}
	else
	{
		//ensure its an uniform and not an attribute
		if (sceGxmProgramParameterGetCategory(*outParamId) == SCE_GXM_PARAMETER_CATEGORY_UNIFORM)
		{
			sceClibPrintf("sceGxmProgramFindParameterByName(%p, \"%s\") returned uniform at address: %p\n", program, name, *outParamId);
			return true;
		}
		else
		{
			sceClibPrintf("WARNING: sceGxmProgramParameterGetCategory(%p) != SCE_GXM_PARAMETER_CATEGORY_UNIFORM \n", *outParamId);
			return false;
		}
	}

	return false;
}

void createShaders()
{
	int err = 0;

	//Use embeded programs (setup at the top of the file)
	sceClibPrintf("clearVertexProgramGxp: %p\n", (void*)gxmProgClearVertexGxp);
	sceClibPrintf("clearFragmentProgramGxp: %p\n", (void*)gxmProgClearFragmentGxp);

	/*
	*   Clear Screen Shader
	*/

	err = sceGxmShaderPatcherRegisterProgram(gxmShaderPatcher, gxmProgClearVertexGxp,
		&gxmClearVertexProgramID);
	sceClibPrintf("sceGxmShaderPatcherRegisterProgram(clearVertexProgramGxp): 0x%08X\n", err);
	err = sceGxmShaderPatcherRegisterProgram(gxmShaderPatcher, gxmProgClearFragmentGxp,
		&gxmClearFragmentProgramID);
	sceClibPrintf("sceGxmShaderPatcherRegisterProgram(clearFragmentProgramGxp): 0x%08X\n", err);

	sceClibPrintf("clearVShaderPatcherId: 0x%08X\n", gxmClearVertexProgramID);
	sceClibPrintf("clearFShaderPatcherId: 0x%08X\n", gxmClearFragmentProgramID);

	const SceGxmProgram* clearVertexProgram =
		sceGxmShaderPatcherGetProgramFromId(gxmClearVertexProgramID);
	const SceGxmProgram* clearFragmentProgram =
		sceGxmShaderPatcherGetProgramFromId(gxmClearFragmentProgramID);

	findGxmShaderAttributeByName(clearVertexProgram, "position", &gxmClearVertexProgram_positionParam);
	findGxmShaderUniformByName(clearFragmentProgram, "u_clearColor", &gxmClearFragmentProgram_u_clearColorParam);

	sceClibPrintf("clearPositionParam at address: %p\n", (void*)gxmClearVertexProgram_positionParam);
	sceClibPrintf("clearColorParam at address: %p\n", (void*)gxmClearFragmentProgram_u_clearColorParam);

	SceGxmVertexAttribute clear_vertex_attribute;
	SceGxmVertexStream clear_vertex_stream;
	clear_vertex_attribute.streamIndex = 0;
	clear_vertex_attribute.offset = 0;
	clear_vertex_attribute.format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
	clear_vertex_attribute.componentCount = 2;
	clear_vertex_attribute.regIndex = sceGxmProgramParameterGetResourceIndex(
		gxmClearVertexProgram_positionParam);
	clear_vertex_stream.stride = sizeof(struct ClearVertex);
	clear_vertex_stream.indexSource = SCE_GXM_INDEX_SOURCE_INDEX_16BIT;

	err = sceGxmShaderPatcherCreateVertexProgram(gxmShaderPatcher, gxmClearVertexProgramID,
		&clear_vertex_attribute, 1,
		&clear_vertex_stream, 1,
		&gxmClearVertexProgramPatched);
	sceClibPrintf("sceGxmShaderPatcherCreateVertexProgram(): 0x%08X\n", err);
	if (err == 0)
	{
		sceClibPrintf("clearVertexProgram created at address: %p\n", (void*)gxmClearVertexProgramPatched);
	}
	else
	{
		sceClibPrintf("clearVertexProgram creation failed\n");
	}

	err = sceGxmShaderPatcherCreateFragmentProgram(gxmShaderPatcher, gxmClearFragmentProgramID,
		SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4,
		gxmMultisampleMode,
		NULL,
		clearVertexProgram,
		&gxmClearFragmentProgramPatched);
	sceClibPrintf("sceGxmShaderPatcherCreateFragmentProgram(): 0x%08X\n", err);
	if (err == 0)
	{
		sceClibPrintf("clearFragmentProgram created at address: %p\n", (void*)gxmClearFragmentProgramPatched);
	}
	else
	{
		sceClibPrintf("clearFragmentProgram creation failed\n");
	}

	//SceUID clear_vertices_uid;
	clearVerticesData = (ClearVertex*)gpuAllocMap(4 * sizeof(struct ClearVertex),
		SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE, SCE_GXM_MEMORY_ATTRIB_READ, &clearVerticesUID);

	//SceUID clear_indices_uid;
	clearIndicesData = (unsigned short*)gpuAllocMap(4 * sizeof(unsigned short),
		SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE, SCE_GXM_MEMORY_ATTRIB_READ,
		&clearIndicesUID);

	clearVerticesData[0] = (ClearVertex){ -1.0f, -1.0f };
	clearVerticesData[1] = (ClearVertex){ 1.0f, -1.0f };
	clearVerticesData[2] = (ClearVertex){ -1.0f,  1.0f };
	clearVerticesData[3] = (ClearVertex){ 1.0f,  1.0f };

	clearIndicesData[0] = 0;
	clearIndicesData[1] = 1;
	clearIndicesData[2] = 2;
	clearIndicesData[3] = 3;

	/*
	*   Basic Shader
	*/

	err = sceGxmShaderPatcherRegisterProgram(gxmShaderPatcher, gxmProgBasicVertexGxp, &gxmBasicVertexProgramID);
	sceClibPrintf("sceGxmShaderPatcherRegisterProgram(basicVertexProgramGxp): 0x%08X\n", err);
	err = sceGxmShaderPatcherRegisterProgram(gxmShaderPatcher, gxmProgBasicFragmentGxp, &gxmBasicFragmentProgramID);
	sceClibPrintf("sceGxmShaderPatcherRegisterProgram(basicFragmentProgramGxp): 0x%08X\n", err);

	const SceGxmProgram* basicVertexProgram =
		sceGxmShaderPatcherGetProgramFromId(gxmBasicVertexProgramID);

	findGxmShaderAttributeByName(basicVertexProgram, "position", &gxmBasicVertexProgram_positionParam);
	findGxmShaderAttributeByName(basicVertexProgram, "color", &gxmBasicVertexProgram_colorParam);
	findGxmShaderUniformByName(basicVertexProgram, "u_modelMatrix", &gxmBasicVertexProgram_u_modelMatrixParam);
	findGxmShaderUniformByName(basicVertexProgram, "u_viewMatrix", &gxmBasicVertexProgram_u_viewMatrixParam);
	findGxmShaderUniformByName(basicVertexProgram, "u_projectionMatrix", &gxmBasicVertexProgram_u_projectionMatrixParam);

	sceClibPrintf("basic PositionParam at address: %p\n", (void*)gxmBasicVertexProgram_positionParam);
	sceClibPrintf("basic ColorParam at address: %p\n", (void*)gxmBasicVertexProgram_colorParam);

	SceGxmVertexAttribute basic_vertex_attributes[2];
	SceGxmVertexStream basic_vertex_stream;
	basic_vertex_attributes[0].streamIndex = 0;
	basic_vertex_attributes[0].offset = 0;
	basic_vertex_attributes[0].format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
	basic_vertex_attributes[0].componentCount = 3;
	basic_vertex_attributes[0].regIndex = sceGxmProgramParameterGetResourceIndex(
		gxmBasicVertexProgram_positionParam);
	basic_vertex_attributes[1].streamIndex = 0;
	basic_vertex_attributes[1].offset = offsetof(UnlitColorVertex, col); //sizeof(vector3f);
	basic_vertex_attributes[1].format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
	basic_vertex_attributes[1].componentCount = 4;
	basic_vertex_attributes[1].regIndex = sceGxmProgramParameterGetResourceIndex(
		gxmBasicVertexProgram_colorParam);
	basic_vertex_stream.stride = sizeof(struct UnlitColorVertex);
	basic_vertex_stream.indexSource = SCE_GXM_INDEX_SOURCE_INDEX_16BIT;

	err = sceGxmShaderPatcherCreateVertexProgram(gxmShaderPatcher,
		gxmBasicVertexProgramID, basic_vertex_attributes,
		2, &basic_vertex_stream, 1, &gxmBasicVertexProgramPatched);
	if (err == 0)
	{
		sceClibPrintf("basic VertexProgram created at address: %p\n", (void*)gxmBasicVertexProgramPatched);
	}
	else
	{
		sceClibPrintf("basic VertexProgram creation failed\n");
	}

	err = sceGxmShaderPatcherCreateFragmentProgram(gxmShaderPatcher,
		gxmBasicFragmentProgramID, SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4,
		gxmMultisampleMode, NULL, basicVertexProgram,
		&gxmBasicFragmentProgramPatched);
	if (err == 0)
	{
		sceClibPrintf("basic FragmentProgram created at address: %p\n", (void*)gxmBasicFragmentProgramPatched);
	}
	else
	{
		sceClibPrintf("basic FragmentProgram creation failed\n");
	}
}

void clearScreen()
{
	//start a new scene
	int ret = sceGxmBeginScene(gxmContext, 
		0, //flags
		gxmRenderTarget,
		NULL, //valid region
		NULL, //vertex sync object
		gxmSyncObjs[gxmBackBufferIndex], //fragment sync object
		&gxmColorSurfaces[gxmBackBufferIndex],
		&gxmDepthStencilSurface);
	
	//clear the screen
	{
		sceGxmSetVertexProgram(gxmContext, gxmClearVertexProgramPatched);
		sceGxmSetFragmentProgram(gxmContext, gxmClearFragmentProgramPatched);

		float clear[4] = { clearColor.r, clearColor.g, clearColor.b, clearColor.a };
		//float clearDepthValue = depthValue * 2 - 1;

		//void* vUniformBuffer = nullptr;
		void* fUniformBuffer = nullptr;

		sceGxmReserveFragmentDefaultUniformBuffer(gxmContext, &fUniformBuffer);
		sceGxmSetUniformDataF(fUniformBuffer, gxmClearFragmentProgram_u_clearColorParam, 0, sizeof(clear) / sizeof(float), clear);
		/*
		sceGxmSetFrontStencilFunc(gxmContext,
			SCE_GXM_STENCIL_FUNC_ALWAYS,
			SCE_GXM_STENCIL_OP_ZERO,
			SCE_GXM_STENCIL_OP_ZERO,
			SCE_GXM_STENCIL_OP_ZERO,
			0, 0xFF);
			*/

		sceGxmSetVertexStream(gxmContext, 0, clearVerticesData);
		sceGxmDraw(gxmContext, SCE_GXM_PRIMITIVE_TRIANGLE_STRIP, SCE_GXM_INDEX_FORMAT_U16, clearIndicesData, 4);
	}
}

void swapBuffers()
{
	//end scene
	sceGxmEndScene(gxmContext, NULL, NULL);
	//PA heartbeat to notify end of frame
	sceGxmPadHeartbeat(&gxmColorSurfaces[gxmBackBufferIndex], gxmSyncObjs[gxmBackBufferIndex]);

	/* -----------------------------------------------------------------
	Flip operation

	Now we have finished submitting rendering work for this frame it
	is time to submit a flip operation.  As part of specifying this
	flip operation we must provide the sync objects for both the old
	buffer and the new buffer.  This is to allow synchronization both
	ways: to not flip until rendering is complete, but also to ensure
	that future rendering to these buffers does not start until the
	flip operation is complete.

	The callback function will be called from an internal thread once
	queued GPU operations involving the sync objects is complete.
	Assuming we have not reached our maximum number of queued frames,
	this function returns immediately.

	Once we have queued our flip, we manually cycle through our back
	buffers before starting the next frame.
   ----------------------------------------------------------------- */

	// queue the display swap for this frame
	DisplayQueueCallbackData displayQueueCallbackData;
	displayQueueCallbackData.addr = gxmColorSurfacesAddr[gxmBackBufferIndex];
	sceGxmDisplayQueueAddEntry(gxmSyncObjs[gxmFrontBufferIndex], // OLD buffer
		gxmSyncObjs[gxmBackBufferIndex], // NEW buffer
		&displayQueueCallbackData);

	// update buffer indices
	gxmFrontBufferIndex = gxmBackBufferIndex;
	gxmBackBufferIndex = (gxmBackBufferIndex + 1) % DISPLAY_BUFFER_COUNT;
}

#define CUBE_SIZE 1.0f
#define CUBE_HALF_SIZE (CUBE_SIZE / 2.0f)

int main()
{
	initGxm(DISPLAY_WIDTH, DISPLAY_HEIGHT, SCE_GXM_MULTISAMPLE_4X);
	//memoryInit();
	initGxmContext();
	createRenderTarget();
	initDisplayColorSurfaces();
	initDepthStencilSurfaces();
	initShaderPatcher();
	createShaders();

	//initialize controller data
	//enable analog stick
	SceCtrlData ctrlData;
	sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);

	sceGxmSetTwoSidedEnable(gxmContext, SCE_GXM_TWO_SIDED_DISABLED);
	/*// Scissor Test shader register
	sceGxmShaderPatcherCreateMaskUpdateFragmentProgram(gxm_shader_patcher, &scissor_test_fragment_program);
	scissor_test_vertices = gpu_alloc_mapped(1 * sizeof(vector4f), VGL_MEM_RAM);*/

	static std::vector<Vector3f> cubeVertices =
	{
		{ -CUBE_HALF_SIZE, +CUBE_HALF_SIZE, +CUBE_HALF_SIZE },
		{ -CUBE_HALF_SIZE, -CUBE_HALF_SIZE, +CUBE_HALF_SIZE },
		{ +CUBE_HALF_SIZE, +CUBE_HALF_SIZE, +CUBE_HALF_SIZE },
		{ +CUBE_HALF_SIZE, -CUBE_HALF_SIZE, +CUBE_HALF_SIZE },
		{ +CUBE_HALF_SIZE, +CUBE_HALF_SIZE, -CUBE_HALF_SIZE },
		{ +CUBE_HALF_SIZE, -CUBE_HALF_SIZE, -CUBE_HALF_SIZE },
		{ -CUBE_HALF_SIZE, +CUBE_HALF_SIZE, -CUBE_HALF_SIZE },
		{ -CUBE_HALF_SIZE, -CUBE_HALF_SIZE, -CUBE_HALF_SIZE }
	};

	static std::vector<Color> cubeVerticesColor =
	{
		{ 1.0f, 0.0f, 0.0f, 1.0f },
		{ 0.0f, 1.0f, 0.0f, 1.0f },
		{ 0.0f, 0.0f, 1.0f, 1.0f },
		{ 1.0f, 1.0f, 0.0f, 1.0f },
		{ 0.0f, 1.0f, 1.0f, 1.0f },
		{ 1.0f, 0.0f, 1.0f, 1.0f },
		{ 0.5f, 0.5f, 0.5f, 1.0f },
		{ 0.25f, 0.75f, 0.25f, 1.0f }
	};

	static std::vector<unsigned short> cubeIndices =
	{
		// Front face
		0, 1, 2, 2, 1, 3,
		// Right face
		2, 3, 4, 4, 3, 5,
		// Back face
		4, 5, 6, 6, 5, 7,
		// Left face
		6, 7, 0, 0, 7, 1,
		// Top face
		6, 0, 4, 4, 0, 2,
		// Bottom face
		1, 7, 3, 3, 7, 5
	};

	//allocate memory for the vertex data
	sceClibPrintf("Allocating memory for the vertex data\n");
	SceUID vertexDataUID, indexDataUID;

	UnlitColorVertex* vertexData = (UnlitColorVertex*)gpuAllocMap(cubeVertices.size() * sizeof(UnlitColorVertex), 
		SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE, SCE_GXM_MEMORY_ATTRIB_READ, 
		&vertexDataUID);
	sceClibPrintf("Allocating memory for the index data\n");
	unsigned short* indexData = (unsigned short*)gpuAllocMap(cubeIndices.size() * sizeof(unsigned short), 
		SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE, SCE_GXM_MEMORY_ATTRIB_READ, 
		&indexDataUID);

	//interweave the position and colors into Vertex struct
	for (int i = 0; i < cubeVertices.size(); i++)
	{
		vertexData[i] = (struct UnlitColorVertex){ cubeVertices[i].x, cubeVertices[i].y, cubeVertices[i].z, cubeVerticesColor[i] };
	}
	//copy the index data
	memcpy(indexData, cubeIndices.data(), cubeIndices.size() * sizeof(unsigned short));
	//for (int i = 0; i < cubeIndices.size(); i++)
	//{
	//	indexData[i] = cubeIndices[i];
	//}

	//set model position and rotation
	Vector3f cubePosition = { 0.0f, 0.0f, -2.0f };
	Vector3f cubeRotation = { 0.0f, 0.0f, 0.0f };
	//create model matrix
	Matrix4x4 cubeModelMatrix = createTransformationMatrix(cubePosition, cubeRotation, Vector3f{ 1.0f, 1.0f, 1.0f });

	//create view matrix
	Vector3f cameraPosition = { 0.0f, 0.0f, 0.0f };
	Vector3f cameraRotation = { 0.0f, 0.0f, 0.0f };
	//params: position, rotation, fov, aspect ratio, near plane, far plane
	Camera camera = Camera(cameraPosition, cameraRotation, 45.0f, (float)DISPLAY_WIDTH / (float)DISPLAY_HEIGHT, 0.1f, 100.0f);

	bool running = true;
	while (running)
	{
		sceCtrlPeekBufferPositive(0, &ctrlData, 1);
		if (ctrlData.buttons & SCE_CTRL_START)
		{
			running = false;
		}
		if (ctrlData.buttons & SCE_CTRL_TRIANGLE)
		{
			running = false;
		}

		//update cube rotation
		cubeRotation.x += 0.36f; // * deltaTime TODO
		cubeRotation.y += 0.50f;
		cubeRotation.z += 0.01f;

		//cubeModelMatrix.rotate(Vector3f(cubeRotation.x, cubeRotation.y, cubeRotation.z));
		cubeModelMatrix = createTransformationMatrix(cubePosition, cubeRotation, Vector3f{ 1.0f, 1.0f, 1.0f });

		clearScreen();

		//render
		sceGxmSetVertexProgram(gxmContext, gxmBasicVertexProgramPatched);
		sceGxmSetFragmentProgram(gxmContext, gxmBasicFragmentProgramPatched);

		void* basicVertexBufferA;
		sceGxmReserveVertexDefaultUniformBuffer(gxmContext, &basicVertexBufferA);
		sceGxmSetUniformDataF(basicVertexBufferA, gxmBasicVertexProgram_u_modelMatrixParam, 0, 16, (float*)cubeModelMatrix.getData());
		
		void* basicVertexBufferB;
		sceGxmReserveVertexDefaultUniformBuffer(gxmContext, &basicVertexBufferB);
		sceGxmSetUniformDataF(basicVertexBufferB, gxmBasicVertexProgram_u_viewMatrixParam, 0, 16, (float*)camera.getViewMatrix().getData());
		
		void* basicVertexBufferC;
		sceGxmReserveVertexDefaultUniformBuffer(gxmContext, &basicVertexBufferC);
		sceGxmSetUniformDataF(basicVertexBufferC, gxmBasicVertexProgram_u_projectionMatrixParam, 0, 16, (float*)camera.getProjectionMatrix().getData());

		sceGxmSetVertexStream(gxmContext, 0, vertexData);
		sceGxmDraw(gxmContext, SCE_GXM_PRIMITIVE_TRIANGLES, SCE_GXM_INDEX_FORMAT_U16, indexData, 36);

		swapBuffers();
	}

	sceClibPrintf("Exiting...\n");

	sceGxmFinish(gxmContext);

	//cleanup
	//TO DO: free shader patcher programs and graphics data with UIDs

	//wait until display queue is finsihed before deallocating display buffers
	sceClibPrintf("...Waiting for GXM Display Queue to finish\n");
	sceGxmDisplayQueueFinish();
	sceClibPrintf("Calling sceGxmFinish()\n");
	sceGxmFinish(gxmContext);

	sceClibPrintf("Freeing clear shader vertex data\n");
	gpuFreeUnmap(clearVerticesUID);
	sceClibPrintf("Freeing clear shader index data\n");
	gpuFreeUnmap(clearIndicesUID);

	sceClibPrintf("Freeing other model vertex and index data\n");
	gpuFreeUnmap(vertexDataUID);
	gpuFreeUnmap(indexDataUID);

	//unregister programs and destroy shader patcher

	sceClibPrintf("Releasing clear shader programs\n");
	sceGxmShaderPatcherReleaseVertexProgram(gxmShaderPatcher, gxmClearVertexProgramPatched);
	sceGxmShaderPatcherReleaseFragmentProgram(gxmShaderPatcher, gxmClearFragmentProgramPatched);

	sceClibPrintf("Releasing basic shader programs\n");
	sceGxmShaderPatcherReleaseVertexProgram(gxmShaderPatcher, gxmBasicVertexProgramPatched);
	sceGxmShaderPatcherReleaseFragmentProgram(gxmShaderPatcher, gxmBasicFragmentProgramPatched);

	sceClibPrintf("Unregistering clear shader programs\n");
	sceGxmShaderPatcherUnregisterProgram(gxmShaderPatcher, gxmClearVertexProgramID);
	sceGxmShaderPatcherUnregisterProgram(gxmShaderPatcher, gxmClearFragmentProgramID);

	sceClibPrintf("Unregistering basic shader programs\n");
	sceGxmShaderPatcherUnregisterProgram(gxmShaderPatcher, gxmBasicVertexProgramID);
	sceGxmShaderPatcherUnregisterProgram(gxmShaderPatcher, gxmBasicFragmentProgramID);

	sceClibPrintf("Destroying GXM Shader Patcher\n");
	sceGxmShaderPatcherDestroy(gxmShaderPatcher);

	//free shader patcher memory 
	sceClibPrintf("Freeing shader patcher related memory\n");
	gpuFreeUnmap(gxmShaderPatcherBufferUID);
	gpuVertexUsseFreeUnmap(gxmShaderPatcherVertexUsseUID);
	gpuFragmentUsseFreeUnmap(gxmShaderPatcherFragmentUsseUID);

	//free surfaces and sync objects
	sceClibPrintf("Freeing surfaces and sync objects\n");
	gpuFreeUnmap(gxmDepthStencilSurfaceUID);
	for (int i = 0; i < DISPLAY_BUFFER_COUNT; i++)
	{
		gpuFreeUnmap(gxmColorSurfaceUIDs[i]);
		sceGxmSyncObjectDestroy(gxmSyncObjs[i]);
	}

	//destroy render target
	sceClibPrintf("Destroying GXM render target\n");
	sceGxmDestroyRenderTarget(gxmRenderTarget);

	//destroy context and free ring buffers and context memory
	sceClibPrintf("Freeing ring buffer related memory\n");
	gpuFreeUnmap(vdmRingBufferUID);
	gpuFreeUnmap(vertexRingBufferUID);
	gpuFreeUnmap(fragmentRingBufferUID);
	gpuFragmentUsseFreeUnmap(fragmentUsseRingBufferUID);

	sceClibPrintf("Destroying GXM Context\n");
	sceGxmDestroyContext(gxmContext);

	//terminate libgxm
	sceClibPrintf("Terminating libGXM\n");
	sceGxmTerminate();

	sceKernelExitProcess(0);
	return 0; //technically redundant
}