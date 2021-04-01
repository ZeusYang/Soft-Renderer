﻿#include "TRShadingPipeline.h"

#include <algorithm>
#include <iostream>

#include "TRParallelWrapper.h"

namespace TinyRenderer
{
	//----------------------------------------------VertexData----------------------------------------------

	TRShadingPipeline::VertexData TRShadingPipeline::VertexData::lerp(
		const TRShadingPipeline::VertexData &v0,
		const TRShadingPipeline::VertexData &v1,
		float frac)
	{
		//Linear interpolation
		VertexData result;
		result.pos = (1.0f - frac) * v0.pos + frac * v1.pos;
		result.col = (1.0f - frac) * v0.col + frac * v1.col;
		result.nor = (1.0f - frac) * v0.nor + frac * v1.nor;
		result.tex = (1.0f - frac) * v0.tex + frac * v1.tex;
		result.cpos = (1.0f - frac) * v0.cpos + frac * v1.cpos;
		result.spos.x = (1.0f - frac) * v0.spos.x + frac * v1.spos.x;
		result.spos.y = (1.0f - frac) * v0.spos.y + frac * v1.spos.y;

		result.TBN = v0.TBN;

		return result;
	}

	TRShadingPipeline::VertexData TRShadingPipeline::VertexData::barycentricLerp(
		const VertexData &v0, 
		const VertexData &v1, 
		const VertexData &v2,
		const glm::vec3 &w)
	{
		VertexData result;
		result.pos = w.x * v0.pos + w.y * v1.pos + w.z * v2.pos;
		result.col = w.x * v0.col + w.y * v1.col + w.z * v2.col;
		result.nor = w.x * v0.nor + w.y * v1.nor + w.z * v2.nor;
		result.tex = w.x * v0.tex + w.y * v1.tex + w.z * v2.tex;
		result.cpos = w.x * v0.cpos + w.y * v1.cpos + w.z * v2.cpos;
		result.spos.x = w.x * v0.spos.x + w.y * v1.spos.x + w.z * v2.spos.x;
		result.spos.y = w.x * v0.spos.y + w.y * v1.spos.y + w.z * v2.spos.y;

		result.TBN = v0.TBN;

		return result;
	}

	void TRShadingPipeline::VertexData::prePerspCorrection(VertexData &v)
	{
		//Perspective correction: the world space properties should be multipy by 1/w before rasterization
		//https://zhuanlan.zhihu.com/p/144331875
		//We use pos.w to store 1/w
		float one_div_w =  1.0f / v.cpos.w;
		v.pos = glm::vec4(v.pos.x * one_div_w, v.pos.y * one_div_w, v.pos.z * one_div_w, one_div_w);
		v.tex = v.tex * one_div_w;
		v.nor = v.nor * one_div_w;
		v.col = v.col * one_div_w;
	}

	void TRShadingPipeline::VertexData::aftPrespCorrection(VertexData &v)
	{
		//Perspective correction: the world space properties should be multipy by w after rasterization
		//https://zhuanlan.zhihu.com/p/144331875
		//We use pos.w to store 1/w
		float w = 1.0f / v.pos.w;
		//v.cpos.z *= w;
		v.pos = glm::vec4(v.pos.x * w, v.pos.y * w, v.pos.z * w, v.pos.w);
		v.tex = v.tex * w;
		v.nor = v.nor * w;
		v.col = v.col * w;
	}

	//----------------------------------------------TRShadingPipeline----------------------------------------------

	std::vector<TRTexture2D::ptr> TRShadingPipeline::m_global_texture_units = {};
	std::vector<TRPointLight> TRShadingPipeline::m_point_lights = {};
	glm::vec3 TRShadingPipeline::m_viewer_pos = glm::vec3(0.0f);

