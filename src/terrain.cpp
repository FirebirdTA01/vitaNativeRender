#include "terrain.h"
#include <cmath>

// Vertices per side for each LOD (densest first)
static const int LOD_VERTICES[TerrainChunk::LOD_COUNT] = { 33, 17, 9, 5 };

TerrainChunk::TerrainChunk(int chunkXin, int chunkZin, float chunkWorldSize, float terrainHeight)
	: chunkX(chunkXin), chunkZ(chunkZin), chunkSize(chunkWorldSize), currentLOD(LOD_0)
{
	// Calculate world position of chunk center
	center.x = (chunkX + 0.5f) * chunkSize;
	center.y = (terrainHeight);
	center.z = (chunkZ + 0.5f) * chunkSize;

	// Bounding radius for frustum culling
	boundingRadius = chunkSize * 0.707f; // sqrt(2)/2

	// Generate all LODs
	for (int i = 0; i < LOD_COUNT; i++)
	{
		generateLODMesh(LOD_VERTICES[i], lodMeshes[i]);
	}
}

TerrainChunk::~TerrainChunk()
{
}

TerrainChunk::LODLevel TerrainChunk::calculateLOD(const Vector3f& cameraPos) const
{
	// Distance from camera to chunk center
	float dx = center.x - cameraPos.x;
	float dy = center.y - cameraPos.y;
	float dz = center.z - cameraPos.z;
	float distance = sqrtf(dx * dx + dy * dy + dz * dz);

	// Distance to chunk edge
	float distToEdge = distance - boundingRadius;
	if (distToEdge < 0.0f)
		distToEdge = 0.0f; // inside the sphere

	// Select LOD based on edge distance
	for (int i = LOD_COUNT - 1; i >= 0; i--)
	{
		if (distToEdge >= LOD_DISTANCES[i])
		{
			return static_cast<LODLevel>(i);
		}
	}

	return LOD_0;
}

TerrainChunk::LODMesh* TerrainChunk::getLODMesh(LODLevel lod)
{
	return &lodMeshes[lod];
}

const TerrainChunk::LODMesh* TerrainChunk::getLODMesh(LODLevel lod) const
{
	return &lodMeshes[lod];
}

TerrainChunk::LODMesh* TerrainChunk::getCurrentLODMesh()
{
	return &lodMeshes[currentLOD];
}

const TerrainChunk::LODMesh* TerrainChunk::getCurrentLODMesh() const
{
	return &lodMeshes[currentLOD];
}

Vector3f TerrainChunk::getCenter() const
{
	return center;
}

float TerrainChunk::getBoundingRadius() const
{
	return boundingRadius;
}

TerrainChunk::LODLevel TerrainChunk::getCurrentLOD() const
{
	return currentLOD;
}

void TerrainChunk::setCurrentLOD(LODLevel lod)
{
	currentLOD = lod;
}

bool TerrainChunk::isInFrustum(const Matrix4x4& viewProjMatrix) const
{
	// Simple sphere frustum test
	
	//Row-major layout:
	// row0 = m[0],m[1],m[2],m[3]
	// row1 = m[4],m[5],m[6],m[7]
	// row2 = m[8],m[9],m[10],m[11]
	// row3 = m[12],m[13],m[14],m[15]
	const float* m = viewProjMatrix.getData();

	struct Plane
	{
		float a, b, c, d;
	}planes[6];

	// LEFT = row3 + row0
	planes[0] = 
	{
	  m[12] + m[0],
	  m[13] + m[1],
	  m[14] + m[2],
	  m[15] + m[3]
	};

	// RIGHT = row3 - row0
	planes[1] = 
	{
	  m[12] - m[0],
	  m[13] - m[1],
	  m[14] - m[2],
	  m[15] - m[3]
	};

	// BOTTOM = row3 + row1
	planes[2] = 
	{
	  m[12] + m[4],
	  m[13] + m[5],
	  m[14] + m[6],
	  m[15] + m[7]
	};

	// TOP = row3 - row1
	planes[3] = 
	{
	  m[12] - m[4],
	  m[13] - m[5],
	  m[14] - m[6],
	  m[15] - m[7]
	};

	// NEAR = row3 + row2
	planes[4] = 
	{
	  m[12] + m[8],
	  m[13] + m[9],
	  m[14] + m[10],
	  m[15] + m[11]
	};

	// FAR = row3 - row2
	planes[5] = 
	{
	  m[12] - m[8],
	  m[13] - m[9],
	  m[14] - m[10],
	  m[15] - m[11]
	};

	// normalize + sphere-plane test
	for (int i = 0; i < 6; ++i) 
	{
		float invLen = 1.0f / sqrtf(
			planes[i].a * planes[i].a +
			planes[i].b * planes[i].b +
			planes[i].c * planes[i].c
		);

		planes[i].a *= invLen;
		planes[i].b *= invLen;
		planes[i].c *= invLen;
		planes[i].d *= invLen;

		// signed distance from center to plane
		float dist = planes[i].a * center.x
			+ planes[i].b * center.y
			+ planes[i].c * center.z
			+ planes[i].d;

		// if the entire sphere is “behind” this plane, cull it
		if (dist < -boundingRadius)
			return false;
	}

	return true;
}

