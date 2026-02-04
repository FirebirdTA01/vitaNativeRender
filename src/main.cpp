#include <psp2/gxm.h>
#include <psp2/kernel/clib.h>
#include <psp2/display.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/ctrl.h>
#include <psp2/rtc.h>
#include <vector>
#include <algorithm>
#include <stdlib.h> // For malloc, free
#include <string.h> // For memcpy
#include <random>

#include "commonUtils.h"
#include "memory.h"
#include "matrix.h"
#include "camera.h"
#include "light.h"
#include "terrain.h"
#include "texture.h"
#include "EMP_Logo.h"
#include "EMP_Logo_Alpha.h"
#include "terrainTextures.h"
#include "benchmark.h"

#define DISPLAY_WIDTH 960 // Default display width in pixels
#define DISPLAY_HEIGHT 544 // Default display height in pixels
#define DISPLAY_STRIDE 1024
#define DISPLAY_MAX_BUFFER_COUNT 3 // Maximum amount of display buffers to use //not used yet
#define DISPLAY_BUFFER_COUNT 2
#define MAX_PENDING_SWAPS (DISPLAY_BUFFER_COUNT - 1)

#define DISPLAY_COLOR_FORMAT SCE_GXM_COLOR_FORMAT_A8B8G8R8
#define DISPLAY_PIXEL_FORMAT SCE_DISPLAY_PIXELFORMAT_A8B8G8R8

static SceGxmMultisampleMode gxmMultisampleMode; //this is set later in initGxm
static bool wireFrame = false;
static BenchmarkState benchmarkState = {};

//not used yet
#define MAX_IDX_NUMBER 0xC000 // Maximum allowed number of indices per draw call for glDrawArrays

#define ANALOG_THRESHOLD 20 //not used yet

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
extern unsigned char _binary_textured_v_gxp_start;
extern unsigned char _binary_textured_f_gxp_start;
extern unsigned char _binary_texturedScreenLiteral_v_gxp_start;
extern unsigned char _binary_texturedScreenLiteral_f_gxp_start;
extern unsigned char _binary_texturedLit_v_gxp_start;
extern unsigned char _binary_texturedLit_f_gxp_start;
extern unsigned char _binary_texturedLitInstanced_v_gxp_start;
extern unsigned char _binary_terrain_v_gxp_start;
extern unsigned char _binary_terrain_f_gxp_start;

static const SceGxmProgram* const gxmProgClearVertexGxp = (SceGxmProgram*)&_binary_clear_v_gxp_start;
static const SceGxmProgram* const gxmProgClearFragmentGxp = (SceGxmProgram*)&_binary_clear_f_gxp_start;
static const SceGxmProgram* const gxmProgBasicVertexGxp = (SceGxmProgram*)&_binary_basic_v_gxp_start;
static const SceGxmProgram* const gxmProgBasicFragmentGxp = (SceGxmProgram*)&_binary_basic_f_gxp_start;
static const SceGxmProgram* const gxmProgTexturedVertexGxp = (SceGxmProgram*)&_binary_textured_v_gxp_start;
static const SceGxmProgram* const gxmProgTexturedFragmentGxp = (SceGxmProgram*)&_binary_textured_f_gxp_start;
static const SceGxmProgram* const gxmProgTexturedScreenLiteralVertexGxp = (SceGxmProgram*)&_binary_texturedScreenLiteral_v_gxp_start;
static const SceGxmProgram* const gxmProgTexturedScreenLiteralFragmentGxp = (SceGxmProgram*)&_binary_texturedScreenLiteral_f_gxp_start;
static const SceGxmProgram* const gxmProgTexturedLitVertexGxp = (SceGxmProgram*)&_binary_texturedLit_v_gxp_start;
static const SceGxmProgram* const gxmProgTexturedLitFragmentGxp = (SceGxmProgram*)&_binary_texturedLit_f_gxp_start;
static const SceGxmProgram* const gxmProgTexturedLitInstancedVertexGxp = (SceGxmProgram*)&_binary_texturedLitInstanced_v_gxp_start;
static const SceGxmProgram* const gxmProgTerrainVertexGxp = (SceGxmProgram*)&_binary_terrain_v_gxp_start;
static const SceGxmProgram* const gxmProgTerrainFragmentGxp = (SceGxmProgram*)&_binary_terrain_f_gxp_start;

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

static SceGxmShaderPatcherId gxmTexturedVertexProgramID;
static SceGxmShaderPatcherId gxmTexturedFragmentProgramID;
static const SceGxmProgramParameter* gxmTexturedVertexProgram_positionParam;
static const SceGxmProgramParameter* gxmTexturedVertexProgram_texCoordParam;
static const SceGxmProgramParameter* gxmTexturedVertexProgram_u_modelMatrixParam;
static const SceGxmProgramParameter* gxmTexturedVertexProgram_u_viewMatrixParam;
static const SceGxmProgramParameter* gxmTexturedVertexProgram_u_projectionMatrixParam;
static const SceGxmProgramParameter* gxmTexturedFragmentProgram_u_textureParam;
static SceGxmVertexProgram* gxmTexturedVertexProgramPatched;
static SceGxmFragmentProgram* gxmTexturedFragmentProgramPatched;

static SceGxmShaderPatcherId gxmTexturedScreenLiteralVertexProgramID;
static SceGxmShaderPatcherId gxmTexturedScreenLiteralFragmentProgramID;
static const SceGxmProgramParameter* gxmTexturedScreenLiteralVertexProgram_positionParam;
static const SceGxmProgramParameter* gxmTexturedScreenLiteralVertexProgram_texCoordParam;
static const SceGxmProgramParameter* gxmTexturedScreenLiteralVertexProgram_u_alphaParam;
static const SceGxmProgramParameter* gxmTexturedScreenLiteralVertexProgram_u_transformParam;
static const SceGxmProgramParameter* gxmTexturedScreenLiteralFragmentProgram_u_textureParam;
static SceGxmVertexProgram* gxmTexturedScreenLiteralVertexProgramPatched;
static SceGxmFragmentProgram* gxmTexturedScreenLiteralFragmentProgramPatched;

static SceGxmShaderPatcherId gxmTexturedLitVertexProgramID;
static SceGxmShaderPatcherId gxmTexturedLitFragmentProgramID;
static const SceGxmProgramParameter* gxmTexturedLitVertexProgram_positionParam;
static const SceGxmProgramParameter* gxmTexturedLitVertexProgram_texCoordParam;
static const SceGxmProgramParameter* gxmTexturedLitVertexProgram_normalParam;
static const SceGxmProgramParameter* gxmTexturedLitVertexProgram_u_modelMatrixParam;
static const SceGxmProgramParameter* gxmTexturedLitVertexProgram_u_viewMatrixParam;
static const SceGxmProgramParameter* gxmTexturedLitVertexProgram_u_projectionMatrixParam;
static const SceGxmProgramParameter* gxmTexturedLitFragmentProgram_u_lightCountParam;
static const SceGxmProgramParameter* gxmTexturedLitFragmentProgram_u_lightPositionsParam;
static const SceGxmProgramParameter* gxmTexturedLitFragmentProgram_u_lightColorsParam;
static const SceGxmProgramParameter* gxmTexturedLitFragmentProgram_u_lightPowersParam;
static const SceGxmProgramParameter* gxmTexturedLitFragmentProgram_u_lightRadiiParam;
static SceGxmVertexProgram* gxmTexturedLitVertexProgramPatched;
static SceGxmFragmentProgram* gxmTexturedLitFragmentProgramPatched;

static SceGxmShaderPatcherId gxmTexturedLitInstancedVertexProgramID;
static const SceGxmProgramParameter* gxmTexturedLitInstancedVertexProgram_positionParam;
static const SceGxmProgramParameter* gxmTexturedLitInstancedVertexProgram_texCoordParam;
static const SceGxmProgramParameter* gxmTexturedLitInstancedVertexProgram_normalParam;
static const SceGxmProgramParameter* gxmTexturedLitInstancedVertexProgram_i_m0Param;
static const SceGxmProgramParameter* gxmTexturedLitInstancedVertexProgram_i_m1Param;
static const SceGxmProgramParameter* gxmTexturedLitInstancedVertexProgram_i_m2Param;
static const SceGxmProgramParameter* gxmTexturedLitInstancedVertexProgram_i_m3Param;
static const SceGxmProgramParameter* gxmTexturedLitInstancedVertexProgram_u_viewMatrixParam;
static const SceGxmProgramParameter* gxmTexturedLitInstancedVertexProgram_u_projectionMatrixParam;
static SceGxmVertexProgram* gxmTexturedLitInstancedVertexProgramPatched;

static SceGxmShaderPatcherId gxmTerrainVertexProgramID;
static SceGxmShaderPatcherId gxmTerrainFragmentProgramID;
static const SceGxmProgramParameter* gxmTerrainVertexProgram_positionParam;
static const SceGxmProgramParameter* gxmTerrainVertexProgram_texCoordParam;
static const SceGxmProgramParameter* gxmTerrainVertexProgram_normalParam;
static const SceGxmProgramParameter* gxmTerrainVertexProgram_tangentParam;
static const SceGxmProgramParameter* gxmTerrainVertexProgram_bitangentParam;
static const SceGxmProgramParameter* gxmTerrainVertexProgram_u_viewMatrixParam;
static const SceGxmProgramParameter* gxmTerrainVertexProgram_u_projectionMatrixParam;
static const SceGxmProgramParameter* gxmTerrainVertexProgram_u_cameraPositionParam;
static const SceGxmProgramParameter* gxmTerrainVertexProgram_u_modelMatrixParam;
static const SceGxmProgramParameter* gxmTerrainFragmentProgram_u_lightCountParam;
static const SceGxmProgramParameter* gxmTerrainFragmentProgram_u_lightPositionsParam;
static const SceGxmProgramParameter* gxmTerrainFragmentProgram_u_lightColorsParam;
static const SceGxmProgramParameter* gxmTerrainFragmentProgram_u_lightPowersParam;
static const SceGxmProgramParameter* gxmTerrainFragmentProgram_u_lightRadiiParam;
static const SceGxmProgramParameter* gxmTerrainFragmentProgram_u_F0Param;
static SceGxmVertexProgram* gxmTerrainVertexProgramPatched;
static SceGxmFragmentProgram* gxmTerrainFragmentProgramPatched;

struct InstanceData
{
	float modelMatrix[16];
};

//Used by Lit Textured Shader
struct PerFrameVertexUniforms
{
	float viewMatrix[16];
	float projectionMatrix[16];
};

struct PerFrameFragmentUniforms
{
	uint32_t lightCount;			// 4 bytes
	uint8_t padding[4];				// 4 bytes
	float lightPositions[8][4];		// 128 bytes (8 vec4s)
	float lightColors[8][4];		// 128 bytes (8 vec4s)
	float lightPowers[8];			// 32 bytes (8 floats)
	float lightRadii[8];			// 32 bytes (8 floats)
	float cameraPosition[3];		// 12 bytes (vec3)
};

//Used by Terrain Shader
struct PerFrameTerrainVertexUniforms
{
	float viewMatrix[16];
	float projectionMatrix[16];
	float cameraPosition[3];
};
static_assert(sizeof(PerFrameTerrainVertexUniforms) == 140, "PerFrameTerrainVertexUniforms buffer size mismatch");

struct PerFrameTerrainFragmentUniforms
{
	uint32_t lightCount;			// 4 bytes
	uint8_t padding[4];				// 4 bytes
	float lightPositions[8][4];		// 128 bytes (8 vec4s)
	float lightColors[8][4];		// 128 bytes (8 vec4s)
	float lightPowers[8];			// 32 bytes (8 floats)
	float lightRadii[8];			// 32 bytes (8 floats)
};
static_assert(sizeof(PerFrameTerrainFragmentUniforms) == 328, "PerFrameTerrainFragmentUniforms buffer size mismatch");

SceUID perFrameVertexUniformBufferUID;
SceUID perFrameFragmentUniformBufferUID;
SceUID perFrameTerrainVertexUniformBufferUID;
SceUID perFrameTerrainFragmentUniformBufferUID;
SceUID instanceDataBufferUID;
PerFrameVertexUniforms* perFrameVertexUniformBuffer;
PerFrameFragmentUniforms* perFrameFragmentUniformBuffer;
PerFrameTerrainVertexUniforms* perFrameTerrainVertexUniformBuffer;
PerFrameTerrainFragmentUniforms* perFrameTerrainFragmentUniformBuffer;
InstanceData* instanceDataBuffer;

// Holds terrain chunk GPU data for each LOD
struct ChunkGPUData
{
	SceUID vertexUIDs[TerrainChunk::LOD_COUNT];
	SceUID indexUIDs[TerrainChunk::LOD_COUNT];
	TerrainPBRVertex* vertexPtrs[TerrainChunk::LOD_COUNT];
	uint16_t* indexPtrs[TerrainChunk::LOD_COUNT];
};

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

//used for the reused/persistent uniform buffers by the lit textured shader
void initializeUniformBuffers()
{
	perFrameVertexUniformBuffer = (PerFrameVertexUniforms*)gpuAllocMap(
		sizeof(PerFrameVertexUniforms), 
		SCE_KERNEL_MEMBLOCK_TYPE_USER_RW, //SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW (test which is faster)
		SCE_GXM_MEMORY_ATTRIB_READ, 
		&perFrameVertexUniformBufferUID);

	perFrameFragmentUniformBuffer = (PerFrameFragmentUniforms*)gpuAllocMap(
		sizeof(PerFrameFragmentUniforms),
		SCE_KERNEL_MEMBLOCK_TYPE_USER_RW, //SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW
		SCE_GXM_MEMORY_ATTRIB_READ,
		&perFrameFragmentUniformBufferUID);

	perFrameTerrainVertexUniformBuffer = (PerFrameTerrainVertexUniforms*)gpuAllocMap(
		sizeof(PerFrameTerrainVertexUniforms),
		SCE_KERNEL_MEMBLOCK_TYPE_USER_RW, //SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW (test which is faster)
		SCE_GXM_MEMORY_ATTRIB_READ,
		&perFrameTerrainVertexUniformBufferUID);

	perFrameTerrainFragmentUniformBuffer = (PerFrameTerrainFragmentUniforms*)gpuAllocMap(
		sizeof(PerFrameTerrainFragmentUniforms),
		SCE_KERNEL_MEMBLOCK_TYPE_USER_RW, //SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW
		SCE_GXM_MEMORY_ATTRIB_READ,
		&perFrameTerrainFragmentUniformBufferUID);
}

