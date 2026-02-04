#include "texture.h"
#include "memory.h"
#include <psp2/kernel/clib.h>
#include <math.h>
#include <string.h> // For memcpy

Texture::Texture()
	: texType(SCE_GXM_TEXTURE_LINEAR),
	texFormat(SCE_GXM_TEXTURE_FORMAT_U8U8U8U8_ABGR),
	width(0),
	height(0),
	minFilter(SCE_GXM_TEXTURE_FILTER_LINEAR),
	magFilter(SCE_GXM_TEXTURE_FILTER_LINEAR),
	uAddrMode(SCE_GXM_TEXTURE_ADDR_REPEAT),
	vAddrMode(SCE_GXM_TEXTURE_ADDR_REPEAT),
	mipCount(0),
	lodBias(0),
	mipFiltering(false),
	memUid(-1),
	memAddr(nullptr)
{

}

Texture::~Texture()
{
	release();
}

void Texture::bindMemory(void* addr, SceUID uid)
{
    memAddr = addr;
    memUid = uid;
}

bool Texture::loadFromData(const unsigned char* base, int comp, bool isNormalMap)
{
    //calculate mipmap count
    int w = width;
    int h = height;
    unsigned int mips = 1;

    while (w > 1 || h > 1)
    {
        if (w > 1) w /= 2;
        if (h > 1) h /= 2;
        mips++;
    }

    //Calculate total size with proper alignment
    size_t dataSize = calculateTextureDataSize(width, height, comp, mips);

    sceClibPrintf("Texture::loadFromData - Size: %dx%d, Components: %d, Mips: %u, Total size: %u bytes\n",
        width, height, comp, mips, (unsigned)dataSize);

    SceUID uid;
    void* addr = gpuAllocMap(dataSize,
        SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW,
        SCE_GXM_MEMORY_ATTRIB_READ,
        &uid);
    if (!addr)
    {
        sceClibPrintf("ERROR: Texture::loadFromData() failed to allocate GPU memory\n");
        return false;
    }

    generateMipmaps((unsigned char*)addr, base, width, height, comp, mips, isNormalMap);

    bindMemory(addr, uid);
    setMipCount(mips);
    return init() == SCE_OK;
}

bool Texture::loadFromDataExpanded(const unsigned char* base, int srcComp, int dstComp, unsigned char fillValue, bool isNormalMap)
{
    if (dstComp <= srcComp)
    {
        return loadFromData(base, srcComp, isNormalMap);
    }

    // Expand source data to destination component count
    size_t pixelCount = (size_t)width * height;
    unsigned char* expanded = (unsigned char*)malloc(pixelCount * dstComp);
    if (!expanded) return false;

    for (size_t i = 0; i < pixelCount; i++)
    {
        for (int c = 0; c < srcComp; c++)
            expanded[i * dstComp + c] = base[i * srcComp + c];
        for (int c = srcComp; c < dstComp; c++)
            expanded[i * dstComp + c] = fillValue;
    }

    bool result = loadFromData(expanded, dstComp, isNormalMap);
    free(expanded);
    return result;
}

int32_t Texture::init()
{
	if (memAddr == nullptr)
	{
		sceClibPrintf("ERROR: Texture init() called before allocation! memAddr = nullptr\n");
		return -1;
	}

	int32_t result = SCE_OK;

	switch (texType)
	{
	case SceGxmTextureType::SCE_GXM_TEXTURE_LINEAR:
        if (mipCount == 0)
        {
            sceClibPrintf("WARNING: Texture has 0 mipmaps, setting to 1\n");
            mipCount = 1;
        }

        result = sceGxmTextureInitLinear(&texture, memAddr,
            texFormat,
            width,
            height,
            mipCount - 1);
		break;

	case SceGxmTextureType::SCE_GXM_TEXTURE_SWIZZLED:
        if (mipCount == 0)
        {
            sceClibPrintf("WARNING: Texture has 0 mipmaps, setting to 1\n");
            mipCount = 1;
        }

        result = sceGxmTextureInitSwizzled(&texture, memAddr,
            texFormat,
            width,
            height,
            mipCount - 1);
        sceClibPrintf("sceGxmTextureInitSwizzled(): 0x%08X\n", result);
		break;

	default:
		sceClibPrintf("ERROR: Texture::init() cannot initialize texture of unknown type: %d\n", texType);
		return -2;
	}

	if (result != SCE_OK)
	{
		sceClibPrintf("ERROR: Texture::init() failed during sceGxmTextureInitLinear: %d\n", result);
		return result;
	}

	sceGxmTextureSetMinFilter(&texture, minFilter);
	sceGxmTextureSetMagFilter(&texture, magFilter);
	sceGxmTextureSetMipFilter(&texture,
		mipFiltering ? SCE_GXM_TEXTURE_MIP_FILTER_ENABLED : SCE_GXM_TEXTURE_MIP_FILTER_DISABLED);

	sceGxmTextureSetUAddrMode(&texture, uAddrMode);
	sceGxmTextureSetVAddrMode(&texture, vAddrMode);

	sceGxmTextureSetMipmapCount(&texture, mipCount);
	sceGxmTextureSetLodBias(&texture, lodBias);

	return SCE_OK;
}