void TerrainChunk::generateLODMesh(int verticesPerSide, LODMesh& lodMesh)
{
	lodMesh.vertices.clear();
	lodMesh.indices.clear();

	int gridSize = verticesPerSide - 1;
	lodMesh.vertices.reserve(verticesPerSide * verticesPerSide);
	lodMesh.indices.reserve(gridSize * gridSize * 6);

	float vertexSpacing = chunkSize / static_cast<float>(gridSize);
	// Y is up and grid is on XZ plane
	Vector3f normal = { 0.0f, 1.0f, 0.0f };
	// Tangents and bitangents are set in PBRVertex constructor

	// Generate Vertices
	for (int z = 0; z < verticesPerSide; z++)
	{
		for (int x = 0; x < verticesPerSide; x++)
		{
			// Position relative to chunk origin
			Vector3f pos;
			pos.x = (chunkX * chunkSize) + (x * vertexSpacing);
			pos.y = 0.0f;
			pos.z = (chunkZ * chunkSize) + (z * vertexSpacing);

			// UV coordinates - map to the chunks portion of the texture
			// Texture repeats over each chunk
			Vector2f uv;
			uv.x = static_cast<float>(x) / static_cast<float>(gridSize);
			uv.y = static_cast<float>(z) / static_cast<float>(gridSize);

			PBRVertex vertex(pos, uv, normal);
			lodMesh.vertices.push_back(vertex);
		}
	}

	// Generate indices
	for (int z = 0; z < gridSize; z++)
	{
		for (int x = 0; x < gridSize; x++)
		{
			unsigned int start = z * verticesPerSide + x;

			//Triangle 1
			lodMesh.indices.push_back(start);
			lodMesh.indices.push_back(start + verticesPerSide);
			lodMesh.indices.push_back(start + 1);

			//Triangle 2
			lodMesh.indices.push_back(start + 1);
			lodMesh.indices.push_back(start + verticesPerSide);
			lodMesh.indices.push_back(start + verticesPerSide + 1);
		}
	}

	lodMesh.vertexCount = lodMesh.vertices.size();
	lodMesh.indexCount = lodMesh.indices.size();
}

Terrain::Terrain()
	: visibleChunkCount(0), terrainOffset(-TERRAIN_SIZE * 0.5f, 0.0f, -TERRAIN_SIZE * 0.5f)
{
	// Create all chunks
	chunks.reserve(CHUNKS_PER_SIDE * CHUNKS_PER_SIDE);

	// To center the grid
	float halfSize = TERRAIN_SIZE / 2.0f;

	// Vertices
	for (int z = 0; z < CHUNKS_PER_SIDE; z++)
	{
		for (int x = 0; x < CHUNKS_PER_SIDE; x++)
		{
			// Adjust chunk positions to center the terrain
			float chunkWorldX = (x * CHUNK_SIZE) - halfSize;
			float chunkWorldZ = (z * CHUNK_SIZE) - halfSize;

			auto chunk = std::make_unique<TerrainChunk>(x, z, CHUNK_SIZE);
			chunks.push_back(std::move(chunk));
		}
	}

	//create the model matrix so that chunk centers (0...500) shift to -250...+250 (world pos 0,0)
	modelMatrix = createTransformationMatrix(
		terrainOffset,
		Vector3f(0.0f, 0.0f, 0.0f),
		Vector3f(1.0f, 1.0f, 1.0f)
	);
}

Terrain::~Terrain()
{

}

std::vector<TerrainChunk*> Terrain::getVisibleChunks(const Matrix4x4& viewProjMatrix)
{
	std::vector<TerrainChunk*> visible;
	visible.reserve(chunks.size());

	visibleChunkCount = 0;
	for (auto& chunk : chunks)
	{
		if (chunk->isInFrustum(viewProjMatrix))
		{
			visible.push_back(chunk.get());
			visibleChunkCount++;
		}
	}

	return visible;
}

const std::vector<std::unique_ptr<TerrainChunk>>& Terrain::getChunks() const
{
	return chunks;
}

TerrainChunk* Terrain::getChunk(int chunkX, int chunkZ)
{
	if (chunkX < 0 || chunkX >= CHUNKS_PER_SIDE ||
		chunkZ < 0 || chunkZ >= CHUNKS_PER_SIDE)
	{
		return nullptr;
	}

	int index = chunkZ * CHUNKS_PER_SIDE + chunkX;
	return chunks[index].get();
}

int Terrain::getTotalVertices() const
{
	int total = 0;
	for (const auto& chunk : chunks)
	{
		total += chunk->getCurrentLODMesh()->vertexCount;
	}
	return total;
}

int Terrain::getTotalIndices() const
{
	int total = 0;
	for (const auto& chunk : chunks)
	{
		total += chunk->getCurrentLODMesh()->indexCount;
	}
	return total;
}

void Terrain::updateLODs(const Vector3f& cameraPos)
{
	for (auto& chunk : chunks)
	{
		// shift camera into local terrain space
		Vector3f localCam = cameraPos - terrainOffset;
		TerrainChunk::LODLevel newLOD = chunk->calculateLOD(localCam);
		chunk->setCurrentLOD(newLOD);
	}
}

Matrix4x4& Terrain::getModelMatrix()
{
	return modelMatrix;
}

