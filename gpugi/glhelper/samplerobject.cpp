#include "SamplerObject.hpp"
#include "../utilities/assert.hpp"

namespace gl
{
	std::unordered_map<SamplerObject::Desc, SamplerObject, SamplerObject::Desc::GetHash> SamplerObject::s_existingSamplerObjects;
	const SamplerObject* SamplerObject::s_pSamplerBindings[32];

	SamplerObject::Desc::Desc(Filter minFilter, Filter magFilter, Filter mipFilter,
		Border borderHandling, unsigned int maxAnisotropy, const ei::Vec3& borderColor) :
		Desc(minFilter, magFilter, mipFilter, borderHandling, borderHandling, borderHandling, maxAnisotropy, borderColor)
	{}

	SamplerObject::Desc::Desc(Filter minFilter, Filter magFilter, Filter mipFilter,
		Border borderHandlingU, Border borderHandlingV, Border m_borderHandlingW,
		unsigned int maxAnisotropy, const ei::Vec3& borderColor) :
		minFilter(minFilter),
		magFilter(magFilter),
		mipFilter(mipFilter),
		borderHandlingU(borderHandlingU),
		borderHandlingV(borderHandlingV),
		borderHandlingW(m_borderHandlingW),
		maxAnisotropy(maxAnisotropy),
		borderColor(borderColor)
	{
	}

	SamplerObject::SamplerObject(SamplerObject&& cpy) :
		m_samplerId(cpy.m_samplerId)
	{
		cpy.m_samplerId = std::numeric_limits<GLuint>::max();
	}


	SamplerObject::SamplerObject(const Desc& desc)
	{
		Assert(desc.maxAnisotropy > 0, "Anisotropy level of 0 is invalid! Must be between 1 and GPU's max.");

		GL_CALL(glGenSamplers, 1, &m_samplerId);
		GL_CALL(glSamplerParameteri, m_samplerId, GL_TEXTURE_WRAP_S, static_cast<GLint>(desc.borderHandlingU));
		GL_CALL(glSamplerParameteri, m_samplerId, GL_TEXTURE_WRAP_T, static_cast<GLint>(desc.borderHandlingV));
		GL_CALL(glSamplerParameteri, m_samplerId, GL_TEXTURE_WRAP_R, static_cast<GLint>(desc.borderHandlingW));

		GLint minFilterGl;
		if (desc.minFilter == Filter::NEAREST)
			minFilterGl = desc.mipFilter == Filter::NEAREST ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST_MIPMAP_LINEAR;
		else
			minFilterGl = desc.mipFilter == Filter::NEAREST ? GL_LINEAR_MIPMAP_NEAREST : GL_LINEAR_MIPMAP_LINEAR;

		GL_CALL(glSamplerParameteri, m_samplerId, GL_TEXTURE_MIN_FILTER, minFilterGl);
		GL_CALL(glSamplerParameteri, m_samplerId, GL_TEXTURE_MAG_FILTER, desc.magFilter == Filter::NEAREST ? GL_NEAREST : GL_LINEAR);

		GL_CALL(glSamplerParameteri, m_samplerId, GL_TEXTURE_MAX_ANISOTROPY_EXT, desc.maxAnisotropy);

		GL_CALL(glSamplerParameterfv, m_samplerId, GL_TEXTURE_BORDER_COLOR, desc.borderColor.m_data);
	}

	SamplerObject::~SamplerObject()
	{
		if (m_samplerId != std::numeric_limits<GLuint>::max())
			GL_CALL(glDeleteSamplers, 1, &m_samplerId);
	}

	const SamplerObject& SamplerObject::GetSamplerObject(const Desc& desc)
	{
		auto it = s_existingSamplerObjects.find(desc);
		if (it == s_existingSamplerObjects.end())
		{
			auto newElem = s_existingSamplerObjects.emplace(desc, SamplerObject(desc));
			return newElem.first->second;
		}

		return it->second;
	}

	void SamplerObject::BindSampler(unsigned int textureStage) const
	{
		Assert(textureStage < sizeof(s_pSamplerBindings) / sizeof(SamplerObject*), "Can't bind sampler to slot " << textureStage << " .Maximum number of slots is " << sizeof(s_pSamplerBindings) / sizeof(SamplerObject*));
		if (s_pSamplerBindings[textureStage] != this)
		{
			glBindSampler(textureStage, m_samplerId);
			s_pSamplerBindings[textureStage] = this;
		}
	}
}