void Texture::release()
{
	if (memAddr && memUid >= 0)
	{
		gpuFreeUnmap(memUid);
		memAddr = nullptr;
		memUid = -1;
	}
}

uint32_t Texture::getWidth() const
{
	return width;
}

uint32_t Texture::getHeight() const{
	return height; 
}

const SceGxmTexture* Texture::getTexture() const 
{
	return &texture; 
}

void Texture::setTextureType(SceGxmTextureType t) 
{ 
	texType = t; 
}

void Texture::setFormat(SceGxmTextureFormat f) 
{ 
	texFormat = f; 
}

void Texture::setSize(uint32_t w, uint32_t h) 
{
	width = w; height = h;
}

void Texture::setFilters(SceGxmTextureFilter minF, SceGxmTextureFilter magF, bool enableMipFilters)
{
	minFilter = minF;
	magFilter = magF;
	mipFiltering = enableMipFilters;
}

void Texture::setAddressModes(SceGxmTextureAddrMode uMode, SceGxmTextureAddrMode vMode)
{
	uAddrMode = uMode;
	vAddrMode = vMode;
}

void Texture::setMipCount(uint32_t count) 
{ 
	mipCount = count;
}

void Texture::setLodBias(uint32_t bias) 
{ 
	lodBias = bias;
}

size_t Texture::calculateTextureDataSize(int width, int height, int comp, unsigned int mipCount)
{
    size_t totalSize = 0;
    int w = width;
    int h = height;

    for (unsigned int level = 0; level < mipCount; ++level)
    {
        size_t levelSize = (size_t)w * h * comp;
        totalSize += ALIGN(levelSize, TEXTURE_ALIGNMENT);

        if (w > 1) w /= 2;
        if (h > 1) h /= 2;
    }

    return totalSize;
}

