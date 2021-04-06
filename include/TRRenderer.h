#ifndef TRRENDERER_H
#define TRRENDERER_H

#include "glm/glm.hpp"
#include "SDL2/SDL.h"

#include "TRFrameBuffer.h"
#include "TRDrawableMesh.h"
#include "TRShadingState.h"
#include "TRShadingPipeline.h"

namespace TinyRenderer
{
	class TRRenderer final
	{
	public:
		typedef std::shared_ptr<TRRenderer> ptr;

		TRRenderer(int width, int height);
		~TRRenderer() = default;

		//Drawable objects load/unload
		void addDrawableMesh(TRDrawableMesh::ptr mesh);
		void addDrawableMesh(const std::vector<TRDrawableMesh::ptr> &meshes);
		void unloadDrawableMesh();

		void clearColor(const glm::vec4 &color) { m_backBuffer->clearColor(color); }
		void clearDepth(const float &depth) { m_backBuffer->clearDepth(depth); }
		void clearColorAndDepth(const glm::vec4 &color, const float &depth) { m_backBuffer->clearColorAndDepth(color, depth); }

		//Setting
		void setViewMatrix(const glm::mat4 &view) { m_viewMatrix = view; }
		void setModelMatrix(const glm::mat4 &model) { m_modelMatrix = model; }
		void setProjectMatrix(const glm::mat4 &project, float near, float far) { m_projectMatrix = project;m_frustum_near_far = glm::vec2(near, far); }
		void setShaderPipeline(TRShadingPipeline::ptr shader) { m_shader_handler = shader; }
		void setViewerPos(const glm::vec3 &viewer);

		int addPointLight(glm::vec3 pos, glm::vec3 atten, glm::vec3 color);
		TRPointLight &getPointLight(const int &index);

		//Draw call
		unsigned int renderAllDrawableMeshes();

		unsigned int renderDrawableMesh(unsigned int &index);

		//Commit rendered result
		unsigned char* commitRenderedColorBuffer();

	private:

		//Homogeneous space clipping - Sutherland Hodgeman algorithm
		std::vector<TRShadingPipeline::VertexData> clipingSutherlandHodgeman(
			const TRShadingPipeline::VertexData &v0,
			const TRShadingPipeline::VertexData &v1,
			const TRShadingPipeline::VertexData &v2) const;

		//Cliping auxiliary functions
		std::vector<TRShadingPipeline::VertexData> clipingSutherlandHodgeman_aux(
			const std::vector<TRShadingPipeline::VertexData> &polygon,
			const int &axis, 
			const int &side) const;

		//Back face culling
		bool isBackFacing(const glm::ivec2 &v0, const glm::ivec2 &v1, const glm::ivec2 &v2, TRCullFaceMode mode) const;

		bool isDepthTestEnable() const { return m_shading_state.trDepthTestMode == TRDepthTestMode::TR_DEPTH_TEST_ENABLE; }
		bool isDepthWriteEnable() const { return m_shading_state.trDepthWriteMode == TRDepthWriteMode::TR_DEPTH_WRITE_ENABLE; }
		bool isBackFaceCullEnable() const { return m_shading_state.trCullFaceMode != TRCullFaceMode::TR_CULL_DISABLE; }

	private:

		//Drawable mesh array
		std::vector<TRDrawableMesh::ptr> m_drawableMeshes;

		//MVP transformation matrices
		glm::mat4 m_modelMatrix = glm::mat4(1.0f);				//From local space  -> world space
		glm::mat4 m_viewMatrix = glm::mat4(1.0f);				//From world space  -> camera space
		glm::mat4 m_projectMatrix = glm::mat4(1.0f);			//From camera space -> clip space
		glm::mat4 m_viewportMatrix = glm::mat4(1.0f);			//From ndc space    -> screen space

		TRShadingState m_shading_state;
		std::vector<std::vector<TRShadingPipeline::QuadFragments>> m_fragmentsCache;

		//Near plane & far plane
		glm::vec2 m_frustum_near_far;

		//Shader pipeline handler
		TRShadingPipeline::ptr m_shader_handler = nullptr;

		//Double buffers
		TRFrameBuffer::ptr m_backBuffer;                      // The frame buffer that's goint to be written.
		TRFrameBuffer::ptr m_frontBuffer;                     // The frame buffer that's goint to be displayed.
		std::vector<unsigned char> m_renderedImg;			// The rendered image.
	};
}

#endif