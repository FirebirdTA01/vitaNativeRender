#include "commonUtils.h"
#include "matrix.h"
#include <psp2/types.h>
#include <vector>
#include <memory>

//GPU Buffer Pool for efficient memory usage
//PSVita CDRAM type memory is very limited and alignment size is much larger than most meshes, leading to lots of waste
class TerrainBufferPool
{
public:
	struct BufferAllocation
	{
		void* cpuData; //CPU-side data pointer
		void* gpuData; //GPU-mapped memory
		size_t offset; //offset within the pool
		size_t size; //size of this allocation
	};

	TerrainBufferPool();
	~TerrainBufferPool();

	// Pre-calculate total size needed and allocate pools
	bool init(size_t totalVertexSize, size_t totalIndexSize);

	BufferAllocation allocateVertices(size_t size);
	BufferAllocation allocateIndices(size_t size);
	void* getVertexPoolBase() const;
	void* getIndexPoolBase() const;

private:
	SceUID vertexPoolUID;
	SceUID indexPoolUID;
	void* vertexPoolBase;
	void* indexPoolBase;
	size_t vertexPoolSize;
	size_t indexPoolSize;
	size_t currentVertexOffset;
	size_t currentIndexOffset;
};

class TerrainChunk
{
public:
	enum LODLevel
	{
		LOD_0 = 0,	// 64x64
		LOD_1,		// 32x32
		LOD_2,		// 16x16
		LOD_3,		// 8x8
		LOD_4,		// 2x2
		//LOD_5,		// 2x2
		LOD_COUNT
	};

	struct LODMesh
	{
		TerrainBufferPool::BufferAllocation vertexAlloc;
		TerrainBufferPool::BufferAllocation indexAlloc;
		size_t vertexCount;
		size_t indexCount;

		//CPU-side data for generation (released after GPU upload)
		std::vector<PBRVertex>* tempVertices;
		std::vector<uint16_t>* tempIndices;
	};

	//static constexpr int MaxResidentLOD = 3; // keep LOD_0...LOD_3 resident

	TerrainChunk(int chunkX, int chunkZ, float chunkWorldSize, float terrainHeight = 0.0f);
	~TerrainChunk();

	// Functions used for use with memory pool
	void initializeWithPool(TerrainBufferPool* pool);
	static void calculateMemoryRequirements(size_t& vertexSize, size_t& indexSize);
	//Upload mesh data to GPU pool
	void uploadToGPU();
	//Release temporary CPU data after upload
	void releaseCPUData();
	// End of functions for use with memory pool

	// Get appropriate LOD based on camera distance
	LODLevel calculateLOD(const Vector3f& cameraPos) const;

	//Get mesh data for specific LOD
	LODMesh* getLODMesh(LODLevel lod);
	const LODMesh* getLODMesh(LODLevel lod) const;

	//Get current LOD mesh based on last calculated LOD
	LODMesh* getCurrentLODMesh();
	const LODMesh* getCurrentLODMesh() const;

	Vector3f getCenter() const;
	float getBoundingRadius() const;
	LODLevel getCurrentLOD() const;
	void setCurrentLOD(LODLevel lod);

	// Check if chunk is in frustum
	bool isInFrustum(const Matrix4x4& viewProjMatrix) const;


private:
	void generateLODMesh(int verticesPerSide, LODMesh& lodMesh);

	TerrainBufferPool* bufferPool;
	int chunkX, chunkZ; // Coordinates in the terrain grid
	Vector3f center;	// World space center of chunk
	float boundingRadius;
	float chunkSize;	// World size of this chunk

	LODMesh lodMeshes[LOD_COUNT];
	LODLevel currentLOD;

	// Distance thresholds for edge-aware LOD (chunk size - 500 / 10 = 62.5)
	static constexpr float LOD_DISTANCES[LOD_COUNT] = {
		0.0f,	// LOD_0: 0 - 1 units
		1.0f,	// LOD_1: 1 - 2 units
		2.0f,	// LOD_2: 2 - 10
		10.0f,	// LOD_3: 10 - 50
		50.0f	// LOD_4: 50+
		//75.0f	// LOD_5: 75+
	};
};

class Terrain
{
public:
	static constexpr int CHUNKS_PER_SIDE = 14; // 14x14 chunks per terrain tile
	static constexpr int CHUNK_GRID_SIZE = 64; // Each chunk is 64x64 at highest LOD
	static constexpr int TERRAIN_GRID_SIZE = CHUNKS_PER_SIDE * CHUNK_GRID_SIZE;
	static constexpr float TERRAIN_SIZE = 500.0f;
	static constexpr float CHUNK_SIZE = TERRAIN_SIZE / CHUNKS_PER_SIDE;
	static constexpr float TEXTURE_TILE_COUNT = 100.0f; // Number of texture repititions across the entire terrain tile

	Terrain();
	~Terrain();

	bool initialize();
	// Get chunks visible after frustum culling
	std::vector<TerrainChunk*> getVisibleChunks(const Matrix4x4& viewProjMatrix);
	// Get all chunks (for initialization)
	const std::vector<std::unique_ptr<TerrainChunk>>& getChunks() const;
	// Get chunk at specified grid coordinate
	TerrainChunk* getChunk(int chunkX, int chunkZ);
	
	int getTotalVertices() const;
	int getTotalIndices() const;

	// Update LODs for all chunks
	void updateLODs(const Vector3f& cameraPos);

	TerrainBufferPool* getBufferPool();
	Matrix4x4& getModelMatrix();

private:
	std::vector<std::unique_ptr<TerrainChunk> > chunks;
	std::unique_ptr<TerrainBufferPool> bufferPool;
	Matrix4x4 modelMatrix;
	int visibleChunkCount;

	// Terrain position offset (for multiple terrain tiles) and to position the first tile centered at 0,0
	Vector3f terrainOffset;
};