void Texture::generateMipmaps(unsigned char* gpuMemory, const unsigned char* base,
    int width, int height, int comp,
    unsigned int mipCount,
    bool isNormalMap)
{
    //Copy base level to GPU memory
    size_t baseSize = (size_t)width * height * comp;
    sceClibMemcpy(gpuMemory, base, baseSize);

    //track offset for each mip level
    size_t offset = ALIGN(baseSize, TEXTURE_ALIGNMENT);

    int w = width;
    int h = height;
    unsigned char* prevLevel = gpuMemory;

    sceClibPrintf("Generating MipMaps...\n");

    // Generate each mip level
    for (unsigned int level = 1; level < mipCount; ++level)
    {
        int prevW = w;
        int prevH = h;
        if (w > 1) w /= 2;
        if (h > 1) h /= 2;

        unsigned char* currentLevel = gpuMemory + offset;

        if (!isNormalMap)
        {
            for (int y = 0; y < h; ++y)
            {
                for (int x = 0; x < w; ++x)
                {
                    float nx = 0.0f, ny = 0.0f, nz = 0.0f;
                    int sampleCount = 0;

                    for (int c = 0; c < comp; ++c)
                    {
                        // Calculate source indices with bounds checking
                        int srcX0 = x * 2;
                        int srcY0 = y * 2;
                        int srcX1 = (srcX0 + 1 < prevW) ? srcX0 + 1 : srcX0;
                        int srcY1 = (srcY0 + 1 < prevH) ? srcY0 + 1 : srcY0;

                        int idx0 = (srcY0 * prevW + srcX0) * comp + c;
                        int idx1 = (srcY0 * prevW + srcX1) * comp + c;
                        int idx2 = (srcY1 * prevW + srcX0) * comp + c;
                        int idx3 = (srcY1 * prevW + srcX1) * comp + c;

                        unsigned int sum = prevLevel[idx0] + prevLevel[idx1] +
                            prevLevel[idx2] + prevLevel[idx3];
                        currentLevel[(y * w + x) * comp + c] = (unsigned char)(sum >> 2);
                    }
                }
            }
        }
        else // Normal maps need special handling
        {
            for (int y = 0; y < h; ++y)
            {
                for (int x = 0; x < w; ++x)
                {
                    float nx = 0.0f, ny = 0.0f, nz = 0.0f;
                    int sampleCount = 0;

                    // Sample 2x2 block from previous level
                    for (int sy = 0; sy < 2; ++sy)
                    {
                        for (int sx = 0; sx < 2; ++sx)
                        {
                            int srcX = x * 2 + sx;
                            int srcY = y * 2 + sy;

                            if (srcX < prevW && srcY < prevH)
                            {
                                int idx = (srcY * prevW + srcX) * comp;

                                // Decode normal from [0,255] to [-1,1]
                                float px = (prevLevel[idx + 0] / 255.0f) * 2.0f - 1.0f;
                                float py = (prevLevel[idx + 1] / 255.0f) * 2.0f - 1.0f;
                                float pz = comp >= 3 ? (prevLevel[idx + 2] / 255.0f) * 2.0f - 1.0f : 0.0f;

                                // Reconstruct Z if only 2 components
                                if (comp == 2)
                                {
                                    float lenSq = px * px + py * py;
                                    pz = lenSq <= 1.0f ? sqrtf(1.0f - lenSq) : 0.0f;
                                }

                                nx += px;
                                ny += py;
                                nz += pz;
                                sampleCount++;
                            }
                        }
                    }

                    // Average and normalize
                    if (sampleCount > 0)
                    {
                        nx /= sampleCount;
                        ny /= sampleCount;
                        nz /= sampleCount;

                        float len = sqrtf(nx * nx + ny * ny + nz * nz);
                        if (len > 0.0001f)
                        {
                            nx /= len;
                            ny /= len;
                            nz /= len;
                        }
                        else
                        {
                            // Default to up vector if degenerate
                            nx = 0.0f;
                            ny = 0.0f;
                            nz = 1.0f;
                        }
                    }

                    // Encode back to [0,255]
                    int dstIdx = (y * w + x) * comp;
                    currentLevel[dstIdx + 0] = (unsigned char)((nx * 0.5f + 0.5f) * 255.0f + 0.5f);
                    currentLevel[dstIdx + 1] = (unsigned char)((ny * 0.5f + 0.5f) * 255.0f + 0.5f);
                    if (comp >= 3)
                    {
                        currentLevel[dstIdx + 2] = (unsigned char)((nz * 0.5f + 0.5f) * 255.0f + 0.5f);
                    }

                    // Copy alpha channel if present
                    if (comp == 4)
                    {
                        // Average alpha values
                        unsigned int alphaSum = 0;
                        int alphaCount = 0;
                        for (int sy = 0; sy < 2; ++sy)
                        {
                            for (int sx = 0; sx < 2; ++sx)
                            {
                                int srcX = x * 2 + sx;
                                int srcY = y * 2 + sy;
                                if (srcX < prevW && srcY < prevH)
                                {
                                    alphaSum += prevLevel[(srcY * prevW + srcX) * comp + 3];
                                    alphaCount++;
                                }
                            }
                        }
                        currentLevel[dstIdx + 3] = alphaCount > 0 ? (unsigned char)(alphaSum / alphaCount) : 255;
                    }
                }
            }
        }
        prevLevel = currentLevel;
        size_t levelSize = (size_t)w * h * comp;
        offset += ALIGN(levelSize, TEXTURE_ALIGNMENT);
    }
}