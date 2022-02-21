#include "Mesh.h"

#include <assimp/LogStream.hpp>
#include <assimp/DefaultLogger.hpp>
#include <assimp/postprocess.h>

#include <iostream>

struct AssimpLogger : public Assimp::LogStream
{
	static void Initialize()
	{
		if (Assimp::DefaultLogger::isNullLogger())
		{
			Assimp::DefaultLogger::create("", Assimp::Logger::VERBOSE);
			Assimp::DefaultLogger::get()->attachStream(new AssimpLogger, Assimp::Logger::Err | Assimp::Logger::Warn);
		}
	}

	virtual void write(const char* message) override
	{
		std::cout << "[ASSIMP ERROR]: " << message;
	}
};

Mesh::Mesh(const std::string& filePath)
	: filePath(filePath), boundingBox(nullptr)
{
	//AssimpLogger::Initialize();

	std::cout << "Loading mesh " << filePath << "..." << std::endl;;
	this->importer = CreateScope<Assimp::Importer>();

	const aiScene* scene = this->importer->ReadFile(filePath, MeshUtils::ASSIMP_FLAGS);
	if (!scene || !scene->HasMeshes())
	{
		std::cout << "Failed to load mesh file: " << filePath << std::endl;
		return;
	}

	this->assimpScene = scene;
	this->inverseTransform = glm::inverse(MeshUtils::ConvertToGLMMat4(scene->mRootNode->mTransformation));
	this->submeshes.reserve(scene->mNumMeshes);

	uint32_t vertexCount = 0;
	uint32_t indexCount = 0;

	for (unsigned int i = 0; i < scene->mNumMeshes; i++)
	{
		aiMesh* assimpMesh = scene->mMeshes[i];

		// Create a submesh
		Submesh submesh;
		submesh.baseVertex = vertexCount; // Assign at what index this submeshes vertices start
		submesh.baseIndex = indexCount; // Assign at what index this submeshes indices start
		submesh.materialIndex = assimpMesh->mMaterialIndex; // Assign at what index this submeshes materials start
		submesh.vertexCount = assimpMesh->mNumVertices;
		submesh.indexCount = assimpMesh->mNumFaces * 3;
		submesh.meshName = assimpMesh->mName.C_Str();

		vertexCount += assimpMesh->mNumVertices;
		indexCount += submesh.indexCount;

		if (!assimpMesh->HasPositions())
		{
			std::cout << filePath << " does not have position!" << std::endl;
			return;
		}

		if (!assimpMesh->HasNormals())
		{
			std::cout << filePath << " does not have normals!" << std::endl;
			return;
		}

		// Define min/max for this meshes bounding box
		glm::vec3 min = glm::vec3(FLT_MAX, FLT_MAX, FLT_MAX);
		glm::vec3 max = glm::vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
		for (unsigned int j = 0; j < assimpMesh->mNumVertices; j++)
		{
			glm::vec3 position = glm::vec3(assimpMesh->mVertices[j].x, assimpMesh->mVertices[j].y, assimpMesh->mVertices[j].z);
			glm::vec3 normal = glm::vec3(assimpMesh->mNormals[j].x, assimpMesh->mNormals[j].y, assimpMesh->mNormals[j].z);

			// Find the lowest and highest points of the bounding box
			min.x = glm::min(position.x, min.x);
			min.y = glm::min(position.y, min.y);
			min.z = glm::min(position.z, min.z);
			max.x = glm::max(position.x, max.x);
			max.y = glm::max(position.y, max.y);
			max.z = glm::max(position.z, max.z);

			glm::vec2 textureCoords(0.0f);
			if (assimpMesh->HasTextureCoords(0))
			{
				textureCoords.x = assimpMesh->mTextureCoords[0][j].x;
				textureCoords.y = assimpMesh->mTextureCoords[0][j].y;
			}

			this->vertices.push_back(new Vertex(position, normal, textureCoords));
		}

		submesh.boundingBox->Resize(min, max); // Resize the bounding box to the proper size

		// Assign faces/indices
		for (unsigned int j = 0; j < assimpMesh->mNumFaces; j++)
		{
			if (assimpMesh->mFaces[j].mNumIndices != 3)
			{
				std::cout << "Face must be a triangle!" << std::endl;
				return;
			}

			Face face;
			face.v1 = assimpMesh->mFaces[j].mIndices[0];
			face.v2 = assimpMesh->mFaces[j].mIndices[1];
			face.v3 = assimpMesh->mFaces[j].mIndices[2];
			this->faces.push_back(face);
		}

		this->submeshes.push_back(submesh);
	}

	LoadNodes(scene->mRootNode); // Load all the submeshes

	// Configure parent's bounding box based off of the submeshes we just added
	glm::vec3 parentMin = glm::vec3(FLT_MAX, FLT_MAX, FLT_MAX);
	glm::vec3 parentMax = glm::vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
	for (Submesh& submesh : this->submeshes)
	{
		AABB* submeshAABB = submesh.boundingBox;
		glm::vec3 submeshMin = submeshAABB->GetMin();
		glm::vec3 submeshMax = submeshAABB->GetMax();

		glm::vec3 min = glm::vec3(submesh.transform * glm::vec4(submeshMin, 1.0f));
		glm::vec3 max = glm::vec3(submesh.transform * glm::vec4(submeshMax, 1.0f));

		parentMin.x = glm::min(parentMin.x, min.x);
		parentMin.y = glm::min(parentMin.y, min.y);
		parentMin.z = glm::min(parentMin.z, min.z);
		parentMax.x = glm::max(parentMax.x, max.x);
		parentMax.y = glm::max(parentMax.y, max.y);
		parentMax.z = glm::max(parentMax.z, max.z);
	}
	this->boundingBox = new AABB(parentMin, parentMax);

	if (scene->HasMaterials())
	{
		SetupMaterials();
	}

	BufferLayout bufferLayout = {
		{ ShaderDataType::Float3, "vPosition" },
		{ ShaderDataType::Float3, "vNormal" },
		{ ShaderDataType::Float2, "vTextureCoordinates" }
	};

	this->vertexArray = new VertexArrayObject(); 

	uint32_t vertexBufferSize = (uint32_t) (this->vertices.size() * Vertex::Size());
	float* vertexBuffer = MeshUtils::ConvertVerticesToArray(this->vertices);

	this->vertexBuffer = new VertexBuffer(vertexBuffer, vertexBufferSize);
	this->vertexBuffer->SetLayout(bufferLayout);

	uint32_t indexBufferSize = (uint32_t)(this->faces.size() * sizeof(Face));
	uint32_t* indexBuffer = MeshUtils::ConvertIndicesToArray(this->faces);

	this->vertexArray->Bind();
	this->indexBuffer = new IndexBuffer(indexBuffer, indexBufferSize);

	this->vertexArray->AddVertexBuffer(this->vertexBuffer);
	this->vertexArray->SetIndexBuffer(this->indexBuffer);

	delete[] vertexBuffer;
	delete[] indexBuffer;
}

Mesh::~Mesh()
{
	delete vertexArray;
	delete vertexBuffer;
	delete indexBuffer;

	for (IVertex* v : vertices) delete v;

	delete boundingBox;
	for (Submesh& submesh : submeshes) delete submesh.boundingBox;
}

void Mesh::LoadNodes(aiNode* node, const glm::mat4& parentTransform)
{
	glm::mat4 transform = parentTransform * MeshUtils::ConvertToGLMMat4(node->mTransformation);
	this->nodeMap[node].resize(node->mNumMeshes);
	for (unsigned int i = 0; i < node->mNumMeshes; i++)
	{
		unsigned int meshIndex = node->mMeshes[i];
		Submesh& submesh = this->submeshes[meshIndex];
		submesh.nodeName = node->mName.C_Str();
		submesh.transform = transform;
		this->nodeMap[node][i] = meshIndex;
	}

	for (unsigned int i = 0; i < node->mNumChildren; i++)
	{
		LoadNodes(node->mChildren[i], transform);
	}
}

void Mesh::SetupMaterials()
{
	// TODO: Setup materials
}