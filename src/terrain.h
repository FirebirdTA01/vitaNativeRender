#include "commonUtils.h"
#include "matrix.h"
#include <vector>
#include <memory>

class TerrainChunk
{
public:
	enum LODLevel
	{
		LOD_0 = 0,	// 32x32
		LOD_1,		// 16x16
		LOD_2,		// 8x8
		LOD_3,		// 4x4
		LOD_COUNT
	};

	struct LODMesh
	{
		std::vector<PBRVertex> vertices;
		std::vector<unsigned int> indices;
		size_t vertexCount;
		size_t indexCount;
	};

	TerrainChunk(int chunkX, int chunkZ, float chunkWorldSize, float terrainHeight = 0.0f);
	~TerrainChunk();

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

	int chunkX, chunkZ; // Coordinates in the terrain grid
	Vector3f center;	// World space center of chunk
	float boundingRadius;
	float chunkSize;	// World size of this chunk

	LODMesh lodMeshes[LOD_COUNT];
	LODLevel currentLOD;

	// Distance thresholds
	static constexpr float LOD_DISTANCES[LOD_COUNT] = {
		0.0f,	// LOD_0: 0-50 units
		50.0f,	// LOD_1: 50-100 units
		100.0f,	// LOD_2: 100-200
		200.0f	// LOD_3: 200+
	};
};

class Terrain
{
public:
	static constexpr int CHUNKS_PER_SIDE = 4; // 4x4 chunks per terrain tile
	static constexpr int CHUNK_GRID_SIZE = 32; // Each chunk is 32x32 at highest LOD
	static constexpr int TERRAIN_GRID_SIZE = CHUNKS_PER_SIDE * CHUNK_GRID_SIZE; // 128x128 total
	static constexpr float TERRAIN_SIZE = 500.0f;
	static constexpr float CHUNK_SIZE = TERRAIN_SIZE / CHUNKS_PER_SIDE;

	Terrain();
	~Terrain();

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

	Matrix4x4& getModelMatrix();

private:
	std::vector<std::unique_ptr<TerrainChunk> > chunks;
	Matrix4x4 modelMatrix;
	int visibleChunkCount;

	// Terrain position offset (for multiple terrain tiles) and to position the first tile centered at 0,0
	Vector3f terrainOffset;
};