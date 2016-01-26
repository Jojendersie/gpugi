#include "importancevisualization.hpp"
#include "renderer/hierarchyimportance.hpp"
#include "renderer/renderersystem.hpp"
#include <glhelper/screenalignedtriangle.hpp>
#include <glhelper/buffer.hpp>
#include <glhelper/texture2d.hpp>

#include "control/scriptprocessing.hpp"
#include "control/globalconfig.hpp"

const std::string ImportanceVisualization::Name = "Importance Visualization";
const int VISSHADER_LOCAL_SIZE_X = 8;
const int VISSHADER_LOCAL_SIZE_Y = 8;

ImportanceVisualization::ImportanceVisualization(Renderer& _parentRenderer) :
	DebugRenderer(_parentRenderer),
	m_screenTri(new gl::ScreenAlignedTriangle()),
	m_infoShader("RayTraceImportanceVis")
{
	if(dynamic_cast<HierarchyImportance*>(&m_parentRenderer))
	{
		m_infoShader.AddShaderFromFile(gl::ShaderObject::ShaderType::VERTEX, "shader/utils/screenTri.vert");
		m_infoShader.AddShaderFromFile(gl::ShaderObject::ShaderType::FRAGMENT, "shader/hierarchy/trianglevis.frag");
		m_infoShader.CreateProgram();
	} else {
		LOG_ERROR("Importance visualization not supported for renderers other than HierarchyImportance!");
	}
}

ImportanceVisualization::~ImportanceVisualization()
{
}

void ImportanceVisualization::Draw()
{
	HierarchyImportance* parentRenderer = dynamic_cast<HierarchyImportance*>(&m_parentRenderer);
	if(parentRenderer)
	{
		m_parentRenderer.Draw();

		m_infoShader.Activate();
		//GL_CALL(glUniform1i, 101, parentRenderer->GetNumImportanceIterations());
		m_screenTri->Draw();
	}
}