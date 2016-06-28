#include "hierarchyvisualization.hpp"
#include <glhelper/buffer.hpp>
#include <glhelper/vertexarrayobject.hpp>
#include <glhelper/statemanagement.hpp>

#include "../../scene/scene.hpp"

#include "../../control/scriptprocessing.hpp"
#include "../../control/globalconfig.hpp"

#include "../hierarchyimportance.hpp"
#include "../renderersystem.hpp"

const std::string HierarchyVisualization::Name = "Hierarchy Visualization";

HierarchyVisualization::HierarchyVisualization(Renderer& _parentRenderer) :
	DebugRenderer(_parentRenderer),
	m_shader("HierarchyVisualization")
{
	// For HierarchyImportance renderer use importance for coloring.
	HierarchyImportance* hierarchyImportance = dynamic_cast<HierarchyImportance*>(&_parentRenderer);
	m_showImportance = hierarchyImportance != nullptr;
	if(hierarchyImportance)
		hierarchyImportance->UpdateHierarchyNodeImportance();

	ei::Vec3 boxVertices[] = {
		ei::Vec3(0.0f, 1.0f, 0.0f),
		ei::Vec3(1.0f, 1.0f, 0.0f),
		ei::Vec3(0.0f, 0.0f, 0.0f),
		ei::Vec3(1.0f, 0.0f, 0.0f),
		ei::Vec3(0.0f, 1.0f, 1.0f),
		ei::Vec3(1.0f, 1.0f, 1.0f),
		ei::Vec3(0.0f, 0.0f, 1.0f),
		ei::Vec3(1.0f, 0.0f, 1.0f),
	};
	m_boxVertexBuffer = std::make_unique<gl::Buffer>(sizeof(ei::Vec3) * 8, gl::Buffer::IMMUTABLE, boxVertices);

	std::uint32_t triIndices[] =
	{
		0, 1, 2,    // side 1
		2, 1, 3,
		4, 0, 6,    // side 2
		6, 0, 2,
		7, 5, 6,    // side 3
		6, 5, 4,
		3, 1, 7,    // side 4
		7, 1, 5,
		4, 5, 0,    // side 5
		0, 5, 1,
		3, 7, 2,    // side 6
		2, 7, 6,
	};
	m_boxSolidIndices = std::make_unique<gl::Buffer>(sizeof(std::uint32_t) * 6 * 2 * 3, gl::Buffer::IMMUTABLE, triIndices);

	std::uint32_t lineIndices[12 * 2] =
	{
		0, 1, 1, 3, 3, 2, 2, 0,
		4, 5, 5, 7, 7, 6, 6, 4,
		1, 5, 0, 4, 2, 6, 3, 7
	};
	m_boxLineIndices = std::make_unique<gl::Buffer>(sizeof(std::uint32_t) * 12 * 2, gl::Buffer::IMMUTABLE, lineIndices);


	typedef gl::VertexArrayObject::Attribute Attrib;
	m_boxVAO.reset(new gl::VertexArrayObject(
	{
		Attrib(Attrib::Type::FLOAT, 3, 0),
		Attrib(Attrib::Type::UINT32, 1, 1, Attrib::IntegerHandling::INTEGER),
		Attrib(Attrib::Type::UINT32, 1, 1, Attrib::IntegerHandling::INTEGER),
	}, { 0, 1 }));

	GlobalConfig::AddParameter("hvis_range", { 0, 4 }, "Hierarchy range displayed by HierachyVisualization (first min inclusive, second max exclusive)");
	GlobalConfig::AddListener("hvis_range", "HierachyVisualization", [](const GlobalConfig::ParameterType& p){
		if (p[0].As<int>() >= p[1].As<int>())
		{
			LOG_ERROR("Invalid setting for \"hvis_range\". First value is min and must always be smaller than the max value (second).");
		}
	});
	GlobalConfig::AddParameter("hvis_solid", { false }, "Hierarchy range drawing mode, true for solid (with zbuffer), false for lines.");
}

HierarchyVisualization::~HierarchyVisualization()
{
	GlobalConfig::RemoveParameter("hvis_range");
	GlobalConfig::RemoveParameter("hvis_solid");
}