	void TRShadingPipeline::rasterize_fill_edge_function(
		const VertexData &v0,
		const VertexData &v1,
		const VertexData &v2,
		const unsigned int &screen_width,
		const unsigned int &screene_height,
		std::vector<QuadFragments> &rasterized_fragments)
	{
		//Edge function rasterization algorithm
		//Accelerated Half-Space Triangle Rasterization
		//Refs:Mileff P, Nehéz K, Dudra J. Accelerated half-space triangle rasterization[J].
		//     Acta Polytechnica Hungarica, 2015, 12(7): 217-236.
		//	   http://acta.uni-obuda.hu/Mileff_Nehez_Dudra_63.pdf

		VertexData v[] = { v0, v1, v2 };
		glm::ivec2 bounding_min;
		glm::ivec2 bounding_max;
		bounding_min.x = std::max(std::min(v0.spos.x, std::min(v1.spos.x, v2.spos.x)), 0);
		bounding_min.y = std::max(std::min(v0.spos.y, std::min(v1.spos.y, v2.spos.y)), 0);
		bounding_max.x = std::min(std::max(v0.spos.x, std::max(v1.spos.x, v2.spos.x)), (int)screen_width - 1);
		bounding_max.y = std::min(std::max(v0.spos.y, std::max(v1.spos.y, v2.spos.y)), (int)screene_height - 1);

		//Adjust the order
		{
			int orient = 0;
			auto e1 = v1.spos - v0.spos;
			auto e2 = v2.spos - v0.spos;
			orient = e1.x * e2.y - e1.y * e2.x;
			if (orient > 0)
			{
				std::swap(v[1], v[2]);
			}
		}

		const glm::ivec2 &A = v[0].spos;
		const glm::ivec2 &B = v[1].spos;
		const glm::ivec2 &C = v[2].spos;

		const int I01 = A.y - B.y, I02 = B.y - C.y, I03 = C.y - A.y;
		const int J01 = B.x - A.x, J02 = C.x - B.x, J03 = A.x - C.x;
		const int K01 = A.x * B.y - A.y * B.x;
		const int K02 = B.x * C.y - B.y * C.x;
		const int K03 = C.x * A.y - C.y * A.x;

		int F01 = I01 * bounding_min.x + J01 * bounding_min.y + K01;
		int F02 = I02 * bounding_min.x + J02 * bounding_min.y + K02;
		int F03 = I03 * bounding_min.x + J03 * bounding_min.y + K03;

		//Degenerated to a line or a point
		if (F01 + F02 + F03 == 0)
			return;

		//Top left fill rule
		const int E1_t = (((B.y > A.y) || (A.y == B.y && A.x > B.x)) ? 0 : -1);
		const int E2_t = (((C.y > B.y) || (B.y == C.y && B.x > C.x)) ? 0 : -1);
		const int E3_t = (((A.y > C.y) || (C.y == A.y && C.x > A.x)) ? 0 : -1);

		int Cy1 = F01, Cy2 = F02, Cy3 = F03;
		const float one_div_delta = 1.0f / (F01 + F02 + F03);

		auto edge_func = [&](const int &x, const int &y, const int &Cx1, const int &Cx2, const int &Cx3, VertexData &p) -> bool
		{
			//Invalid fragment
			if (x > bounding_max.x || y > bounding_max.y)
			{
				p = VertexData(glm::ivec2(-1));
				return false;
			}

			//Note: Counter-clockwise winding order
			int E1 = Cx1 + E1_t, E2 = Cx2 + E2_t, E3 = Cx3 + E3_t;
			if (E1 <= 0 && E2 <= 0 && E3 <= 0)
			{
				glm::vec3 uvw(Cx2, Cx3, Cx1);
				p = VertexData::barycentricLerp(v[0], v[1], v[2], uvw * one_div_delta);
				p.spos = glm::ivec2(x, y);
				return true;
			}

			//Invalid fragment
			p = VertexData(glm::ivec2(-1));
			return false;
		};

		//Strict barycenteric weights calculation
		auto barycentericWeight = [&](const int &x, const int &y) -> glm::vec3
		{
			glm::vec3 s[2];
			s[0] = glm::vec3(v[2].spos.x - v[0].spos.x, v[1].spos.x - v[0].spos.x, v[0].spos.x - x);
			s[1] = glm::vec3(v[2].spos.y - v[0].spos.y, v[1].spos.y - v[0].spos.y, v[0].spos.y - y);
			auto uf = glm::cross(s[0], s[1]);
			return glm::vec3(1.f - (uf.x + uf.y) / uf.z, uf.y / uf.z, uf.x / uf.z);
		};

		for(int y = bounding_min.y;y <= bounding_max.y;y += 2)
		{
			int Cx1 = Cy1, Cx2 = Cy2, Cx3 = Cy3;
			for (int x = bounding_min.x; x <= bounding_max.x; x += 2)
			{
				//2x2 fragments block
				QuadFragments group;
				bool inside1 = edge_func(x, y, Cx1, Cx2, Cx3, group.fragments[0]);
				bool inside2 = edge_func(x + 1, y, Cx1 + I01, Cx2 + I02, Cx3 + I03, group.fragments[1]);
				bool inside3 = edge_func(x, y + 1, Cx1 + J01, Cx2 + J02, Cx3 + J03, group.fragments[2]);
				bool inside4 = edge_func(x + 1, y + 1, Cx1 + J01 + I01, Cx2 + J02 + I02, Cx3 + J03 + I03, group.fragments[3]);
				//Note: at least one of them is inside the triangle.
				if (inside1 || inside2 || inside3 || inside4)
				{
					if (!inside1)
					{
						group.fragments[0] = VertexData::barycentricLerp(v[0], v[1], v[2], barycentericWeight(x, y));
						group.fragments[0].spos = glm::ivec2(-1);
					}
					if (!inside2)
					{
						group.fragments[1] = VertexData::barycentricLerp(v[0], v[1], v[2], barycentericWeight(x + 1, y));
						group.fragments[1].spos = glm::ivec2(-1);
					}
					if (!inside3)
					{
						group.fragments[2] = VertexData::barycentricLerp(v[0], v[1], v[2], barycentericWeight(x, y + 1));
						group.fragments[2].spos = glm::ivec2(-1);
					}
					if (!inside4)
					{
						group.fragments[3] = VertexData::barycentricLerp(v[0], v[1], v[2], barycentericWeight(x + 1, y + 1));
						group.fragments[3].spos = glm::ivec2(-1);
					}

					rasterized_fragments.push_back(group);
				}
				Cx1 += 2 * I01; Cx2 += 2 * I02; Cx3 += 2 * I03;
			}
			Cy1 += 2 * J01;	Cy2 += 2 * J02; Cy3 += 2 * J03;
		}
	}