void freeUniformBuffers()
{
	gpuFreeUnmap(perFrameVertexUniformBufferUID);
	gpuFreeUnmap(perFrameFragmentUniformBufferUID);
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
			unsigned int containerIndex = sceGxmProgramParameterGetContainerIndex(*outParamId);
			sceClibPrintf("Uniform at address: %p exists in container index %u\n", *outParamId, containerIndex);
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

	/*
	*   Texture Shader
	*/

	err = sceGxmShaderPatcherRegisterProgram(gxmShaderPatcher, gxmProgTexturedVertexGxp, &gxmTexturedVertexProgramID);
	sceClibPrintf("sceGxmShaderPatcherRegisterProgram(texturedVertexProgramGxp): 0x%08X\n", err);
	err = sceGxmShaderPatcherRegisterProgram(gxmShaderPatcher, gxmProgTexturedFragmentGxp, &gxmTexturedFragmentProgramID);
	sceClibPrintf("sceGxmShaderPatcherRegisterProgram(texturedFragmentProgramGxp): 0x%08X\n", err);

	const SceGxmProgram* texturedVertexProgram =
		sceGxmShaderPatcherGetProgramFromId(gxmTexturedVertexProgramID);

	findGxmShaderAttributeByName(texturedVertexProgram, "position", &gxmTexturedVertexProgram_positionParam);
	findGxmShaderAttributeByName(texturedVertexProgram, "texCoord", &gxmTexturedVertexProgram_texCoordParam);
	findGxmShaderUniformByName(texturedVertexProgram, "u_modelMatrix", &gxmTexturedVertexProgram_u_modelMatrixParam);
	findGxmShaderUniformByName(texturedVertexProgram, "u_viewMatrix", &gxmTexturedVertexProgram_u_viewMatrixParam);
	findGxmShaderUniformByName(texturedVertexProgram, "u_projectionMatrix", &gxmTexturedVertexProgram_u_projectionMatrixParam);

	sceClibPrintf("textured PositionParam at address: %p\n", (void*)gxmTexturedVertexProgram_positionParam);
	sceClibPrintf("textured TexCoordParam at address: %p\n", (void*)gxmTexturedVertexProgram_texCoordParam);

	// Blend info
	SceGxmBlendInfo blendInfo;
	//sceClibMemset(&blendInfo, 0, sizeof(SceGxmBlendInfo));
	blendInfo.colorMask = SCE_GXM_COLOR_MASK_ALL; // Write to all color channels
	blendInfo.colorFunc = SCE_GXM_BLEND_FUNC_ADD; // Add the source and destination colors
	blendInfo.alphaFunc = SCE_GXM_BLEND_FUNC_ADD; // Add the source and destination alphas
	blendInfo.colorSrc = SCE_GXM_BLEND_FACTOR_SRC_ALPHA; // Multiply the source color by the source alpha
	blendInfo.colorDst = SCE_GXM_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA; // Multiply the destination color by 1-src alpha
	blendInfo.alphaSrc = SCE_GXM_BLEND_FACTOR_SRC_ALPHA; // For alpha, use the source alpha
	blendInfo.alphaDst = SCE_GXM_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA; // For destination, use 1-src alpha

	SceGxmVertexAttribute textured_vertex_attributes[2];
	SceGxmVertexStream textured_vertex_stream;
	textured_vertex_attributes[0].streamIndex = 0;
	textured_vertex_attributes[0].offset = 0;
	textured_vertex_attributes[0].format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
	textured_vertex_attributes[0].componentCount = 3;
	textured_vertex_attributes[0].regIndex = sceGxmProgramParameterGetResourceIndex(
		gxmTexturedVertexProgram_positionParam);
	textured_vertex_attributes[1].streamIndex = 0;
	textured_vertex_attributes[1].offset = offsetof(UnlitTexturedVertex, uv); //sizeof(vector3f);
	textured_vertex_attributes[1].format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
	textured_vertex_attributes[1].componentCount = 2;
	textured_vertex_attributes[1].regIndex = sceGxmProgramParameterGetResourceIndex(
		gxmTexturedVertexProgram_texCoordParam);
	textured_vertex_stream.stride = sizeof(struct UnlitTexturedVertex);
	textured_vertex_stream.indexSource = SCE_GXM_INDEX_SOURCE_INDEX_16BIT;

	err = sceGxmShaderPatcherCreateVertexProgram(gxmShaderPatcher,
		gxmTexturedVertexProgramID, textured_vertex_attributes,
		2, &textured_vertex_stream, 1, &gxmTexturedVertexProgramPatched);
	if (err == 0)
	{
		sceClibPrintf("textured VertexProgram created at address: %p\n", (void*)gxmTexturedVertexProgramPatched);
	}
	else
	{
		sceClibPrintf("textured VertexProgram creation failed\n");
	}

	err = sceGxmShaderPatcherCreateFragmentProgram(gxmShaderPatcher,
		gxmTexturedFragmentProgramID, SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4,
		gxmMultisampleMode, &blendInfo, texturedVertexProgram,
		&gxmTexturedFragmentProgramPatched);
	if (err == 0)
	{
		sceClibPrintf("textured FragmentProgram created at address: %p\n", (void*)gxmTexturedFragmentProgramPatched);
	}
	else
	{
		sceClibPrintf("textured FragmentProgram creation failed\n");
	}

	/*
	*   Textured Screen Literal Shader
	*/

	err = sceGxmShaderPatcherRegisterProgram(gxmShaderPatcher, gxmProgTexturedScreenLiteralVertexGxp, &gxmTexturedScreenLiteralVertexProgramID);
	sceClibPrintf("sceGxmShaderPatcherRegisterProgram(texturedScreenLiteralVertexProgramGxp): 0x%08X\n", err);
	err = sceGxmShaderPatcherRegisterProgram(gxmShaderPatcher, gxmProgTexturedScreenLiteralFragmentGxp, &gxmTexturedScreenLiteralFragmentProgramID);
	sceClibPrintf("sceGxmShaderPatcherRegisterProgram(texturedScreenLiteralFragmentProgramGxp): 0x%08X\n", err);

	const SceGxmProgram* texturedScreenLiteralVertexProgram =
		sceGxmShaderPatcherGetProgramFromId(gxmTexturedScreenLiteralVertexProgramID);
	const SceGxmProgram* texturedScreenLiteralFragmentProgram =
		sceGxmShaderPatcherGetProgramFromId(gxmTexturedScreenLiteralFragmentProgramID);

	findGxmShaderAttributeByName(texturedScreenLiteralVertexProgram, "position", &gxmTexturedScreenLiteralVertexProgram_positionParam);
	findGxmShaderAttributeByName(texturedScreenLiteralVertexProgram, "texCoord", &gxmTexturedScreenLiteralVertexProgram_texCoordParam);
	findGxmShaderUniformByName(texturedScreenLiteralVertexProgram, "u_alpha", &gxmTexturedScreenLiteralVertexProgram_u_alphaParam);
	findGxmShaderUniformByName(texturedScreenLiteralVertexProgram, "u_transform", &gxmTexturedScreenLiteralVertexProgram_u_transformParam);


	sceClibPrintf("textured PositionParam at address: %p\n", (void*)gxmTexturedVertexProgram_positionParam);
	sceClibPrintf("textured TexCoordParam at address: %p\n", (void*)gxmTexturedVertexProgram_texCoordParam);
	sceClibPrintf("textured AlphaParam at address: %p\n", (void*)gxmTexturedScreenLiteralVertexProgram_u_alphaParam);
	sceClibPrintf("textured TransformParam at address: %p\n", (void*)gxmTexturedScreenLiteralVertexProgram_u_transformParam);

	SceGxmVertexAttribute texturedScreenLiteral_vertex_attributes[2];
	SceGxmVertexStream texturedScreenLiteral_vertex_stream;
	texturedScreenLiteral_vertex_attributes[0].streamIndex = 0;
	texturedScreenLiteral_vertex_attributes[0].offset = 0;
	texturedScreenLiteral_vertex_attributes[0].format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
	texturedScreenLiteral_vertex_attributes[0].componentCount = 3;
	texturedScreenLiteral_vertex_attributes[0].regIndex = sceGxmProgramParameterGetResourceIndex(
		gxmTexturedScreenLiteralVertexProgram_positionParam);
	texturedScreenLiteral_vertex_attributes[1].streamIndex = 0;
	texturedScreenLiteral_vertex_attributes[1].offset = offsetof(UnlitTexturedVertex, uv); //sizeof(vector3f);
	texturedScreenLiteral_vertex_attributes[1].format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
	texturedScreenLiteral_vertex_attributes[1].componentCount = 2;
	texturedScreenLiteral_vertex_attributes[1].regIndex = sceGxmProgramParameterGetResourceIndex(
		gxmTexturedScreenLiteralVertexProgram_texCoordParam);
	texturedScreenLiteral_vertex_stream.stride = sizeof(struct UnlitTexturedVertex);
	texturedScreenLiteral_vertex_stream.indexSource = SCE_GXM_INDEX_SOURCE_INDEX_16BIT;

	sceClibPrintf("Creating texturedScreenLiteral VertexProgram\n");
	err = sceGxmShaderPatcherCreateVertexProgram(gxmShaderPatcher,
		gxmTexturedScreenLiteralVertexProgramID, texturedScreenLiteral_vertex_attributes,
		2, &texturedScreenLiteral_vertex_stream, 1, &gxmTexturedScreenLiteralVertexProgramPatched);
	if (err == 0)
	{
		sceClibPrintf("texturedScreenLiteral VertexProgram created at address: %p\n", (void*)gxmTexturedScreenLiteralVertexProgramPatched);
	}
	else
	{
		sceClibPrintf("texturedScreenLiteral VertexProgram creation failed\n");
	}

	sceClibPrintf("Creating texturedScreenLiteral FragmentProgram\n");
	err = sceGxmShaderPatcherCreateFragmentProgram(gxmShaderPatcher,
		gxmTexturedScreenLiteralFragmentProgramID, SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4,
		gxmMultisampleMode, &blendInfo, texturedScreenLiteralVertexProgram,
		&gxmTexturedScreenLiteralFragmentProgramPatched);
	if (err == 0)
	{
		sceClibPrintf("texturedScreenLiteral FragmentProgram created at address: %p\n", (void*)gxmTexturedScreenLiteralFragmentProgramPatched);
	}
	else
	{
		sceClibPrintf("texturedScreenLiteral FragmentProgram creation failed\n");
	}

	/*
	*	TexturedLit Shader
	*/

	err = sceGxmShaderPatcherRegisterProgram(gxmShaderPatcher, gxmProgTexturedLitVertexGxp, &gxmTexturedLitVertexProgramID);
	sceClibPrintf("sceGxmShaderPatcherRegisterProgram(texturedLitVertexProgramGxp): 0x%08X\n", err);
	err = sceGxmShaderPatcherRegisterProgram(gxmShaderPatcher, gxmProgTexturedLitFragmentGxp, &gxmTexturedLitFragmentProgramID);
	sceClibPrintf("sceGxmShaderPatcherRegisterProgram(texturedLitFragmentProgramGxp): 0x%08X\n", err);

	const SceGxmProgram* texturedLitVertexProgram =
		sceGxmShaderPatcherGetProgramFromId(gxmTexturedLitVertexProgramID);
	const SceGxmProgram* texturedLitFragmentProgram =
		sceGxmShaderPatcherGetProgramFromId(gxmTexturedLitFragmentProgramID);

	findGxmShaderAttributeByName(texturedLitVertexProgram, "position", &gxmTexturedLitVertexProgram_positionParam);
	findGxmShaderAttributeByName(texturedLitVertexProgram, "texCoord", &gxmTexturedLitVertexProgram_texCoordParam);
	findGxmShaderAttributeByName(texturedLitVertexProgram, "normal", &gxmTexturedLitVertexProgram_normalParam);
	findGxmShaderUniformByName(texturedLitVertexProgram, "u_modelMatrix", &gxmTexturedLitVertexProgram_u_modelMatrixParam);
	findGxmShaderUniformByName(texturedLitVertexProgram, "u_perVFrame.u_viewMatrix", &gxmTexturedLitVertexProgram_u_viewMatrixParam);
	findGxmShaderUniformByName(texturedLitVertexProgram, "u_perVFrame.u_projectionMatrix", &gxmTexturedLitVertexProgram_u_projectionMatrixParam);
	findGxmShaderUniformByName(texturedLitFragmentProgram, "u_perPFrame.u_lightCount", &gxmTexturedLitFragmentProgram_u_lightCountParam);
	findGxmShaderUniformByName(texturedLitFragmentProgram, "u_perPFrame.u_lightColors", &gxmTexturedLitFragmentProgram_u_lightColorsParam);
	findGxmShaderUniformByName(texturedLitFragmentProgram, "u_perPFrame.u_lightPowers", &gxmTexturedLitFragmentProgram_u_lightPowersParam);
	findGxmShaderUniformByName(texturedLitFragmentProgram, "u_perPFrame.u_lightRadii", &gxmTexturedLitFragmentProgram_u_lightRadiiParam);
	findGxmShaderUniformByName(texturedLitFragmentProgram, "u_perPFrame.u_lightPositions", &gxmTexturedLitFragmentProgram_u_lightPositionsParam);

	sceClibPrintf("texturedLit PositionParam at address: %p\n", (void*)gxmTexturedLitVertexProgram_positionParam);
	sceClibPrintf("texturedLit TexCoordParam at address: %p\n", (void*)gxmTexturedLitVertexProgram_texCoordParam);
	sceClibPrintf("texturedLit NormalParam at address: %p\n", (void*)gxmTexturedLitVertexProgram_normalParam);
	sceClibPrintf("texturedLit ModelMatrixParam at address: %p\n", (void*)gxmTexturedLitVertexProgram_u_modelMatrixParam);
	sceClibPrintf("texturedLit ViewMatrixParam at address: %p\n", (void*)gxmTexturedLitVertexProgram_u_viewMatrixParam);
	sceClibPrintf("texturedLit ProjectionMatrixParam at address: %p\n", (void*)gxmTexturedLitVertexProgram_u_projectionMatrixParam);
	sceClibPrintf("texturedLit LightCountParam at address: %p\n", (void*)gxmTexturedLitFragmentProgram_u_lightCountParam);
	sceClibPrintf("texturedLit LightPositionsParam at address: %p\n", (void*)gxmTexturedLitFragmentProgram_u_lightPositionsParam);
	sceClibPrintf("texturedLit LightColorsParam at address: %p\n", (void*)gxmTexturedLitFragmentProgram_u_lightColorsParam);
	sceClibPrintf("texturedLit LightPowersParam at address: %p\n", (void*)gxmTexturedLitFragmentProgram_u_lightPowersParam);
	sceClibPrintf("texturedLit LightRadiiParam at address: %p\n", (void*)gxmTexturedLitFragmentProgram_u_lightRadiiParam);

	SceGxmVertexAttribute texturedLit_vertex_attributes[3];
	SceGxmVertexStream texturedLit_vertex_stream;
	texturedLit_vertex_attributes[0].streamIndex = 0;
	texturedLit_vertex_attributes[0].offset = 0;
	texturedLit_vertex_attributes[0].format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
	texturedLit_vertex_attributes[0].componentCount = 3;
	texturedLit_vertex_attributes[0].regIndex = sceGxmProgramParameterGetResourceIndex(
		gxmTexturedLitVertexProgram_positionParam);
	texturedLit_vertex_attributes[1].streamIndex = 0;
	texturedLit_vertex_attributes[1].offset = offsetof(TexturedVertex, uv); //sizeof(vector3f);
	texturedLit_vertex_attributes[1].format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
	texturedLit_vertex_attributes[1].componentCount = 2;
	texturedLit_vertex_attributes[1].regIndex = sceGxmProgramParameterGetResourceIndex(
		gxmTexturedLitVertexProgram_texCoordParam);
	texturedLit_vertex_attributes[2].streamIndex = 0;
	texturedLit_vertex_attributes[2].offset = offsetof(TexturedVertex, normal); //sizeof(vector3f) + sizeof(vector2f);
	texturedLit_vertex_attributes[2].format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
	texturedLit_vertex_attributes[2].componentCount = 3;
	texturedLit_vertex_attributes[2].regIndex = sceGxmProgramParameterGetResourceIndex(
		gxmTexturedLitVertexProgram_normalParam);
	texturedLit_vertex_stream.stride = sizeof(struct TexturedVertex);
	texturedLit_vertex_stream.indexSource = SCE_GXM_INDEX_SOURCE_INDEX_16BIT;

	err = sceGxmShaderPatcherCreateVertexProgram(gxmShaderPatcher,
		gxmTexturedLitVertexProgramID, texturedLit_vertex_attributes,
		3, &texturedLit_vertex_stream, 1, &gxmTexturedLitVertexProgramPatched);
	if (err == 0)
	{
		sceClibPrintf("texturedLit VertexProgram created at address: %p\n", (void*)gxmTexturedLitVertexProgramPatched);
	}
	else
	{
		sceClibPrintf("texturedLit VertexProgram creation failed\n");
	}

	err = sceGxmShaderPatcherCreateFragmentProgram(gxmShaderPatcher,
		gxmTexturedLitFragmentProgramID, SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4,
		gxmMultisampleMode, NULL, texturedLitVertexProgram,
		&gxmTexturedLitFragmentProgramPatched);
	if (err == 0)
	{
		sceClibPrintf("texturedLit FragmentProgram created at address: %p\n", (void*)gxmTexturedLitFragmentProgramPatched);
	}
	else
	{
		sceClibPrintf("texturedLit FragmentProgram creation failed\n");
	}

	/*
	*	TexturedLit Shader (Instanced)
	*/

	err = sceGxmShaderPatcherRegisterProgram(gxmShaderPatcher, gxmProgTexturedLitInstancedVertexGxp, &gxmTexturedLitInstancedVertexProgramID);
	sceClibPrintf("sceGxmShaderPatcherRegisterProgram(texturedLitInstancedVertexProgramGxp): 0x%08X\n", err);

	const SceGxmProgram* texturedLitInstancedVertexProgram =
		sceGxmShaderPatcherGetProgramFromId(gxmTexturedLitInstancedVertexProgramID);

	findGxmShaderAttributeByName(texturedLitInstancedVertexProgram, "position", &gxmTexturedLitInstancedVertexProgram_positionParam);
	findGxmShaderAttributeByName(texturedLitInstancedVertexProgram, "texCoord", &gxmTexturedLitInstancedVertexProgram_texCoordParam);
	findGxmShaderAttributeByName(texturedLitInstancedVertexProgram, "normal", &gxmTexturedLitInstancedVertexProgram_normalParam);
	findGxmShaderAttributeByName(texturedLitInstancedVertexProgram, "i_m0", &gxmTexturedLitInstancedVertexProgram_i_m0Param);
	findGxmShaderAttributeByName(texturedLitInstancedVertexProgram, "i_m1", &gxmTexturedLitInstancedVertexProgram_i_m1Param);
	findGxmShaderAttributeByName(texturedLitInstancedVertexProgram, "i_m2", &gxmTexturedLitInstancedVertexProgram_i_m2Param);
	findGxmShaderAttributeByName(texturedLitInstancedVertexProgram, "i_m3", &gxmTexturedLitInstancedVertexProgram_i_m3Param);
	findGxmShaderAttributeByName(texturedLitInstancedVertexProgram, "u_perVFrame.u_viewMatrix", &gxmTexturedLitInstancedVertexProgram_u_viewMatrixParam);
	findGxmShaderAttributeByName(texturedLitInstancedVertexProgram, "u_perVFrame.u_projectionMatrix", &gxmTexturedLitInstancedVertexProgram_u_projectionMatrixParam);

	sceClibPrintf("texturedLitInstanced PositionParam at address: %p\n", (void*)gxmTexturedLitInstancedVertexProgram_positionParam);
	sceClibPrintf("texturedLitInstanced TexCoordParam at address: %p\n", (void*)gxmTexturedLitInstancedVertexProgram_texCoordParam);
	sceClibPrintf("texturedLitInstanced NormalParam at address: %p\n", (void*)gxmTexturedLitInstancedVertexProgram_normalParam);
	sceClibPrintf("texturedLitInstanced i_m0Param at address: %p\n", (void*)gxmTexturedLitInstancedVertexProgram_i_m0Param);
	sceClibPrintf("texturedLitInstanced i_m1Param at address: %p\n", (void*)gxmTexturedLitInstancedVertexProgram_i_m1Param);
	sceClibPrintf("texturedLitInstanced i_m2Param at address: %p\n", (void*)gxmTexturedLitInstancedVertexProgram_i_m2Param);
	sceClibPrintf("texturedLitInstanced i_m3Param at address: %p\n", (void*)gxmTexturedLitInstancedVertexProgram_i_m3Param);
	sceClibPrintf("texturedLitInstanced ViewMatrixParam at address: %p\n", (void*)gxmTexturedLitInstancedVertexProgram_u_viewMatrixParam);
	sceClibPrintf("texturedLitInstanced ProjectionMatrixParam at address: %p\n", (void*)gxmTexturedLitInstancedVertexProgram_u_projectionMatrixParam);

	SceGxmVertexAttribute texturedLitInstanced_vertex_attributes[7];
	SceGxmVertexStream texturedLitInstanced_vertex_streams[2];

	// Position, TexCoord, Normal (Per-Vertex Data)
	texturedLitInstanced_vertex_attributes[0].streamIndex = 0;
	texturedLitInstanced_vertex_attributes[0].offset = 0;
	texturedLitInstanced_vertex_attributes[0].format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
	texturedLitInstanced_vertex_attributes[0].componentCount = 3;
	texturedLitInstanced_vertex_attributes[0].regIndex = sceGxmProgramParameterGetResourceIndex(
		gxmTexturedLitInstancedVertexProgram_positionParam);

	texturedLitInstanced_vertex_attributes[1].streamIndex = 0;
	texturedLitInstanced_vertex_attributes[1].offset = offsetof(TexturedVertex, uv); //sizeof(vector3f);
	texturedLitInstanced_vertex_attributes[1].format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
	texturedLitInstanced_vertex_attributes[1].componentCount = 2;
	texturedLitInstanced_vertex_attributes[1].regIndex = sceGxmProgramParameterGetResourceIndex(
		gxmTexturedLitInstancedVertexProgram_texCoordParam);

	texturedLitInstanced_vertex_attributes[2].streamIndex = 0;
	texturedLitInstanced_vertex_attributes[2].offset = offsetof(TexturedVertex, normal); //sizeof(vector3f) + sizeof(vector2f);
	texturedLitInstanced_vertex_attributes[2].format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
	texturedLitInstanced_vertex_attributes[2].componentCount = 3;
	texturedLitInstanced_vertex_attributes[2].regIndex = sceGxmProgramParameterGetResourceIndex(
		gxmTexturedLitInstancedVertexProgram_normalParam);

	// Per-Instance Matrix rows on stream 1
	texturedLitInstanced_vertex_attributes[3].streamIndex = 1;
	texturedLitInstanced_vertex_attributes[3].offset = 0; // row 0
	texturedLitInstanced_vertex_attributes[3].format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
	texturedLitInstanced_vertex_attributes[3].componentCount = 4;
	texturedLitInstanced_vertex_attributes[3].regIndex = sceGxmProgramParameterGetResourceIndex(
		gxmTexturedLitInstancedVertexProgram_i_m0Param);

	texturedLitInstanced_vertex_attributes[4].streamIndex = 1;
	texturedLitInstanced_vertex_attributes[4].offset = 16; // row 1 (4 floats * 4 bytes)
	texturedLitInstanced_vertex_attributes[4].format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
	texturedLitInstanced_vertex_attributes[4].componentCount = 4;
	texturedLitInstanced_vertex_attributes[4].regIndex = sceGxmProgramParameterGetResourceIndex(
		gxmTexturedLitInstancedVertexProgram_i_m1Param);

	texturedLitInstanced_vertex_attributes[5].streamIndex = 1;
	texturedLitInstanced_vertex_attributes[5].offset = 32; // row 2
	texturedLitInstanced_vertex_attributes[5].format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
	texturedLitInstanced_vertex_attributes[5].componentCount = 4;
	texturedLitInstanced_vertex_attributes[5].regIndex = sceGxmProgramParameterGetResourceIndex(
		gxmTexturedLitInstancedVertexProgram_i_m2Param);

	texturedLitInstanced_vertex_attributes[6].streamIndex = 1;
	texturedLitInstanced_vertex_attributes[6].offset = 48; // row 3
	texturedLitInstanced_vertex_attributes[6].format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
	texturedLitInstanced_vertex_attributes[6].componentCount = 4;
	texturedLitInstanced_vertex_attributes[6].regIndex = sceGxmProgramParameterGetResourceIndex(
		gxmTexturedLitInstancedVertexProgram_i_m3Param);

	// Stream 0: Per-Vertex Data (indexed by vertex index)
	texturedLitInstanced_vertex_streams[0].stride = sizeof(struct TexturedVertex);
	texturedLitInstanced_vertex_streams[0].indexSource = SCE_GXM_INDEX_SOURCE_INDEX_16BIT;

	// Stream 1: Instance ID (automatically incremented)
	texturedLitInstanced_vertex_streams[1].stride = sizeof(InstanceData); // 64 bytes (4x float4)
	texturedLitInstanced_vertex_streams[1].indexSource = SCE_GXM_INDEX_SOURCE_INSTANCE_16BIT;

	err = sceGxmShaderPatcherCreateVertexProgram(gxmShaderPatcher,
		gxmTexturedLitInstancedVertexProgramID, 
		texturedLitInstanced_vertex_attributes, 7, 
		texturedLitInstanced_vertex_streams, 2, 
		&gxmTexturedLitInstancedVertexProgramPatched);

	if (err == 0)
	{
		sceClibPrintf("texturedLitInstanced VertexProgram created at address: %p\n", (void*)gxmTexturedLitInstancedVertexProgramPatched);
	}
	else
	{
		sceClibPrintf("texturedLitInstanced VertexProgram creation failed\n");
	}

	// Re-use the same fragment program as non-instanced version
	//...

	/*
	*	Terrain Shader
	*/
	

	err = sceGxmShaderPatcherRegisterProgram(gxmShaderPatcher, gxmProgTerrainVertexGxp, &gxmTerrainVertexProgramID);
	sceClibPrintf("sceGxmShaderPatcherRegisterProgram(terrainVertexProgramGxp): 0x%08X\n", err);
	err = sceGxmShaderPatcherRegisterProgram(gxmShaderPatcher, gxmProgTerrainFragmentGxp, &gxmTerrainFragmentProgramID);
	sceClibPrintf("sceGxmShaderPatcherRegisterProgram(terrainFragmentProgramGxp): 0x%08X\n", err);

	const SceGxmProgram* terrainVertexProgram =
		sceGxmShaderPatcherGetProgramFromId(gxmTerrainVertexProgramID);
	const SceGxmProgram* terrainFragmentProgram =
		sceGxmShaderPatcherGetProgramFromId(gxmTerrainFragmentProgramID);

	/*
	static const SceGxmProgramParameter* gxmTerrainVertexProgram_positionParam;
	static const SceGxmProgramParameter* gxmTeerrainVertexProgram_texCoordParam;
	static const SceGxmProgramParameter* gxmTerrainVertexProgram_normalParam;
	static const SceGxmProgramParameter* gxmTerrainVertexProgram_tangentParam;
	static const SceGxmProgramParameter* gxmTerrainVertexProgram_bitangentParam;
	*/

	findGxmShaderAttributeByName(terrainVertexProgram, "in_position", &gxmTerrainVertexProgram_positionParam);
	findGxmShaderAttributeByName(terrainVertexProgram, "in_texCoord", &gxmTerrainVertexProgram_texCoordParam);
	findGxmShaderAttributeByName(terrainVertexProgram, "in_normal", &gxmTerrainVertexProgram_normalParam);
	findGxmShaderAttributeByName(terrainVertexProgram, "in_tangent", &gxmTerrainVertexProgram_tangentParam);
	findGxmShaderAttributeByName(terrainVertexProgram, "in_bitangent", &gxmTerrainVertexProgram_bitangentParam);
	findGxmShaderUniformByName(terrainVertexProgram, "u_modelMatrix", &gxmTerrainVertexProgram_u_modelMatrixParam);
	findGxmShaderUniformByName(terrainVertexProgram, "u_perVFrame.u_viewMatrix", &gxmTerrainVertexProgram_u_viewMatrixParam);
	findGxmShaderUniformByName(terrainVertexProgram, "u_perVFrame.u_projectionMatrix", &gxmTerrainVertexProgram_u_projectionMatrixParam);
	findGxmShaderUniformByName(terrainFragmentProgram, "u_perPFrame.u_lightCount", &gxmTerrainFragmentProgram_u_lightCountParam);
	findGxmShaderUniformByName(terrainFragmentProgram, "u_perPFrame.u_lightColors", &gxmTerrainFragmentProgram_u_lightColorsParam);
	findGxmShaderUniformByName(terrainFragmentProgram, "u_perPFrame.u_lightPowers", &gxmTerrainFragmentProgram_u_lightPowersParam);
	findGxmShaderUniformByName(terrainFragmentProgram, "u_perPFrame.u_lightRadii", &gxmTerrainFragmentProgram_u_lightRadiiParam);
	findGxmShaderUniformByName(terrainFragmentProgram, "u_perPFrame.u_lightPositions", &gxmTerrainFragmentProgram_u_lightPositionsParam);
	findGxmShaderUniformByName(terrainFragmentProgram, "u_F0", &gxmTerrainFragmentProgram_u_F0Param);

	sceClibPrintf("terrain PositionParam at address: %p\n", (void*)gxmTerrainVertexProgram_positionParam);
	sceClibPrintf("terrain TexCoordParam at address: %p\n", (void*)gxmTerrainVertexProgram_texCoordParam);
	sceClibPrintf("terrain NormalParam at address: %p\n", (void*)gxmTerrainVertexProgram_normalParam);
	sceClibPrintf("terrain ModelMatrixParam at address: %p\n", (void*)gxmTerrainVertexProgram_u_modelMatrixParam);
	sceClibPrintf("terrain ViewMatrixParam at address: %p\n", (void*)gxmTerrainVertexProgram_u_viewMatrixParam);
	sceClibPrintf("terrain ProjectionMatrixParam at address: %p\n", (void*)gxmTerrainVertexProgram_u_projectionMatrixParam);
	sceClibPrintf("terrain LightCountParam at address: %p\n", (void*)gxmTerrainFragmentProgram_u_lightCountParam);
	sceClibPrintf("terrain LightPositionsParam at address: %p\n", (void*)gxmTerrainFragmentProgram_u_lightPositionsParam);
	sceClibPrintf("terrain LightColorsParam at address: %p\n", (void*)gxmTerrainFragmentProgram_u_lightColorsParam);
	sceClibPrintf("terrain LightPowersParam at address: %p\n", (void*)gxmTerrainFragmentProgram_u_lightPowersParam);
	sceClibPrintf("terrain LightRadiiParam at address: %p\n", (void*)gxmTerrainFragmentProgram_u_lightRadiiParam);
	sceClibPrintf("terrain F0 at address: %p\n", (void*)gxmTerrainFragmentProgram_u_F0Param);

	SceGxmVertexAttribute terrain_vertex_attributes[5];
	SceGxmVertexStream terrain_vertex_stream;
	terrain_vertex_attributes[0].streamIndex = 0;
	terrain_vertex_attributes[0].offset = 0;
	terrain_vertex_attributes[0].format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
	terrain_vertex_attributes[0].componentCount = 3;
	terrain_vertex_attributes[0].regIndex = sceGxmProgramParameterGetResourceIndex(
		gxmTerrainVertexProgram_positionParam);
	terrain_vertex_attributes[1].streamIndex = 0;
	terrain_vertex_attributes[1].offset = offsetof(TerrainPBRVertex, u);
	terrain_vertex_attributes[1].format = SCE_GXM_ATTRIBUTE_FORMAT_F32;
	terrain_vertex_attributes[1].componentCount = 2;
	terrain_vertex_attributes[1].regIndex = sceGxmProgramParameterGetResourceIndex(
		gxmTerrainVertexProgram_texCoordParam);
	terrain_vertex_attributes[2].streamIndex = 0;
	terrain_vertex_attributes[2].offset = offsetof(TerrainPBRVertex, nx);
	terrain_vertex_attributes[2].format = SCE_GXM_ATTRIBUTE_FORMAT_S16N;  // Signed 16-bit normalized
	terrain_vertex_attributes[2].componentCount = 3;
	terrain_vertex_attributes[2].regIndex = sceGxmProgramParameterGetResourceIndex(
		gxmTerrainVertexProgram_normalParam);
	terrain_vertex_attributes[3].streamIndex = 0;
	terrain_vertex_attributes[3].offset = offsetof(TerrainPBRVertex, tx);
	terrain_vertex_attributes[3].format = SCE_GXM_ATTRIBUTE_FORMAT_S16N;  // Signed 16-bit normalized
	terrain_vertex_attributes[3].componentCount = 3;
	terrain_vertex_attributes[3].regIndex = sceGxmProgramParameterGetResourceIndex(
		gxmTerrainVertexProgram_tangentParam);
	terrain_vertex_attributes[4].streamIndex = 0;
	terrain_vertex_attributes[4].offset = offsetof(TerrainPBRVertex, bx);
	terrain_vertex_attributes[4].format = SCE_GXM_ATTRIBUTE_FORMAT_S16N;  // Signed 16-bit normalized
	terrain_vertex_attributes[4].componentCount = 3;
	terrain_vertex_attributes[4].regIndex = sceGxmProgramParameterGetResourceIndex(
		gxmTerrainVertexProgram_bitangentParam);

	terrain_vertex_stream.stride = sizeof(struct TerrainPBRVertex);
	terrain_vertex_stream.indexSource = SCE_GXM_INDEX_SOURCE_INDEX_32BIT;

	err = sceGxmShaderPatcherCreateVertexProgram(gxmShaderPatcher,
		gxmTerrainVertexProgramID, terrain_vertex_attributes,
		5, &terrain_vertex_stream, 1, &gxmTerrainVertexProgramPatched);
	if (err == 0)
	{
		sceClibPrintf("Terrain VertexProgram created at address: %p\n", (void*)gxmTerrainVertexProgramPatched);
	}
	else
	{
		sceClibPrintf("Terrain VertexProgram creation failed\n");
	}

	err = sceGxmShaderPatcherCreateFragmentProgram(gxmShaderPatcher,
		gxmTerrainFragmentProgramID, SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4,
		gxmMultisampleMode, NULL, terrainVertexProgram,
		&gxmTerrainFragmentProgramPatched);
	if (err == 0)
	{
		sceClibPrintf("Terrain FragmentProgram created at address: %p\n", (void*)gxmTerrainFragmentProgramPatched);
	}
	else
	{
		sceClibPrintf("Terrain FragmentProgram creation failed\n");
	}

	//Now allocate persistent memory for the per-frame uniforms used by the lit shader
	initializeUniformBuffers();
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

	// Get previous fill mode, clear screen requires to be set to FILL
	bool previousMode = wireFrame;

	sceGxmSetFrontPolygonMode(gxmContext, SCE_GXM_POLYGON_MODE_TRIANGLE_FILL);
	sceGxmSetBackPolygonMode(gxmContext, SCE_GXM_POLYGON_MODE_TRIANGLE_FILL);
	
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

	//restore previous mode if necessary
	if (wireFrame)
	{
		sceGxmSetFrontPolygonMode(gxmContext, SCE_GXM_POLYGON_MODE_TRIANGLE_LINE);
		sceGxmSetBackPolygonMode(gxmContext, SCE_GXM_POLYGON_MODE_TRIANGLE_LINE);
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

	//For delta time
	SceRtcTick prevTick;
	sceRtcGetCurrentTick(&prevTick);
	unsigned int tickRes = sceRtcGetTickResolution(); //ticks per second

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

	// Each face is defined in �face space� with its own 4 vertices:
	// UVs: bottom-left (0,0), bottom-right (1,0), top-right (1,1), top-left (0,1)
	std::vector<UnlitTexturedVertex> texturedCubeVertices =
	{
		// Front face (z = +CUBE_HALF_SIZE)
		UnlitTexturedVertex(-CUBE_HALF_SIZE, -CUBE_HALF_SIZE,  CUBE_HALF_SIZE, 0.0f, 1.0f - 0.0f), // 0: BL
		UnlitTexturedVertex(CUBE_HALF_SIZE, -CUBE_HALF_SIZE,  CUBE_HALF_SIZE, 1.0f, 1.0f - 0.0f), // 1: BR
		UnlitTexturedVertex(CUBE_HALF_SIZE,  CUBE_HALF_SIZE,  CUBE_HALF_SIZE, 1.0f, 1.0f - 1.0f), // 2: TR
		UnlitTexturedVertex(-CUBE_HALF_SIZE,  CUBE_HALF_SIZE,  CUBE_HALF_SIZE, 0.0f, 1.0f - 1.0f), // 3: TL

		// Right face (x = +CUBE_HALF_SIZE)
		UnlitTexturedVertex(CUBE_HALF_SIZE, -CUBE_HALF_SIZE,  CUBE_HALF_SIZE, 0.0f, 1.0f - 0.0f), // 4: BL (front lower)
		UnlitTexturedVertex(CUBE_HALF_SIZE, -CUBE_HALF_SIZE, -CUBE_HALF_SIZE, 1.0f, 1.0f - 0.0f), // 5: BR (back lower)
		UnlitTexturedVertex(CUBE_HALF_SIZE,  CUBE_HALF_SIZE, -CUBE_HALF_SIZE, 1.0f, 1.0f - 1.0f), // 6: TR (back upper)
		UnlitTexturedVertex(CUBE_HALF_SIZE,  CUBE_HALF_SIZE,  CUBE_HALF_SIZE, 0.0f, 1.0f - 1.0f), // 7: TL (front upper)

		// Back face (z = -CUBE_HALF_SIZE)
		// When viewed from the back the �local� bottom-left is (CUBE_HALF_SIZE, -CUBE_HALF_SIZE, -CUBE_HALF_SIZE)
		UnlitTexturedVertex(CUBE_HALF_SIZE, -CUBE_HALF_SIZE, -CUBE_HALF_SIZE, 0.0f, 1.0f - 0.0f), // 8: BL
		UnlitTexturedVertex(-CUBE_HALF_SIZE, -CUBE_HALF_SIZE, -CUBE_HALF_SIZE, 1.0f, 1.0f - 0.0f), // 9: BR
		UnlitTexturedVertex(-CUBE_HALF_SIZE,  CUBE_HALF_SIZE, -CUBE_HALF_SIZE, 1.0f, 1.0f - 1.0f), // 10: TR
		UnlitTexturedVertex(CUBE_HALF_SIZE,  CUBE_HALF_SIZE, -CUBE_HALF_SIZE, 0.0f, 1.0f - 1.0f), // 11: TL

		// Left face (x = -CUBE_HALF_SIZE)
		// Viewed from the left, the bottom-left is (-CUBE_HALF_SIZE, -CUBE_HALF_SIZE, -CUBE_HALF_SIZE)
		UnlitTexturedVertex(-CUBE_HALF_SIZE, -CUBE_HALF_SIZE, -CUBE_HALF_SIZE, 0.0f, 1.0f - 0.0f), // 12: BL
		UnlitTexturedVertex(-CUBE_HALF_SIZE, -CUBE_HALF_SIZE,  CUBE_HALF_SIZE, 1.0f, 1.0f - 0.0f), // 13: BR
		UnlitTexturedVertex(-CUBE_HALF_SIZE,  CUBE_HALF_SIZE,  CUBE_HALF_SIZE, 1.0f, 1.0f - 1.0f), // 14: TR
		UnlitTexturedVertex(-CUBE_HALF_SIZE,  CUBE_HALF_SIZE, -CUBE_HALF_SIZE, 0.0f, 1.0f - 1.0f), // 15: TL

		// Top face (y = +CUBE_HALF_SIZE)
		// Viewed from above, we define the vertices so that the lower edge corresponds to z = +CUBE_HALF_SIZE.
		UnlitTexturedVertex(-CUBE_HALF_SIZE,  CUBE_HALF_SIZE,  CUBE_HALF_SIZE, 0.0f, 1.0f - 0.0f), // 16: BL
		UnlitTexturedVertex(CUBE_HALF_SIZE,  CUBE_HALF_SIZE,  CUBE_HALF_SIZE, 1.0f, 1.0f - 0.0f), // 17: BR
		UnlitTexturedVertex(CUBE_HALF_SIZE,  CUBE_HALF_SIZE, -CUBE_HALF_SIZE, 1.0f, 1.0f - 1.0f), // 18: TR
		UnlitTexturedVertex(-CUBE_HALF_SIZE,  CUBE_HALF_SIZE, -CUBE_HALF_SIZE, 0.0f, 1.0f - 1.0f), // 19: TL

		// Bottom face (y = -CUBE_HALF_SIZE)
		// Viewed from below, the bottom-left is defined as (-CUBE_HALF_SIZE, -CUBE_HALF_SIZE, -CUBE_HALF_SIZE)
		UnlitTexturedVertex(-CUBE_HALF_SIZE, -CUBE_HALF_SIZE, -CUBE_HALF_SIZE, 0.0f, 1.0f - 0.0f), // 20: BL
		UnlitTexturedVertex(CUBE_HALF_SIZE, -CUBE_HALF_SIZE, -CUBE_HALF_SIZE, 1.0f, 1.0f - 0.0f), // 21: BR
		UnlitTexturedVertex(CUBE_HALF_SIZE, -CUBE_HALF_SIZE,  CUBE_HALF_SIZE, 1.0f, 1.0f - 1.0f), // 22: TR
		UnlitTexturedVertex(-CUBE_HALF_SIZE, -CUBE_HALF_SIZE,  CUBE_HALF_SIZE, 0.0f, 1.0f - 1.0f)  // 23: TL
	};

	std::vector<TexturedVertex> litTexturedCubeVertices =
	{
		// Front face (Z+)
		TexturedVertex(-CUBE_HALF_SIZE, -CUBE_HALF_SIZE,  CUBE_HALF_SIZE, 0.0f, 1.0f - 0.0f, 0.0f, 0.0f, 1.0f), // 0: BL
		TexturedVertex(CUBE_HALF_SIZE, -CUBE_HALF_SIZE,  CUBE_HALF_SIZE, 1.0f, 1.0f - 0.0f, 0.0f, 0.0f, 1.0f), // 1: BR
		TexturedVertex(CUBE_HALF_SIZE,  CUBE_HALF_SIZE,  CUBE_HALF_SIZE, 1.0f, 1.0f - 1.0f, 0.0f, 0.0f, 1.0f), // 2: TR
		TexturedVertex(-CUBE_HALF_SIZE,  CUBE_HALF_SIZE,  CUBE_HALF_SIZE, 0.0f, 1.0f - 1.0f, 0.0f, 0.0f, 1.0f), // 3: TL

		// Right face (X+)
		TexturedVertex(CUBE_HALF_SIZE, -CUBE_HALF_SIZE,  CUBE_HALF_SIZE, 0.0f, 1.0f - 0.0f, 1.0f, 0.0f, 0.0f), // 4: BL 
		TexturedVertex(CUBE_HALF_SIZE, -CUBE_HALF_SIZE, -CUBE_HALF_SIZE, 1.0f, 1.0f - 0.0f, 1.0f, 0.0f, 0.0f), // 5: BR 
		TexturedVertex(CUBE_HALF_SIZE,  CUBE_HALF_SIZE, -CUBE_HALF_SIZE, 1.0f, 1.0f - 1.0f, 1.0f, 0.0f, 0.0f), // 6: TR 
		TexturedVertex(CUBE_HALF_SIZE,  CUBE_HALF_SIZE,  CUBE_HALF_SIZE, 0.0f, 1.0f - 1.0f, 1.0f, 0.0f, 0.0f), // 7: TL 

		// Back face (Z-) - FIXED NORMAL DIRECTION
		TexturedVertex(CUBE_HALF_SIZE, -CUBE_HALF_SIZE, -CUBE_HALF_SIZE, 0.0f, 1.0f - 0.0f, 0.0f, 0.0f, -1.0f), // 8: BL
		TexturedVertex(-CUBE_HALF_SIZE, -CUBE_HALF_SIZE, -CUBE_HALF_SIZE, 1.0f, 1.0f - 0.0f, 0.0f, 0.0f, -1.0f), // 9: BR
		TexturedVertex(-CUBE_HALF_SIZE,  CUBE_HALF_SIZE, -CUBE_HALF_SIZE, 1.0f, 1.0f - 1.0f, 0.0f, 0.0f, -1.0f), // 10: TR
		TexturedVertex(CUBE_HALF_SIZE,  CUBE_HALF_SIZE, -CUBE_HALF_SIZE, 0.0f, 1.0f - 1.0f, 0.0f, 0.0f, -1.0f), // 11: TL

		// Left face (X-)
		TexturedVertex(-CUBE_HALF_SIZE, -CUBE_HALF_SIZE, -CUBE_HALF_SIZE, 0.0f, 1.0f - 0.0f, -1.0f, 0.0f, 0.0f), // 12: BL
		TexturedVertex(-CUBE_HALF_SIZE, -CUBE_HALF_SIZE,  CUBE_HALF_SIZE, 1.0f, 1.0f - 0.0f, -1.0f, 0.0f, 0.0f), // 13: BR
		TexturedVertex(-CUBE_HALF_SIZE,  CUBE_HALF_SIZE,  CUBE_HALF_SIZE, 1.0f, 1.0f - 1.0f, -1.0f, 0.0f, 0.0f), // 14: TR
		TexturedVertex(-CUBE_HALF_SIZE,  CUBE_HALF_SIZE, -CUBE_HALF_SIZE, 0.0f, 1.0f - 1.0f, -1.0f, 0.0f, 0.0f), // 15: TL

		// Top face (Y+)
		TexturedVertex(-CUBE_HALF_SIZE,  CUBE_HALF_SIZE,  CUBE_HALF_SIZE, 0.0f, 1.0f - 0.0f, 0.0f, 1.0f, 0.0f), // 16: BL
		TexturedVertex(CUBE_HALF_SIZE,  CUBE_HALF_SIZE,  CUBE_HALF_SIZE, 1.0f, 1.0f - 0.0f, 0.0f, 1.0f, 0.0f), // 17: BR
		TexturedVertex(CUBE_HALF_SIZE,  CUBE_HALF_SIZE, -CUBE_HALF_SIZE, 1.0f, 1.0f - 1.0f, 0.0f, 1.0f, 0.0f), // 18: TR
		TexturedVertex(-CUBE_HALF_SIZE,  CUBE_HALF_SIZE, -CUBE_HALF_SIZE, 0.0f, 1.0f - 1.0f, 0.0f, 1.0f, 0.0f), // 19: TL

		// Bottom face (Y-)
		TexturedVertex(-CUBE_HALF_SIZE, -CUBE_HALF_SIZE, -CUBE_HALF_SIZE, 0.0f, 1.0f - 0.0f, 0.0f, -1.0f, 0.0f), // 20: BL
		TexturedVertex(CUBE_HALF_SIZE, -CUBE_HALF_SIZE, -CUBE_HALF_SIZE, 1.0f, 1.0f - 0.0f, 0.0f, -1.0f, 0.0f), // 21: BR
		TexturedVertex(CUBE_HALF_SIZE, -CUBE_HALF_SIZE,  CUBE_HALF_SIZE, 1.0f, 1.0f - 1.0f, 0.0f, -1.0f, 0.0f), // 22: TR
		TexturedVertex(-CUBE_HALF_SIZE, -CUBE_HALF_SIZE,  CUBE_HALF_SIZE, 0.0f, 1.0f - 1.0f, 0.0f, -1.0f, 0.0f)  // 23: TL
	};

	static const unsigned short texturedCubeIndices[36] =
	{
		// Front face
		0,  1,  2,
		0,  2,  3,

		// Right face
		4,  5,  6,
		4,  6,  7,

		// Back face
		8,  9,  10,
		8,  10, 11,

		// Left face
		12, 13, 14,
		12, 14, 15,

		// Top face
		16, 17, 18,
		16, 18, 19,

		// Bottom face
		20, 21, 22,
		20, 22, 23
	};

	//Another more different way to define the data
	std::vector<Vector3f> _surfacePositions =
	{
		{ -1.0f, 1.0f,  0.0f }, //top left
		{ -1.0f, -1.0f, 0.0f }, //bottom left
		{ 1.0f,  -1.0f, 0.0f }, //bottom right
		{ 1.0f,  1.0f,  0.0f }  //top right
	};
	std::vector<Vector2f> _surfaceUVs =
	{
		{ 0, 0 }, //top left
		{ 0, 1 }, //bottom left
		{ 1, 1 }, //bottom right
		{ 1, 0 }  //top right
	};
	std::vector<unsigned int> _surfaceIndices =
	{
		0, 1, 3,
		3, 1, 2
	};

	Terrain terrain;
	terrain.initialize();

	//allocate memory for the vertex data
	sceClibPrintf("Allocating memory for the vertex data...\n");
	SceUID colorCubeVertexDataUID, texturedCubeVertexDataUID, litTexturedCubeVertexDataUID, surfaceVertexDataUID, indexDataUID, texturedIndexDataUID, surfaceIndexDataUID,
		terrainVertexDataUID, terrainIndexDataUID;

	sceClibPrintf("...colored cube vertex data\n");
	UnlitColorVertex* cVertexData = (UnlitColorVertex*)gpuAllocMap(cubeVertices.size() * sizeof(UnlitColorVertex),
		SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE, SCE_GXM_MEMORY_ATTRIB_READ,
		&colorCubeVertexDataUID);

	sceClibPrintf("...textured cube vertex data\n");
	UnlitTexturedVertex* tVertexData = (UnlitTexturedVertex*)gpuAllocMap(24 * sizeof(UnlitTexturedVertex),
		SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW, SCE_GXM_MEMORY_ATTRIB_READ,
		&texturedCubeVertexDataUID);

	sceClibPrintf("...lit textured cube vertex data\n");
	TexturedVertex* ltVertexData = (TexturedVertex*)gpuAllocMap(litTexturedCubeVertices.size() * sizeof(TexturedVertex),
		SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW, SCE_GXM_MEMORY_ATTRIB_READ,
		&litTexturedCubeVertexDataUID);

	sceClibPrintf("...surface vertex data\n");
	UnlitTexturedVertex* sVertexData = (UnlitTexturedVertex*)gpuAllocMap(4 * sizeof(UnlitTexturedVertex),
		SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW, SCE_GXM_MEMORY_ATTRIB_READ,
		&surfaceVertexDataUID);

	sceClibPrintf("Allocating memory for the index data\n");
	unsigned short* indexData = (unsigned short*)gpuAllocMap(cubeIndices.size() * sizeof(unsigned short),
		SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW, SCE_GXM_MEMORY_ATTRIB_READ,
		&indexDataUID);

	//used for both the textured cube and the lit textured cube
	unsigned short* texturedIndexData = (unsigned short*)gpuAllocMap(36 * sizeof(unsigned short),
		SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW, SCE_GXM_MEMORY_ATTRIB_READ,
		&texturedIndexDataUID);

	unsigned int* surfaceIndexData = (unsigned int*)gpuAllocMap(_surfaceIndices.size() * sizeof(unsigned int),
		SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW, SCE_GXM_MEMORY_ATTRIB_READ,
		&surfaceIndexDataUID);

	//interweave the position and colors into Vertex struct
	for (int i = 0; i < cubeVertices.size(); i++)
	{
		cVertexData[i] = (struct UnlitColorVertex){ cubeVertices[i].x, cubeVertices[i].y, cubeVertices[i].z, cubeVerticesColor[i] };
	}
	//interweave the surface position and UVs into Vertex struct
	for (int i = 0; i < _surfacePositions.size(); i++)
	{
		sVertexData[i] = (struct UnlitTexturedVertex){ _surfacePositions[i].x, _surfacePositions[i].y, _surfacePositions[i].z, _surfaceUVs[i].x, _surfaceUVs[i].y };
	}
	memcpy(tVertexData, texturedCubeVertices.data(), 24 * sizeof(UnlitTexturedVertex));
	memcpy(ltVertexData, litTexturedCubeVertices.data(), litTexturedCubeVertices.size() * sizeof(TexturedVertex));
	//copy the index data
	memcpy(indexData, cubeIndices.data(), cubeIndices.size() * sizeof(unsigned short));
	memcpy(texturedIndexData, texturedCubeIndices, 36 * sizeof(unsigned short));
	memcpy(surfaceIndexData, _surfaceIndices.data(), _surfaceIndices.size() * sizeof(unsigned int));
	//for (int i = 0; i < cubeIndices.size(); i++)
	//{
	//	indexData[i] = cubeIndices[i];
	//}

	/*
	std::vector<ChunkGPUData> chunkGPUData;
	chunkGPUData.resize(Terrain::CHUNKS_PER_SIDE* Terrain::CHUNKS_PER_SIDE);

	sceClibPrintf("Allocating GPU memory for terrain chunks...\n");
	int chunkIndex = 0;
	for (const auto& chunk : terrain.getChunks())
	{
		ChunkGPUData& gpuData = chunkGPUData[chunkIndex];

		for (int lod = 0; lod < TerrainChunk::LOD_COUNT; lod++)
		{
			TerrainChunk::LODMesh* lodMesh = chunk->getLODMesh(static_cast<TerrainChunk::LODLevel>(lod));

			sceClibPrintf("Chunk %d LOD %d: %d vertices, %d indices\n", chunkIndex, lod, lodMesh->vertexCount, lodMesh->indexCount);

			// Allocate vertex data
			gpuData.vertexPtrs[lod] = (TerrainPBRVertex*)gpuAllocMap(
				lodMesh->vertexCount * sizeof(TerrainPBRVertex),
				SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW,
				SCE_GXM_MEMORY_ATTRIB_READ,
				&gpuData.vertexUIDs[lod]
			);

			// Allocate index data
			gpuData.indexPtrs[lod] = (uint16_t*)gpuAllocMap(
				lodMesh->indexCount * sizeof(uint16_t),
				SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW,
				SCE_GXM_MEMORY_ATTRIB_READ,
				&gpuData.indexUIDs[lod]
			);

			// Copy data
			memcpy(gpuData.vertexPtrs[lod], lodMesh->tempVertices->data(), lodMesh->vertexCount * sizeof(TerrainPBRVertex));
			memcpy(gpuData.indexPtrs[lod], lodMesh->tempIndices->data(), lodMesh->indexCount * sizeof(uint16_t));
		}

		chunkIndex++;
	}*/

	// Allocate memory for the texture data and load the texture
	SceGxmTexture texture, alphaTexture, allWhiteTexture;// , terrainDiffuse, terrainNormal, terrainRoughness;
	SceUID textureID = 0;
	SceUID alphaTextureID = 0;
	SceUID allWhiteTextureID = 0;
	SceUID terrainDiffuseID = 0;
	SceUID terrainNormalID = 0;
	SceUID terrainRoughnessID = 0;
	void* textureData = nullptr;
	void* alphaTextureData = nullptr;
	void* allWhiteTextureData = nullptr;
	/*
	void* terrainDiffuseData = nullptr;
	void* terrainNormalData = nullptr;
	void* terrainRoughnessData = nullptr;
	*/
	int initResult = 0;

	sceClibPrintf("Allocating memory for the texture data...\n");

	textureData = gpuAllocMap(EMP_Logo_Small_width * EMP_Logo_Small_height * EMP_Logo_Small_comp,
		SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW, SCE_GXM_MEMORY_ATTRIB_READ,
		&textureID);
	memcpy(textureData, EMP_Logo_Small_data, sizeof(EMP_Logo_Small_data));

	alphaTextureData = gpuAllocMap(EMP_Logo_Small_Alpha_width * EMP_Logo_Small_Alpha_height * EMP_Logo_Small_Alpha_comp,
		SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW, SCE_GXM_MEMORY_ATTRIB_READ,
		&alphaTextureID);
	memcpy(alphaTextureData, EMP_Logo_Small_Alpha_data, sizeof(EMP_Logo_Small_Alpha_data));

	allWhiteTextureData = gpuAllocMap(allWhite_width * allWhite_height * allWhite_comp,
		SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW, SCE_GXM_MEMORY_ATTRIB_READ,
		&allWhiteTextureID);
	memcpy(allWhiteTextureData, allWhite_data, sizeof(allWhite_data));

	/*
	terrainDiffuseData = gpuAllocMap(repeatingGravel_512_diffuse_width * repeatingGravel_512_diffuse_height * repeatingGravel_512_diffuse_comp,
		SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW, SCE_GXM_MEMORY_ATTRIB_READ,
		&terrainDiffuseID);
	memcpy(terrainDiffuseData, repeatingGravel_512_diffuse_data, sizeof(repeatingGravel_512_diffuse_data));

	terrainNormalData = gpuAllocMap(repeatingGravel_512_normal_width * repeatingGravel_512_normal_height * repeatingGravel_512_normal_comp,
		SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW, SCE_GXM_MEMORY_ATTRIB_READ,
		&terrainNormalID);
	memcpy(terrainNormalData, repeatingGravel_512_normal_data, sizeof(repeatingGravel_512_normal_data));

	terrainRoughnessData = gpuAllocMap(repeatingGravel_512_roughness_width * repeatingGravel_512_roughness_height * repeatingGravel_512_roughness_comp,
		SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW, SCE_GXM_MEMORY_ATTRIB_READ,
		&terrainRoughnessID);
	memcpy(terrainRoughnessData, repeatingGravel_512_roughness_data, sizeof(repeatingGravel_512_roughness_data));
	*/

	sceClibPrintf("Initializing texture...\n");
	initResult = sceGxmTextureInitLinear(&texture, textureData, SCE_GXM_TEXTURE_FORMAT_U8U8U8_BGR, EMP_Logo_Small_width, EMP_Logo_Small_height, 0);
	//initResult = sceGxmTextureInitLinear(&texture, textureData, SCE_GXM_TEXTURE_FORMAT_U8U8U8U8_ABGR, TEX_WIDTH, TEX_HEIGHT, 1);
	if (initResult != 0)
	{
		sceClibPrintf("sceGxmTextureInitLinear() failed: 0x%08X\n", initResult);
	}

	sceClibPrintf("Initializing alpha texture...\n");
	initResult = sceGxmTextureInitLinear(&alphaTexture, alphaTextureData, SCE_GXM_TEXTURE_FORMAT_U8U8U8U8_ABGR, EMP_Logo_Small_Alpha_width, EMP_Logo_Small_Alpha_height, 0);
	if (initResult != 0)
	{
		sceClibPrintf("sceGxmTextureInitLinear() failed: 0x%08X\n", initResult);
	}

	sceClibPrintf("Initializing all white texture...\n");
	initResult = sceGxmTextureInitLinear(&allWhiteTexture, allWhiteTextureData, SCE_GXM_TEXTURE_FORMAT_U8U8U8U8_ABGR, allWhite_width, allWhite_height, 0);
	if (initResult != 0)
	{
		sceClibPrintf("sceGxmTextureInitLinear() failed: 0x%08X\n", initResult);
	}

	/*
	sceClibPrintf("Initializing terrain diffuse texture...\n");
	initResult = sceGxmTextureInitLinear(&terrainDiffuse, terrainDiffuseData, SCE_GXM_TEXTURE_FORMAT_U8U8U8_BGR, repeatingGravel_512_diffuse_width, repeatingGravel_512_diffuse_height, 0);
	if (initResult != 0)
	{
		sceClibPrintf("sceGxmTextureInitLinear() failed: 0x%08X\n", initResult);
	}

	sceClibPrintf("Initializing terrain normal texture...\n");
	initResult = sceGxmTextureInitLinear(&terrainNormal, terrainNormalData, SCE_GXM_TEXTURE_FORMAT_U8U8U8_BGR, repeatingGravel_512_normal_width, repeatingGravel_512_normal_height, 0);
	if (initResult != 0)
	{
		sceClibPrintf("sceGxmTextureInitLinear() failed: 0x%08X\n", initResult);
	}

	sceClibPrintf("Initializing terrain roughness texture...\n");
	initResult = sceGxmTextureInitLinear(&terrainRoughness, terrainRoughnessData, SCE_GXM_TEXTURE_FORMAT_U8U8U8_BGR, repeatingGravel_512_roughness_width, repeatingGravel_512_roughness_height, 0);
	if (initResult != 0)
	{
		sceClibPrintf("sceGxmTextureInitLinear() failed: 0x%08X\n", initResult);
	}
	*/



	// Set some texture parameters
	sceGxmTextureSetMinFilter(&texture, SCE_GXM_TEXTURE_FILTER_LINEAR);
	sceGxmTextureSetMagFilter(&texture, SCE_GXM_TEXTURE_FILTER_LINEAR);
	sceGxmTextureSetMipFilter(&texture, SCE_GXM_TEXTURE_MIP_FILTER_DISABLED);
	sceGxmTextureSetUAddrMode(&texture, SCE_GXM_TEXTURE_ADDR_REPEAT);
	sceGxmTextureSetVAddrMode(&texture, SCE_GXM_TEXTURE_ADDR_REPEAT);

	/*
	sceGxmTextureSetMinFilter(&terrainDiffuse, SCE_GXM_TEXTURE_FILTER_LINEAR);
	sceGxmTextureSetMagFilter(&terrainDiffuse, SCE_GXM_TEXTURE_FILTER_LINEAR);
	sceGxmTextureSetMipFilter(&terrainDiffuse, SCE_GXM_TEXTURE_MIP_FILTER_DISABLED);
	sceGxmTextureSetUAddrMode(&terrainDiffuse, SCE_GXM_TEXTURE_ADDR_REPEAT);
	sceGxmTextureSetVAddrMode(&terrainDiffuse, SCE_GXM_TEXTURE_ADDR_REPEAT);

	sceGxmTextureSetMinFilter(&terrainNormal, SCE_GXM_TEXTURE_FILTER_LINEAR);
	sceGxmTextureSetMagFilter(&terrainNormal, SCE_GXM_TEXTURE_FILTER_LINEAR);
	sceGxmTextureSetMipFilter(&terrainNormal, SCE_GXM_TEXTURE_MIP_FILTER_DISABLED);
	sceGxmTextureSetUAddrMode(&terrainNormal, SCE_GXM_TEXTURE_ADDR_REPEAT);
	sceGxmTextureSetVAddrMode(&terrainNormal, SCE_GXM_TEXTURE_ADDR_REPEAT);

	sceGxmTextureSetMinFilter(&terrainRoughness, SCE_GXM_TEXTURE_FILTER_LINEAR);
	sceGxmTextureSetMagFilter(&terrainRoughness, SCE_GXM_TEXTURE_FILTER_LINEAR);
	sceGxmTextureSetMipFilter(&terrainRoughness, SCE_GXM_TEXTURE_MIP_FILTER_DISABLED);
	sceGxmTextureSetUAddrMode(&terrainRoughness, SCE_GXM_TEXTURE_ADDR_REPEAT);
	sceGxmTextureSetVAddrMode(&terrainRoughness, SCE_GXM_TEXTURE_ADDR_REPEAT);
	*/

	Texture terrainDiffuseTex, terrainRoughTex, terrainNormalTex;

	terrainDiffuseTex.setFormat(SCE_GXM_TEXTURE_FORMAT_U8U8U8_BGR);
	terrainDiffuseTex.setSize(repeatingGravel_256_diffuse_width, repeatingGravel_256_diffuse_height);
	terrainDiffuseTex.setFilters(SCE_GXM_TEXTURE_FILTER_LINEAR, SCE_GXM_TEXTURE_FILTER_LINEAR, true);
	terrainDiffuseTex.setAddressModes(SCE_GXM_TEXTURE_ADDR_REPEAT, SCE_GXM_TEXTURE_ADDR_REPEAT);

	terrainRoughTex.setFormat(SCE_GXM_TEXTURE_FORMAT_U8U8U8_BGR);
	terrainRoughTex.setSize(repeatingGravel_256_roughness_width, repeatingGravel_256_roughness_height);
	terrainRoughTex.setFilters(SCE_GXM_TEXTURE_FILTER_LINEAR, SCE_GXM_TEXTURE_FILTER_LINEAR, true);
	terrainRoughTex.setAddressModes(SCE_GXM_TEXTURE_ADDR_REPEAT, SCE_GXM_TEXTURE_ADDR_REPEAT);

	terrainNormalTex.setFormat(SCE_GXM_TEXTURE_FORMAT_U8U8U8_BGR);
	terrainNormalTex.setSize(repeatingGravel_256_normal_width, repeatingGravel_256_normal_height);
	terrainNormalTex.setFilters(SCE_GXM_TEXTURE_FILTER_LINEAR, SCE_GXM_TEXTURE_FILTER_LINEAR, true);
	terrainNormalTex.setAddressModes(SCE_GXM_TEXTURE_ADDR_REPEAT, SCE_GXM_TEXTURE_ADDR_REPEAT);

	terrainDiffuseTex.loadFromData(repeatingGravel_256_diffuse_data, repeatingGravel_256_diffuse_comp);
	terrainRoughTex.loadFromData(repeatingGravel_256_roughness_data, repeatingGravel_256_roughness_comp);
	terrainNormalTex.loadFromData(repeatingGravel_256_normal_data, repeatingGravel_256_normal_comp, true);

	//set model position and rotation
	Vector3f colorCubePosition = { -0.8f, 1.0f, -2.5f };
	Vector3f colorCubeRotation = { 0.0f, 0.0f, 0.0f };
	Vector3f texturedCubePosition = { 0.8f, 1.0f, -2.5f };
	Vector3f texturedCubeRotation = { 0.0f, 0.0f, 0.0f };
	Vector3f alphaCubePosition = { 0.0f, 1.0f, -1.0f };
	Vector3f alphaCubeRotation = { 0.0f, 0.0f, 0.0f };
	//create model matrix
	Matrix4x4 colorCubeModelMatrix = createTransformationMatrix(colorCubePosition, colorCubeRotation, Vector3f{ 1.0f, 1.0f, 1.0f });
	Matrix4x4 texturedCubeModelMatrix = createTransformationMatrix(texturedCubePosition, texturedCubeRotation, Vector3f{ 1.0f, 1.0f, 1.0f });
	Matrix4x4 alphaCubeModelMatrix = createTransformationMatrix(alphaCubePosition, alphaCubeRotation, Vector3f{ 1.0f, 1.0f, 1.0f });
	Matrix4x4 surfaceTransformationMatrix = createTransformationMatrix(Vector3f{ 0.0f, 0.0f, 0.0f }, Vector3f{ 0.0f, 0.0f, 0.0f }, Vector3f{ 1.0f, 1.0f, 1.0f });

	//create view matrix
	Vector3f cameraPosition = { 0.0f, 1.0f, 0.0f };
	Vector3f cameraRotation = { 0.0f, 0.0f, 0.0f };
	//params: position, rotation, fov, aspect ratio, near plane, far plane
	Camera camera = Camera(CameraType::PERSPECTIVE, cameraPosition, cameraRotation, 45.0f, (float)DISPLAY_WIDTH / (float)DISPLAY_HEIGHT, 0.1f, 1000.0f);
	Camera orthoCam(CameraType::ORTHOGRAPHIC, Vector3f(0.0f, 0.0f, 0.0f), Vector3f(0.0f, 0.0f, 0.0f), 0.0f, 1.0f, -1.0f, 1.0f);

	//surface alpha
	float alpha = 0.0f;
	float alphaTimer = 0.0f;
	bool increasing = true;

	//Create a simple struct to hold lots of cube data
	struct LitCube
	{
		Vector3f position;
		Vector3f rotation;
		Vector3f scale;
		Matrix4x4 modelMatrix;
	};

	std::random_device rd;
	std::mt19937_64 gen(rd());
	std::uniform_real_distribution<> disPos(-100.0f, 50.0f);
	std::uniform_real_distribution<> disRot(0.0f, 360.0f);
	std::uniform_real_distribution<> disBinary(0.0f, 1.0f); //for binary decisions


	std::vector<LitCube> _litCubes;
	_litCubes.reserve(250);
	const float circleSize = 25.0f;
	for (int i = 0; i < 250; i++)
	{
		LitCube newCube;

		float x, y, z, xyPlaneDistance;
		//gen random points but keep them within bounds and adjust for spherical distribution
		do
		{
			x = (float)disPos(gen);
			y = (float)disPos(gen);
			z = 0.0f; //placeholder

			xyPlaneDistance = 26.0f; //init to value higher than 25 to ensure the loop runs

			// Keep the points above the terrain
			if (y > 0.0f)
				//continue;

			xyPlaneDistance = sqrtf(x * x + y * y);
			if (xyPlaneDistance > circleSize)
				continue; //don't place them outside the sphere

			float zSquared = circleSize * circleSize - xyPlaneDistance * xyPlaneDistance;
			if (zSquared < 0.0f)
				continue; //continue if no valid z is found

			z = sqrtf(zSquared); //get the positive z value
			z *= (disBinary(gen) < 0.5f) ? -1.0f : 1.0f; //randomly make it negative

			if (z > -8.0f)
				z = -8.0f; //keep the cubes in front of the camera by a ways
		} while (xyPlaneDistance > circleSize);

		newCube.position = Vector3f(x, y, z);
		newCube.rotation = Vector3f(disRot(gen), disRot(gen), disRot(gen));
		newCube.scale = Vector3f(1.0f, 1.0f, 1.0f);
		newCube.modelMatrix = createTransformationMatrix(newCube.position, newCube.rotation, newCube.scale);

		_litCubes.push_back(newCube);
	}

	// Allocate instance data buffer
	instanceDataBuffer = (InstanceData*)gpuAllocMap(
		_litCubes.size() * sizeof(InstanceData),
		SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW,
		SCE_GXM_MEMORY_ATTRIB_READ,
		&instanceDataBufferUID
	);

	//create a few lights
	Light firstLight = Light(Vector3f(0.0f, 2.0f, -7.0f), Color(1.0f, 0.0f, 0.0f, 1.0f));
	Light secondLight = Light(Vector3f(-4.0f, 2.0f, -7.0f), Color(0.0f, 1.0f, 0.0f, 1.0f));
	Light thirdLight = Light(Vector3f(4.0f, 2.0f, -7.0f), Color(0.0f, 0.0f, 1.0f, 1.0f));
	Light lights[3] = { firstLight, secondLight, thirdLight };
	
	//for moving the lights
	float lightOneAngle = 0.0f;
	float lightTwoAngle = 0.0f;
	float lightThreeAngle = 0.0f;
	//circle parameters
	const float lightOneRadius = 6.0f;
	const float lightTwoRadius = 6.0f;
	const float lightThreeRadius = 6.0f;
	const Vector3f lightOneCenter = { 0.0f, 2.0f, -7.0f };
	const Vector3f lightTwoCenter = { -4.0f, 2.0f, -7.0f };
	const Vector3f lightThreeCenter = { 4.0f, 2.0f, -7.0f };

	//verify the containers used for the lit cube uniform buffers
	unsigned int perFrameVertexContainer = sceGxmProgramParameterGetContainerIndex(gxmTexturedLitVertexProgram_u_viewMatrixParam);
	unsigned int perFrameFragmentContainer = sceGxmProgramParameterGetContainerIndex(gxmTexturedLitFragmentProgram_u_lightCountParam);
	unsigned int perFrameVertexInstancedContainer = sceGxmProgramParameterGetContainerIndex(gxmTexturedLitInstancedVertexProgram_u_viewMatrixParam);
	unsigned int perFrameTerrainVertexContainer = sceGxmProgramParameterGetContainerIndex(gxmTerrainVertexProgram_u_viewMatrixParam);
	unsigned int perFrameTerrainFragmentContainer = sceGxmProgramParameterGetContainerIndex(gxmTerrainFragmentProgram_u_lightCountParam);
	sceClibPrintf("Per-frame vertex container: %d\n", perFrameVertexContainer);
	sceClibPrintf("Per-frame fragment container: %d\n", perFrameFragmentContainer);
	sceClibPrintf("Size of PerFrameVertexUniformBuffer: %u bytes\n", sizeof(PerFrameVertexUniforms));
	sceClibPrintf("Size of PerFrameFragmentUniformBuffer: %u bytes\n", sizeof(PerFrameFragmentUniforms));
	sceClibPrintf("Per-frame terrain vertex container: %d\n", perFrameTerrainVertexContainer);
	sceClibPrintf("Per-frame terrain fragment container: %d\n", perFrameTerrainFragmentContainer);
	sceClibPrintf("Size of PerFrameTerrainVertexUniformBuffer: %u bytes\n", sizeof(PerFrameTerrainVertexUniforms));
	sceClibPrintf("Size of PerFrameTerrainFragmentUniformBuffer: %u bytes\n", sizeof(PerFrameTerrainFragmentUniforms));

	//Initialize the per-frame uniform buffers to avoid garbage data
	memset(perFrameVertexUniformBuffer, 0, sizeof(PerFrameVertexUniforms));
	memset(perFrameFragmentUniformBuffer, 0, sizeof(PerFrameFragmentUniforms));
	memset(perFrameTerrainVertexUniformBuffer, 0, sizeof(PerFrameTerrainVertexUniforms));
	memset(perFrameTerrainFragmentUniformBuffer, 0, sizeof(PerFrameTerrainFragmentUniforms));

	sceClibPrintf("Entering main loop...\n");
	bool running = true;
	while (running)
	{
		SceRtcTick currentTick;
		sceRtcGetCurrentTick(&currentTick);

		uint64_t deltaTicks = currentTick.tick - prevTick.tick;
		prevTick = currentTick;

		float deltaTime = (float)(deltaTicks * 1000 / tickRes);

		sceCtrlPeekBufferPositive(0, &ctrlData, 1);

		// Benchmark flythrough: L + R + Start triggers it
		if ((ctrlData.buttons & SCE_CTRL_LTRIGGER) &&
			(ctrlData.buttons & SCE_CTRL_RTRIGGER) &&
			(ctrlData.buttons & SCE_CTRL_START) &&
			!benchmarkState.active)
		{
			sceClibPrintf("=== BENCHMARK FLYTHROUGH STARTED ===\n");
			benchmarkInit(benchmarkState);
			camera.setPosition(Vector3f(0.0f, 1.0f, 0.0f));
			camera.setRotation(Vector3f(0.0f, 0.0f, 0.0f));
			lightOneAngle = 0.0f;
			lightTwoAngle = 0.0f;
			lightThreeAngle = 0.0f;
			colorCubeRotation = Vector3f(0.0f, 0.0f, 0.0f);
			texturedCubeRotation = Vector3f(0.0f, 0.0f, 0.0f);
			alphaCubeRotation = Vector3f(0.0f, 0.0f, 0.0f);
			alpha = 0.0f;
			alphaTimer = 0.0f;
			increasing = true;
		}

		if (benchmarkState.active)
		{
			Vector3f benchPos, benchRot;
			if (!benchmarkUpdate(benchmarkState, deltaTime, benchPos, benchRot))
			{
				benchmarkWriteLog(benchmarkState);
			}
			camera.setPosition(benchPos);
			camera.setRotation(benchRot);
			cameraPosition = benchPos;
		}
		else
		{
			if (ctrlData.buttons & SCE_CTRL_START)
			{
				running = false;
			}
			if (ctrlData.buttons & SCE_CTRL_TRIANGLE)
			{
				wireFrame = !wireFrame;

				if (wireFrame)
				{
					sceGxmSetFrontPolygonMode(gxmContext, SCE_GXM_POLYGON_MODE_TRIANGLE_LINE);
					sceGxmSetBackPolygonMode(gxmContext, SCE_GXM_POLYGON_MODE_TRIANGLE_LINE);
				}
				else
				{
					sceGxmSetFrontPolygonMode(gxmContext, SCE_GXM_POLYGON_MODE_TRIANGLE_FILL);
					sceGxmSetBackPolygonMode(gxmContext, SCE_GXM_POLYGON_MODE_TRIANGLE_FILL);
				}
			}

			double rx = (ctrlData.rx - 128.0) / 128.0;
			double ry = (ctrlData.ry - 128.0) / 128.0;
			double lx = (ctrlData.lx - 128.0) / 128.0;
			double ly = (ctrlData.ly - 128.0) / 128.0;
			const double deadzone = 0.25;
			const double sensitivity = 0.005;

			//Update the camera
			Vector3f cameraMove = { 0.0f, 0.0f, 0.0f };
			Vector3f cameraRot = { 0.0f, 0.0f, 0.0f };

			if (abs(rx) < deadzone)
			{
				rx = 0.0;
			}
			else
			{
				cameraRot -= Vector3f(0.0f, (float)rx, 0.0f);
			}

			if (abs(ry) < deadzone)
			{
				ry = 0.0;
			}
			else
			{
				cameraRot -= Vector3f((float)ry, 0.0f, 0.0f);
			}

			if (abs(lx) < deadzone)
			{
				lx = 0.0;
			}
			else
			{
				cameraMove += camera.getRightVector() * lx;
			}

			if (abs(ly) < deadzone)
			{
				ly = 0.0;
			}
			else
			{
				cameraMove -= camera.getForwardVector() * ly;
			}

			camera.varyPosition(cameraMove * sensitivity * deltaTime);
			camera.varyRotation(cameraRot * sensitivity * deltaTime);
			cameraPosition = camera.getPosition();
		}
		

		if (!benchmarkState.active)
		{
			if (increasing)
			{
				if (alphaTimer <= 4.0f)
				{
					alphaTimer += 0.001f * deltaTime;
				}
				else
				{
					alphaTimer = 4.0f;
					alpha += 0.002f * deltaTime;
				}
				if (alpha > 1.0f)
				{
					alpha = 1.0f;
					increasing = false;
					alphaTimer = 0.0f;
				}
			}
			else
			{
				alpha -= 0.004f * deltaTime;
				if (alpha < 0.0f)
				{
					alpha = 0.0f;
					increasing = true;
				}
			}
		}

		//update cube rotation
		colorCubeRotation.x += 0.0036f * deltaTime;
		colorCubeRotation.y += 0.0050f * deltaTime;
		colorCubeRotation.z += 0.001f * deltaTime;

		texturedCubeRotation.x = 1.0f - colorCubeRotation.x;
		texturedCubeRotation.y = 1.0f - colorCubeRotation.y;
		texturedCubeRotation.z = 1.0f - colorCubeRotation.z;

		alphaCubeRotation.x = colorCubeRotation.x / 2;
		alphaCubeRotation.y = colorCubeRotation.y / 2;
		alphaCubeRotation.z = texturedCubeRotation.z / 2;

		//cubeModelMatrix.rotate(Vector3f(cubeRotation.x, cubeRotation.y, cubeRotation.z));
		colorCubeModelMatrix = createTransformationMatrix(colorCubePosition, colorCubeRotation, Vector3f{ 1.0f, 1.0f, 1.0f });
		texturedCubeModelMatrix = createTransformationMatrix(texturedCubePosition, texturedCubeRotation, Vector3f{ 1.0f, 1.0f, 1.0f });
		alphaCubeModelMatrix = createTransformationMatrix(alphaCubePosition, alphaCubeRotation, Vector3f{ 0.5f, 0.5f, 0.5f });
		//create the surface transformation matrix by multiplying the model/view/projection matrices
		surfaceTransformationMatrix = surfaceTransformationMatrix * orthoCam.getViewMatrix() * orthoCam.getProjectionMatrix();

		//move the lights
		lightOneAngle += 0.003f * deltaTime;
		lightTwoAngle += 0.002f * deltaTime;
		lightThreeAngle += 0.004f * deltaTime;

		//keep angles in range to avoid floating point issues over time
		if (lightOneAngle > 6.28318530718f)
			lightOneAngle -= 6.28318530718f;
		if (lightTwoAngle > 6.28318530718f)
			lightTwoAngle -= 6.28318530718f;
		if (lightThreeAngle > 6.28318530718f)
			lightThreeAngle -= 6.28318530718f;

		lights[0].setPosition(Vector3f(
			lightOneCenter.x + lightOneRadius * cosf(lightOneAngle),
			lightOneCenter.y,
			lightOneCenter.z + lightOneRadius * sinf(lightOneAngle)));

		lights[1].setPosition(Vector3f(
			lightTwoCenter.x + lightTwoRadius * cosf(lightTwoAngle),
			lightTwoCenter.y,
			lightTwoCenter.z + lightTwoRadius * sinf(lightTwoAngle)));

		lights[2].setPosition(Vector3f(
			lightThreeCenter.x + lightThreeRadius * cosf(lightThreeAngle),
			lightThreeCenter.y,
			lightThreeCenter.z + lightThreeRadius * sinf(lightThreeAngle)));

		// Update terrain LODs
		terrain.updateLODs(cameraPosition, camera.getForwardVector());

		// Get view-projection matrix for frustum culling (only used on terrain for now)
		// incorporate the terrain's model transform into the cull test
		Matrix4x4 viewProjMatrix = camera.getProjectionMatrix() * camera.getViewMatrix() * terrain.getModelMatrix();

		// Get visible terrain chunks
		std::vector<TerrainChunk*> visibleChunks = terrain.getVisibleChunks(viewProjMatrix);

		// Sort front-to-back for better HSR on SGX543
		std::sort(visibleChunks.begin(), visibleChunks.end(),
			[&cameraPosition](const TerrainChunk* a, const TerrainChunk* b) {
				Vector3f da = a->getCenter() - cameraPosition;
				Vector3f db = b->getCenter() - cameraPosition;
				return (da.x*da.x + da.y*da.y + da.z*da.z) < (db.x*db.x + db.y*db.y + db.z*db.z);
			});

		clearScreen();

		//render

		//terrain first
		sceGxmSetVertexProgram(gxmContext, gxmTerrainVertexProgramPatched);
		sceGxmSetFragmentProgram(gxmContext, gxmTerrainFragmentProgramPatched);

		void* terrainVertexDefaultBuffer;
		sceGxmReserveVertexDefaultUniformBuffer(gxmContext, &terrainVertexDefaultBuffer);
		sceGxmSetUniformDataF(terrainVertexDefaultBuffer, gxmTerrainVertexProgram_u_modelMatrixParam, 0, 16, (float*)terrain.getModelMatrix().getData());

		void* terrainFragmentDefaultBuffer;
		sceGxmReserveFragmentDefaultUniformBuffer(gxmContext, &terrainFragmentDefaultBuffer);
		//Default F0 for non-metallic materials
		float F0[3] = { 0.04f, 0.04f, 0.04f };
		sceGxmSetUniformDataF(terrainFragmentDefaultBuffer, gxmTerrainFragmentProgram_u_F0Param, 0, 3, F0);

		//populate per-frame uniform data
		memcpy(perFrameTerrainVertexUniformBuffer->viewMatrix, camera.getViewMatrix().getData(), sizeof(float) * 16);
		memcpy(perFrameTerrainVertexUniformBuffer->projectionMatrix, camera.getProjectionMatrix().getData(), sizeof(float) * 16);
		perFrameTerrainVertexUniformBuffer->cameraPosition[0] = cameraPosition.x;
		perFrameTerrainVertexUniformBuffer->cameraPosition[1] = cameraPosition.y;
		perFrameTerrainVertexUniformBuffer->cameraPosition[2] = cameraPosition.z;

		perFrameTerrainFragmentUniformBuffer->lightCount = 3;
		for (int i = 0; i < 3; i++)
		{
			perFrameTerrainFragmentUniformBuffer->lightPositions[i][0] = lights[i].getPosition().x;
			perFrameTerrainFragmentUniformBuffer->lightPositions[i][1] = lights[i].getPosition().y;
			perFrameTerrainFragmentUniformBuffer->lightPositions[i][2] = lights[i].getPosition().z;
			perFrameTerrainFragmentUniformBuffer->lightPositions[i][3] = 1.0f; //padding
			perFrameTerrainFragmentUniformBuffer->lightColors[i][0] = lights[i].getColor().r;
			perFrameTerrainFragmentUniformBuffer->lightColors[i][1] = lights[i].getColor().g;
			perFrameTerrainFragmentUniformBuffer->lightColors[i][2] = lights[i].getColor().b;
			perFrameTerrainFragmentUniformBuffer->lightColors[i][3] = 1.0f; //alpha / padding

			perFrameTerrainFragmentUniformBuffer->lightPowers[i] = lights[i].getPower();
			perFrameTerrainFragmentUniformBuffer->lightRadii[i] = lights[i].getRadius();
		}

		//fill in the rest of the 8 lights with all black and 0 power
		for (int i = 3; i < 8; i++)
		{
			perFrameTerrainFragmentUniformBuffer->lightPositions[i][0] = 0.0f;
			perFrameTerrainFragmentUniformBuffer->lightPositions[i][1] = 0.0f;
			perFrameTerrainFragmentUniformBuffer->lightPositions[i][2] = 0.0f;
			perFrameTerrainFragmentUniformBuffer->lightPositions[i][3] = 1.0f;
			perFrameTerrainFragmentUniformBuffer->lightColors[i][0] = 0.0f;
			perFrameTerrainFragmentUniformBuffer->lightColors[i][1] = 0.0f;
			perFrameTerrainFragmentUniformBuffer->lightColors[i][2] = 0.0f;
			perFrameTerrainFragmentUniformBuffer->lightColors[i][3] = 1.0f;
			perFrameTerrainFragmentUniformBuffer->lightPowers[i] = 0.0f;
			perFrameTerrainFragmentUniformBuffer->lightRadii[i] = 0.0f;
		}

		//bind the per-frame uniform buffer
		sceGxmSetVertexUniformBuffer(gxmContext, perFrameTerrainVertexContainer, perFrameTerrainVertexUniformBuffer);
		sceGxmSetFragmentUniformBuffer(gxmContext, perFrameTerrainFragmentContainer, perFrameTerrainFragmentUniformBuffer);

		sceGxmSetFragmentTexture(gxmContext, 0, terrainDiffuseTex.getTexture());
		sceGxmSetFragmentTexture(gxmContext, 1, terrainNormalTex.getTexture());
		sceGxmSetFragmentTexture(gxmContext, 2, terrainRoughTex.getTexture());

		//Get buffer pool base address (terrain meshes are pooled into common ALIGN'ed chunks)
		void* vertexPoolBase = terrain.getBufferPool()->getVertexPoolBase();
		void* indexPoolBase = terrain.getBufferPool()->getIndexPoolBase();

		// Render the visible chunks
		int renderedChunks = 0;
		for (TerrainChunk* chunk : visibleChunks)
		{
			/*
			// Find the chunk index (For GPU data lookup)
			int chunkX = 0, chunkZ = 0;

			int chunkIndex = 0;
			for (const auto& c : terrain.getChunks())
			{
				if (c.get() == chunk) break;
				chunkIndex++;
			}

			ChunkGPUData& gpuData = chunkGPUData[chunkIndex];
			TerrainChunk::LODLevel currentLOD = chunk->getCurrentLOD();
			TerrainChunk::LODMesh* lodMesh = chunk->getCurrentLODMesh();
			*/

			// Use the pool instead of the above

			const TerrainChunk::LODMesh* lodMesh = chunk->getCurrentLODMesh();

			//Calculate acutal GPU addresses from pool + offset
			void* vertexData = (uint8_t*)vertexPoolBase + lodMesh->vertexAlloc.offset;
			void* indexData = (uint8_t*)indexPoolBase + lodMesh->indexAlloc.offset;

			//Bind and render
			//sceGxmSetVertexStream(gxmContext, 0, gpuData.vertexPtrs[currentLOD]);
			sceGxmSetVertexStream(gxmContext, 0, vertexData);
			//sceGxmDraw(gxmContext, SCE_GXM_PRIMITIVE_TRIANGLES, SCE_GXM_INDEX_FORMAT_U16, 
			//	gpuData.indexPtrs[currentLOD], lodMesh->indexCount);
			sceGxmDraw(gxmContext, SCE_GXM_PRIMITIVE_TRIANGLES, SCE_GXM_INDEX_FORMAT_U16, indexData, lodMesh->indexCount);

			renderedChunks++;
		}

		//sceClibPrintf("Rendered %d/%d chunks\n", renderedChunks, Terrain::CHUNKS_PER_SIDE * Terrain::CHUNKS_PER_SIDE);

		//then basic cubes
		sceGxmSetVertexProgram(gxmContext, gxmBasicVertexProgramPatched);
		sceGxmSetFragmentProgram(gxmContext, gxmBasicFragmentProgramPatched);

		void* basicVertexBufferA;
		sceGxmReserveVertexDefaultUniformBuffer(gxmContext, &basicVertexBufferA);
		sceGxmSetUniformDataF(basicVertexBufferA, gxmBasicVertexProgram_u_modelMatrixParam, 0, 16, (float*)colorCubeModelMatrix.getData());

		void* basicVertexBufferB;
		sceGxmReserveVertexDefaultUniformBuffer(gxmContext, &basicVertexBufferB);
		sceGxmSetUniformDataF(basicVertexBufferB, gxmBasicVertexProgram_u_viewMatrixParam, 0, 16, (float*)camera.getViewMatrix().getData());

		void* basicVertexBufferC;
		sceGxmReserveVertexDefaultUniformBuffer(gxmContext, &basicVertexBufferC);
		sceGxmSetUniformDataF(basicVertexBufferC, gxmBasicVertexProgram_u_projectionMatrixParam, 0, 16, (float*)camera.getProjectionMatrix().getData());

		sceGxmSetVertexStream(gxmContext, 0, cVertexData);
		sceGxmDraw(gxmContext, SCE_GXM_PRIMITIVE_TRIANGLES, SCE_GXM_INDEX_FORMAT_U16, indexData, 36);

		// render lit textured cubes
		sceGxmSetVertexProgram(gxmContext, gxmTexturedLitInstancedVertexProgramPatched);
		sceGxmSetFragmentProgram(gxmContext, gxmTexturedLitFragmentProgramPatched);

		//populate per-frame uniform data
		memcpy(perFrameVertexUniformBuffer->viewMatrix, camera.getViewMatrix().getData(), sizeof(float) * 16);
		memcpy(perFrameVertexUniformBuffer->projectionMatrix, camera.getProjectionMatrix().getData(), sizeof(float) * 16);
		// Update instance data buffer with model matrices
		for (int i = 0; i < _litCubes.size(); i++) 
		{
			memcpy(instanceDataBuffer[i].modelMatrix,
				_litCubes[i].modelMatrix.getData(),
				sizeof(float) * 16);
		}

		perFrameFragmentUniformBuffer->lightCount = 3;
		for (int i = 0; i < 3; i++)
		{
			perFrameFragmentUniformBuffer->lightPositions[i][0] = lights[i].getPosition().x;
			perFrameFragmentUniformBuffer->lightPositions[i][1] = lights[i].getPosition().y;
			perFrameFragmentUniformBuffer->lightPositions[i][2] = lights[i].getPosition().z;
			perFrameFragmentUniformBuffer->lightPositions[i][3] = 1.0f; //padding
			perFrameFragmentUniformBuffer->lightColors[i][0] = lights[i].getColor().r;
			perFrameFragmentUniformBuffer->lightColors[i][1] = lights[i].getColor().g;
			perFrameFragmentUniformBuffer->lightColors[i][2] = lights[i].getColor().b;
			perFrameFragmentUniformBuffer->lightColors[i][3] = 1.0f; //alpha / padding

			perFrameFragmentUniformBuffer->lightPowers[i] = lights[i].getPower();
			perFrameFragmentUniformBuffer->lightRadii[i] = lights[i].getRadius();
		}

		//fill in the rest of the 8 lights with all black and 0 power
		for (int i = 3; i < 8; i++)
		{
			perFrameFragmentUniformBuffer->lightPositions[i][0] = 0.0f;
			perFrameFragmentUniformBuffer->lightPositions[i][1] = 0.0f;
			perFrameFragmentUniformBuffer->lightPositions[i][2] = 0.0f;
			perFrameFragmentUniformBuffer->lightPositions[i][3] = 1.0f;
			perFrameFragmentUniformBuffer->lightColors[i][0] = 0.0f;
			perFrameFragmentUniformBuffer->lightColors[i][1] = 0.0f;
			perFrameFragmentUniformBuffer->lightColors[i][2] = 0.0f;
			perFrameFragmentUniformBuffer->lightColors[i][3] = 1.0f;
			perFrameFragmentUniformBuffer->lightPowers[i] = 0.0f;
			perFrameFragmentUniformBuffer->lightRadii[i] = 0.0f;
		}

		perFrameFragmentUniformBuffer->cameraPosition[0] = cameraPosition.x;
		perFrameFragmentUniformBuffer->cameraPosition[1] = cameraPosition.y;
		perFrameFragmentUniformBuffer->cameraPosition[2] = cameraPosition.z;
		
		//bind the per-frame uniform buffer (container 0 from BUFFER[0] in the shader)
		sceGxmSetVertexUniformBuffer(gxmContext, perFrameVertexInstancedContainer, perFrameVertexUniformBuffer);
		sceGxmSetFragmentUniformBuffer(gxmContext, perFrameFragmentContainer, perFrameFragmentUniformBuffer);

		//set texture
		sceGxmSetFragmentTexture(gxmContext, 0, &allWhiteTexture);

		//set vertex streams
		sceGxmSetVertexStream(gxmContext, 0, ltVertexData);
		sceGxmSetVertexStream(gxmContext, 1, instanceDataBuffer);

		//draw the cubes
		// TO DO: Change to instanced rendering
		//for (int i = 0; i < _litCubes.size(); i++)
		//{
		//	LitCube& cube = _litCubes[i];

		//	void* litCubeDefaultVUniformBuffer;
		//	sceGxmReserveVertexDefaultUniformBuffer(gxmContext, &litCubeDefaultVUniformBuffer);

		//	sceGxmSetUniformDataF(litCubeDefaultVUniformBuffer, gxmTexturedLitVertexProgram_u_modelMatrixParam, 0, 16, (float*)cube.modelMatrix.getData());

		//	sceGxmDraw(gxmContext, SCE_GXM_PRIMITIVE_TRIANGLES, SCE_GXM_INDEX_FORMAT_U16, texturedIndexData, 36);
		//}

		sceGxmDrawInstanced(gxmContext, 
			SCE_GXM_PRIMITIVE_TRIANGLES, 
			SCE_GXM_INDEX_FORMAT_U16, 
			texturedIndexData, // Index buffer for one cube
			36 * _litCubes.size(), // Total number of indices to render
			36); // Index wrap count (restart after 36 indices, i.e. one cube)

		// render textured cube
		sceGxmSetVertexProgram(gxmContext, gxmTexturedVertexProgramPatched);
		sceGxmSetFragmentProgram(gxmContext, gxmTexturedFragmentProgramPatched);

		void* texturedVertexBufferA;
		sceGxmReserveVertexDefaultUniformBuffer(gxmContext, &texturedVertexBufferA);
		sceGxmSetUniformDataF(texturedVertexBufferA, gxmTexturedVertexProgram_u_modelMatrixParam, 0, 16, (float*)texturedCubeModelMatrix.getData());

		void* texturedVertexBufferB;
		sceGxmReserveVertexDefaultUniformBuffer(gxmContext, &texturedVertexBufferB);
		sceGxmSetUniformDataF(texturedVertexBufferB, gxmTexturedVertexProgram_u_viewMatrixParam, 0, 16, (float*)camera.getViewMatrix().getData());

		void* texturedVertexBufferC;
		sceGxmReserveVertexDefaultUniformBuffer(gxmContext, &texturedVertexBufferC);
		sceGxmSetUniformDataF(texturedVertexBufferC, gxmTexturedVertexProgram_u_projectionMatrixParam, 0, 16, (float*)camera.getProjectionMatrix().getData());

		sceGxmSetFragmentTexture(gxmContext, 0, &texture);
		sceGxmSetVertexStream(gxmContext, 0, tVertexData);
		sceGxmDraw(gxmContext, SCE_GXM_PRIMITIVE_TRIANGLES, SCE_GXM_INDEX_FORMAT_U16, texturedIndexData, 36);

		// render alpha cube

		//Disable backface culling and depth writes
		sceGxmSetTwoSidedEnable(gxmContext, SCE_GXM_TWO_SIDED_ENABLED);
		sceGxmSetFrontDepthWriteEnable(gxmContext, SCE_GXM_DEPTH_WRITE_DISABLED);
		sceGxmSetBackDepthWriteEnable(gxmContext, SCE_GXM_DEPTH_WRITE_DISABLED);

		// First pass, render the back faces of the cube
		sceGxmSetCullMode(gxmContext, SCE_GXM_CULL_CCW);

		// Reserve new uniforms for the alpha cube draw call:
		void* alphaVertexBufferA;
		sceGxmReserveVertexDefaultUniformBuffer(gxmContext, &alphaVertexBufferA);
		sceGxmSetUniformDataF(alphaVertexBufferA, gxmTexturedVertexProgram_u_modelMatrixParam, 0, 16, (float*)alphaCubeModelMatrix.getData());

		void* alphaVertexBufferB;
		sceGxmReserveVertexDefaultUniformBuffer(gxmContext, &alphaVertexBufferB);
		sceGxmSetUniformDataF(alphaVertexBufferB, gxmTexturedVertexProgram_u_viewMatrixParam, 0, 16, (float*)camera.getViewMatrix().getData());

		void* alphaVertexBufferC;
		sceGxmReserveVertexDefaultUniformBuffer(gxmContext, &alphaVertexBufferC);
		sceGxmSetUniformDataF(alphaVertexBufferC, gxmTexturedVertexProgram_u_projectionMatrixParam, 0, 16, (float*)camera.getProjectionMatrix().getData());

		// Reuse the same texture and vertex stream
		sceGxmSetFragmentTexture(gxmContext, 0, &alphaTexture);
		sceGxmSetVertexStream(gxmContext, 0, tVertexData);

		sceGxmDraw(gxmContext, SCE_GXM_PRIMITIVE_TRIANGLES, SCE_GXM_INDEX_FORMAT_U16, texturedIndexData, 36);

		// Second pass, render the front faces of the cube
		sceGxmSetCullMode(gxmContext, SCE_GXM_CULL_CW);

		// Reuse the same uniforms and texture
		sceGxmDraw(gxmContext, SCE_GXM_PRIMITIVE_TRIANGLES, SCE_GXM_INDEX_FORMAT_U16, texturedIndexData, 36);

		// Re-enable backface culling and depth writes
		sceGxmSetTwoSidedEnable(gxmContext, SCE_GXM_TWO_SIDED_DISABLED);
		sceGxmSetFrontDepthWriteEnable(gxmContext, SCE_GXM_DEPTH_WRITE_ENABLED);
		sceGxmSetBackDepthWriteEnable(gxmContext, SCE_GXM_DEPTH_WRITE_ENABLED);

		// render surface
		sceGxmSetVertexProgram(gxmContext, gxmTexturedScreenLiteralVertexProgramPatched);
		sceGxmSetFragmentProgram(gxmContext, gxmTexturedScreenLiteralFragmentProgramPatched);

		void* surfaceVertexBufferA;
		sceGxmReserveVertexDefaultUniformBuffer(gxmContext, &surfaceVertexBufferA);
		sceGxmSetUniformDataF(surfaceVertexBufferA, gxmTexturedScreenLiteralVertexProgram_u_alphaParam, 0, 1, &alpha);

		void* surfaceVertexBufferB;
		sceGxmReserveVertexDefaultUniformBuffer(gxmContext, &surfaceVertexBufferB);
		sceGxmSetUniformDataF(surfaceVertexBufferB, gxmTexturedScreenLiteralVertexProgram_u_transformParam, 0, 16, (float*)surfaceTransformationMatrix.getData());

		sceGxmSetFragmentTexture(gxmContext, 0, &texture);
		sceGxmSetVertexStream(gxmContext, 0, sVertexData);

		sceGxmDraw(gxmContext, SCE_GXM_PRIMITIVE_TRIANGLES, SCE_GXM_INDEX_FORMAT_U32, surfaceIndexData, 6);

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
	gpuFreeUnmap(colorCubeVertexDataUID);
	gpuFreeUnmap(texturedCubeVertexDataUID);
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