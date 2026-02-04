#pragma once

#include <psp2/gxm.h>

class Texture
{
public:
	//TO DO: finish constructors
	Texture();
	~Texture();

	void bindMemory(void* addr, SceUID uid);
	bool loadFromData(const unsigned char* base, int comp, bool isNormalMap = false);
	// Load with component expansion (e.g., 3-component source -> 4-component GPU format)
	bool loadFromDataExpanded(const unsigned char* base, int srcComp, int dstComp, unsigned char fillValue = 255, bool isNormalMap = false);
	// Initialize the GXM texture object; returns SCE_OK on success or error code
	int32_t init();
	// Release GPU memory and unmap
	void release();

	uint32_t getWidth() const;
	uint32_t getHeight() const;
	const SceGxmTexture* getTexture() const;

	void setTextureType(SceGxmTextureType t);
	void setFormat(SceGxmTextureFormat f);
	void setSize(uint32_t w, uint32_t h);
	void setFilters(SceGxmTextureFilter minF,SceGxmTextureFilter magF, bool enableMip);
	void setAddressModes(SceGxmTextureAddrMode uMode, SceGxmTextureAddrMode vMode);
	void setMipCount(uint32_t count);
	void setLodBias(uint32_t bias);

	// Calculate total size needed for texture with mipmaps, respecting PS Vita alignment
	static size_t calculateTextureDataSize(int width, int height, int comp, unsigned int mipCount);

	static void generateMipmaps(unsigned char* gpuMemory,
		const unsigned char* base,
		int width, int height, int comp,
		unsigned int mipCount,
		bool isNormalMap = false);

private:
	SceGxmTexture texture;
	SceGxmTextureType texType;
	SceGxmTextureFormat texFormat;
	uint32_t width;
	uint32_t height;

	SceGxmTextureFilter minFilter;
	SceGxmTextureFilter magFilter;

	SceGxmTextureAddrMode uAddrMode;
	SceGxmTextureAddrMode vAddrMode;

	uint32_t mipCount;
	uint32_t lodBias;
	bool mipFiltering;

	void* memAddr;
	SceUID memUid;

	static const size_t TEXTURE_ALIGNMENT = 16;
};