void HierarchyVisualization::SetScene(std::shared_ptr<Scene> _scene)
{
	RecompileShaders(_scene->GetBvhTypeDefineString());

	// Create list of instances.
	std::unique_ptr<Instance[]> instances(new Instance[_scene->GetNumInnerNodes()]);
	for (unsigned int i = 0; i < _scene->GetNumInnerNodes(); ++i)
	{
		instances[i].nodeIndex = i;
		instances[i].depth = std::numeric_limits<std::uint32_t>::max();
	}
	instances[0].depth = 0;

	// Find out the depth of each instance
	std::vector<Instance*> pathStack;
	pathStack.reserve(128); // Just a hopefully overestimated guess.
	for (unsigned int i = 1; i < _scene->GetNumInnerNodes(); ++i)
	{
		if (instances[i].depth == std::numeric_limits<std::uint32_t>::max()) // yet unknown
		{
			Instance* parentInst = &instances[i];
			while (parentInst->depth == std::numeric_limits<std::uint32_t>::max())
			{
				pathStack.push_back(parentInst);
				parentInst = &instances[_scene->GetHierarchyParentsRAM()[parentInst->nodeIndex]];
			}

			std::uint32_t depthOffset = parentInst->depth;

			while (!pathStack.empty())
			{
				pathStack.back()->depth = ++depthOffset;
				pathStack.pop_back();
			}
		}
	}

	// Sort instances by their depth.
	std::sort(instances.get(), instances.get() + _scene->GetNumInnerNodes(), [](const Instance& a, const Instance& b) { return a.depth < b.depth; });

	// Create offset list for different hierarchy level.
	m_hierachyLevelOffset.push_back(0);
	for (unsigned int i = 1; i < _scene->GetNumInnerNodes(); ++i)
	{
		if (instances[i-1].depth != instances[i].depth)
			m_hierachyLevelOffset.push_back(i);
	}
	LOG_LVL2("Scene hierarchy has a total of " << _scene->GetNumInnerNodes() << " inner nodes, distributed over " << m_hierachyLevelOffset.size() << " levels.");
	m_hierachyLevelOffset.push_back(_scene->GetNumInnerNodes());

	// Upload
	m_instanceBuffer = std::make_unique<gl::Buffer>(sizeof(Instance) * _scene->GetNumInnerNodes(), gl::Buffer::IMMUTABLE, instances.get());
}

void HierarchyVisualization::Draw()
{
	GL_CALL(glClear, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	bool solid = GlobalConfig::GetParameter("hvis_solid")[0].As<bool>();  

	auto levelrange = GlobalConfig::GetParameter("hvis_range");
	auto rangeStart = levelrange[0].As<int>();
	auto rangeEnd = std::min(static_cast<int>(m_hierachyLevelOffset.size() - 2), levelrange[1].As<int>());
	if (rangeStart > m_hierachyLevelOffset.size() - 2 || rangeStart >= rangeEnd)
		return;

	gl::MappedUBOView memoryView(m_shader.GetUniformBufferInfo()["DebugSettings"], m_settingsUBO->Map(gl::Buffer::MapType::WRITE, gl::Buffer::MapWriteFlag::INVALIDATE_BUFFER));
//	memoryView["IterationCount"].Set(static_cast<std::int32_t>(m_parentRenderer.GetRendererSystem().GetIterationCount()));
	memoryView["MinDepth"].Set(static_cast<std::int32_t>(rangeStart));
	memoryView["MaxDepth"].Set(static_cast<std::int32_t>(rangeEnd));
	m_settingsUBO->Unmap();


	m_shader.Activate();

	m_boxVAO->Bind();
	m_boxVertexBuffer->BindVertexBuffer(0, 0, sizeof(ei::Vec3));
	m_instanceBuffer->BindVertexBuffer(1, 0, sizeof(Instance));
	
	if(solid)
	{
		gl::Enable(gl::Cap::DEPTH_TEST);
		m_boxSolidIndices->BindIndexBuffer();
		GL_CALL(glDrawElementsInstancedBaseInstance, GL_TRIANGLES, 6 * 2 * 3, GL_UNSIGNED_INT, nullptr,
			m_hierachyLevelOffset[rangeEnd] - m_hierachyLevelOffset[rangeStart], m_hierachyLevelOffset[rangeStart]);
		gl::Disable(gl::Cap::DEPTH_TEST);
	}
	else
	{
		m_boxLineIndices->BindIndexBuffer();
		GL_CALL(glDrawElementsInstancedBaseInstance, GL_LINES, 12 * 2, GL_UNSIGNED_INT, nullptr,
			m_hierachyLevelOffset[rangeEnd] - m_hierachyLevelOffset[rangeStart], m_hierachyLevelOffset[rangeStart]);
	}
		
}

void HierarchyVisualization::RecompileShaders(const std::string& _additionalDefines)
{
	if(m_showImportance)
	{
		m_shader.AddShaderFromFile(gl::ShaderObject::ShaderType::VERTEX, "shader/debug/hierarchybox.vert", "#define SHOW_NODE_IMPORTANCE\n" + _additionalDefines);
		m_shader.AddShaderFromFile(gl::ShaderObject::ShaderType::FRAGMENT, "shader/debug/hierarchybox_importance.frag", _additionalDefines);
	}
	else
	{
		m_shader.AddShaderFromFile(gl::ShaderObject::ShaderType::VERTEX, "shader/debug/hierarchybox.vert", _additionalDefines);
		m_shader.AddShaderFromFile(gl::ShaderObject::ShaderType::FRAGMENT, "shader/debug/hierarchybox.frag", _additionalDefines);
	}
	m_shader.CreateProgram();

	m_settingsUBO = std::make_unique<gl::Buffer>(m_shader.GetUniformBufferInfo()["DebugSettings"].bufferDataSizeByte, gl::Buffer::UsageFlag::MAP_WRITE);
	m_settingsUBO->BindUniformBuffer(8);
}