	int TRShadingPipeline::upload_texture_2D(TRTexture2D::ptr tex)
	{
		if (tex != nullptr)
		{
			m_global_texture_units.push_back(tex);
			return m_global_texture_units.size() - 1;
		}
		return -1;
	}

	TRTexture2D::ptr TRShadingPipeline::getTexture2D(int index)
	{
		if (index < 0 || index >= m_global_texture_units.size())
			return nullptr;
		return m_global_texture_units[index];
	}

	int TRShadingPipeline::addPointLight(glm::vec3 pos, glm::vec3 atten, glm::vec3 color)
	{
		m_point_lights.push_back(TRPointLight(pos, atten, color));
		return m_point_lights.size() - 1;
	}

	TRPointLight &TRShadingPipeline::getPointLight(int index)
	{
		return m_point_lights[index];
	}

	glm::vec4 TRShadingPipeline::texture2D(const unsigned int &id, const glm::vec2 &uv,
		const glm::vec2 &dUVdx, const glm::vec2 &dUVdy)
	{
		if (id < 0 || id >= m_global_texture_units.size())
			return glm::vec4(0.0f);
		const auto &texture = m_global_texture_units[id];
		if (texture->isGeneratedMipmap())
		{
			//Calculate lod level
			glm::vec2 dfdx = dUVdx * glm::vec2(texture->getWidth(), texture->getHeight());
			glm::vec2 dfdy = dUVdy * glm::vec2(texture->getWidth(), texture->getHeight());
			float L = glm::max(glm::dot(dfdx, dfdx), glm::dot(dfdy, dfdy));
			return texture->sample(uv, glm::max(0.5f * glm::log2(L), 0.0f));
		}
		else
		{
			return texture->sample(uv);
		}
	}

}