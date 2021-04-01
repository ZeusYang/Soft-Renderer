#ifndef TRDRAWABLEMESH_H
#define TRDRAWABLEMESH_H

#include <vector>
#include <memory>
#include <string>

#include "glm/glm.hpp"

#include "TRShadingState.h"

namespace TinyRenderer
{
	class TRVertexAttrib final
	{
	public:
		std::vector<glm::vec4> vpositions;
		std::vector<glm::vec4> vcolors;
		std::vector<glm::vec2> vtexcoords;
		std::vector<glm::vec3> vnormals;

		void clear()
		{
			std::vector<glm::vec4>().swap(vpositions);
			std::vector<glm::vec4>().swap(vcolors);
			std::vector<glm::vec2>().swap(vtexcoords);
			std::vector<glm::vec3>().swap(vnormals);

		}
	};

	class TRMeshFace final
	{
	public:
		unsigned int vposIndex[3];
		unsigned int vnorIndex[3];
		unsigned int vtexIndex[3];

		//TBN matrix
		glm::vec3 tangent;
		glm::vec3 bitangent;
	};

	class TRDrawableMesh
	{
	public:

		typedef std::shared_ptr<TRDrawableMesh> ptr;

		TRDrawableMesh() = default;
		~TRDrawableMesh() = default;
		
		TRDrawableMesh(const std::string &filename);
		TRDrawableMesh(const TRDrawableMesh& mesh)
			: m_vertices_attrib(mesh.m_vertices_attrib), m_mesh_faces(mesh.m_mesh_faces) {}
		TRDrawableMesh& operator=(const TRDrawableMesh& mesh);

		void loadMeshFromFile(const std::string &filename);

		TRVertexAttrib& getVerticesAttrib() { return m_vertices_attrib; }
		std::vector<TRMeshFace>& getMeshFaces() { return m_mesh_faces; }
		const TRVertexAttrib& getVerticesAttrib() const { return m_vertices_attrib; }
		const std::vector<TRMeshFace>& getMeshFaces() const { return m_mesh_faces; }

		void clear();

		//Setting
		void setPolygonMode(TRPolygonMode mode) { m_drawing_config.polygonMode = mode; }
		void setCullfaceMode(TRCullFaceMode mode) { m_drawing_config.cullfaceMode = mode; }
		void setDepthtestMode(TRDepthTestMode mode) { m_drawing_config.depthtestMode = mode; }
		void setDepthwriteMode(TRDepthWriteMode mode) { m_drawing_config.depthwriteMode = mode; }
		void setModelMatrix(const glm::mat4& mat) { m_drawing_config.modelMatrix = mat; }
		void setLightingMode(TRLightingMode mode) { m_drawing_config.lightingMode = mode; }
		void setDiffuseMapTexId(const int &id) { m_drawing_material.diffuseMapTexId = id; }
		void setSpecularMapTexId(const int &id) { m_drawing_material.specularMapTexId = id; }
		void setNormalMapTexId(const int &id) { m_drawing_material.normalMapTexId = id; }
		void setGlowMapTexId(const int &id) { m_drawing_material.glowMapTexId = id; }
		void setAmbientCoff(const glm::vec3 &cof) { m_drawing_material.kA = cof; }
		void setDiffuseCoff(const glm::vec3 &cof) { m_drawing_material.kD = cof; }
		void setSpecularCoff(const glm::vec3 &cof) { m_drawing_material.kS = cof; }
		void setEmissionCoff(const glm::vec3 &cof) { m_drawing_material.kE = cof; }

		//Getter
		TRPolygonMode getPolygonMode() const { return m_drawing_config.polygonMode; }
		TRCullFaceMode getCullfaceMode() const { return m_drawing_config.cullfaceMode; }
		TRDepthTestMode getDepthtestMode() const { return m_drawing_config.depthtestMode; }
		TRDepthWriteMode getDepthwriteMode() const { return m_drawing_config.depthwriteMode; }
		const glm::mat4& getModelMatrix() const { return m_drawing_config.modelMatrix; }
		TRLightingMode getLightingMode() const { return m_drawing_config.lightingMode; }
		const int& getDiffuseMapTexId() const { return m_drawing_material.diffuseMapTexId; }
		const int& getSpecularMapTexId() const { return m_drawing_material.specularMapTexId; }
		const int& getNormalMapTexId() const { return m_drawing_material.normalMapTexId; }
		const int& getGlowMapTexId() const { return m_drawing_material.glowMapTexId; }
		const glm::vec3& getAmbientCoff() const { return m_drawing_material.kA; }
		const glm::vec3& getDiffuseCoff() const { return m_drawing_material.kD; }
		const glm::vec3& getSpecularCoff() const { return m_drawing_material.kS; }
		const glm::vec3& getEmissionCoff() const { return m_drawing_material.kE; }
		const float& getSpecularExponent() const { return m_drawing_material.shininess; }

	protected:
		TRVertexAttrib m_vertices_attrib;
		std::vector<TRMeshFace> m_mesh_faces;

		//Configuration
		struct DrawableConfig
		{
			TRPolygonMode polygonMode = TRPolygonMode::TR_TRIANGLE_FILL;
			TRCullFaceMode cullfaceMode = TRCullFaceMode::TR_CULL_BACK;
			TRDepthTestMode depthtestMode = TRDepthTestMode::TR_DEPTH_TEST_ENABLE;
			TRDepthWriteMode depthwriteMode = TRDepthWriteMode::TR_DEPTH_WRITE_ENABLE;
			TRLightingMode lightingMode = TRLightingMode::TR_LIGHTING_ENABLE;
			glm::mat4 modelMatrix = glm::mat4(1.0f);
		};
		DrawableConfig m_drawing_config;

		//Material
		struct DrawableMaterial
		{
			int diffuseMapTexId = -1;
			int specularMapTexId = -1;
			int normalMapTexId = -1;
			int glowMapTexId = -1;
			glm::vec3 kA = glm::vec3(0.0f);//Ambient coefficient
			glm::vec3 kD = glm::vec3(1.0f);//Diffuse coefficient
			glm::vec3 kS = glm::vec3(0.0f);//Specular coefficient
			glm::vec3 kE = glm::vec3(0.0f);//Emission
			float shininess = 1.0f;		   //Specular highlight exponment
		};
		DrawableMaterial m_drawing_material;
	};

}

#endif