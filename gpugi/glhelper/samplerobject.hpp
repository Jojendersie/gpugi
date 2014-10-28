#pragma once

#include "gl.hpp"
#include <ei/matrix.hpp>
#include <cinttypes>

#include <unordered_map>

namespace gl
{
	/// \brief Easy to use OpenGL Sampler Object
	///
	/// Will store all runtime create sampler objects in a intern static list. List will be cleared on program shutdown or on explicit call.
	/// Also binds sampler only if not already bound.
	class SamplerObject
	{
	public:
		enum class Filter
		{
			NEAREST,
			LINEAR
		};
		enum class Border
		{
			REPEAT = GL_REPEAT,
			MIRROR = GL_MIRRORED_REPEAT,
			CLAMP = GL_CLAMP_TO_EDGE,
			BORDER = GL_CLAMP_TO_BORDER
		};

		struct Desc
		{
			/// Constructs a descriptor with the same border handling for all dimensions.
			Desc(Filter minFilter, Filter magFilter, Filter mipFilter, Border borderHandling,
				unsigned int maxAnisotropy = 1, const ei::Vec3& borderColor = ei::Vec3(1.0f));

			/// Constructs a descriptor with the different border handling for each dimension.
			Desc(Filter minFilter, Filter magFilter, Filter mipFilter, Border borderHandlingU, Border borderHandlingV, Border m_borderHandlingW,
				unsigned int maxAnisotropy = 1, const ei::Vec3&borderColor = ei::Vec3(1.0f));

			bool operator == (const Desc& other) const { return memcmp(this, &other, sizeof(Desc)) == 0; }

			Filter minFilter;
			Filter magFilter;
			Filter mipFilter;
			Border borderHandlingU;
			Border borderHandlingV;
			Border borderHandlingW;
			unsigned int maxAnisotropy;
			ei::Vec3  borderColor;


			struct GetHash
			{
				size_t operator()(const gl::SamplerObject::Desc& desc) const
				{
					size_t hash = (static_cast<size_t>(desc.minFilter) << 0) + (static_cast<size_t>(desc.magFilter) << 1) + (static_cast<size_t>(desc.mipFilter) << 2);
					hash |= (static_cast<size_t>(desc.borderHandlingU) + static_cast<size_t>(desc.borderHandlingV) + static_cast<size_t>(desc.borderHandlingW) +
						reinterpret_cast<const uint64_t*>(&desc.maxAnisotropy)[0] + reinterpret_cast<const uint64_t*>(&desc.maxAnisotropy)[1]) << 3;
					return hash;
				}
			};
		};

		/// Will create or return a existing SamplerObject.
		static const SamplerObject& GetSamplerObject(const Desc& desc);

		/// Binds sampler to given texture stage if not already bound.
		void BindSampler(GLuint textureStage) const;

		/// Removes all existing sampler objects.
		void ClearAllCachedSamplerObjects() { s_existingSamplerObjects.clear(); }

		SamplerId GetInternSamplerId() const { return m_samplerId; }

		SamplerObject(SamplerObject&& cpy);
		~SamplerObject();

	private:
		SamplerObject(const Desc& samplerDesc);

		static const SamplerObject* s_pSamplerBindings[32];
		static std::unordered_map<Desc, SamplerObject, Desc::GetHash> s_existingSamplerObjects;

		SamplerId m_samplerId;
	};
};
