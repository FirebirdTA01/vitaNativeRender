#include "commonUtils.h"
#include "matrix.h"
#include <vector>

class Terrain
{
public:
	Terrain();
	~Terrain();

	size_t getVertexCount() const;
	size_t getIndexCount() const;
	
	std::vector<PBRVertex>* getVertices();
	std::vector<unsigned int>* getIndices();
	Matrix4x4& getModelMatrix();

private:
	std::vector<PBRVertex> terrainVertices;
	std::vector<unsigned int> terrainIndices;
	Matrix4x4 modelMatrix;
};