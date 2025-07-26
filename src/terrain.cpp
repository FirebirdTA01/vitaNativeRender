#include "terrain.h"

// Grid spaces making up a terrain tile (128x128 - two triangles per grid space)
#define GRID_SIZE 128
// Total size terrain tile
#define TILE_SIZE 500.0f

Terrain::Terrain()
{
	terrainVertices.reserve(static_cast<size_t>(GRID_SIZE * GRID_SIZE));
	//Two triangles per grid square with 3 indices each = 6
	terrainIndices.reserve(static_cast<size_t>(GRID_SIZE - 1) * (GRID_SIZE - 1) * 6);

	// To center the grid
	float halfSize = TILE_SIZE / 2.0f;
	float vertexSpacing = TILE_SIZE / static_cast<float>(GRID_SIZE - 1);

	// Y is up and grid is on XZ plane
	Vector3f normal = { 0.0f, 1.0f, 0.0f };
	// Tangents and bitangents are set in PBRVertex constructor

	// Vertices
	for (int z = 0; z < GRID_SIZE; z++)
	{
		for (int x = 0; x < GRID_SIZE; x++)
		{
			Vector3f pos = {
				(static_cast<float>(x) * vertexSpacing) - halfSize, // centered x
				0.0f, // flat terrain at y = 0
				(static_cast<float>(z) * vertexSpacing) - halfSize //centered z
			};

			//Normalize UVs
			Vector2f uv = {
				x / static_cast<float>(GRID_SIZE - 1),
				z / static_cast<float>(GRID_SIZE - 1)
			};

			PBRVertex vertex = PBRVertex(pos, uv, normal);

			terrainVertices.push_back(vertex);
		}
	}

	// Indices (two triangles per grid square
	for (int z = 0; z < GRID_SIZE - 1; z++)
	{
		for (int x = 0; x < GRID_SIZE - 1; x++)
		{
			unsigned int start = z * GRID_SIZE + x;
			//Triangle 1
			terrainIndices.push_back(start);
			terrainIndices.push_back(start + GRID_SIZE);
			terrainIndices.push_back(start + 1);

			//Triangle 2
			terrainIndices.push_back(start + 1);
			terrainIndices.push_back(start + GRID_SIZE);
			terrainIndices.push_back(start + GRID_SIZE + 1);
		}
	}

	//create the model matrix
	modelMatrix = createTransformationMatrix(Vector3f(0.0f, 0.0f, 0.0f), Vector3f(0.0f, 0.0f, 0.0f), Vector3f(1.0f, 1.0f, 1.0f));
}

Terrain::~Terrain()
{

}

size_t Terrain::getVertexCount() const
{
	return terrainVertices.size();
}

size_t Terrain::getIndexCount() const
{
	return terrainIndices.size();
}

std::vector<PBRVertex>* Terrain::getVertices()
{
	return &terrainVertices;
}

std::vector<unsigned int>* Terrain::getIndices()
{
	return &terrainIndices;
}

Matrix4x4& Terrain::getModelMatrix()
{
	return modelMatrix;
}