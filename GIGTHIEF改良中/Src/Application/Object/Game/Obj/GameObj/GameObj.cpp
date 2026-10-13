#include "GameObj.h"

void Furniture::Init()
{
	Math::Vector3 scale(1.0f, 1.0f, 1.0f);
	Math::Matrix scalemat = Math::Matrix::CreateScale(scale);
	Math::Matrix rotatemat = Math::Matrix::CreateRotationY(DirectX::XMConvertToRadians(90));
	Math::Matrix transmat = Math::Matrix::CreateTranslation(0.0f, 0.0f, -1.0f);

	m_mWorld = scalemat * rotatemat * transmat;

	m_model = std::make_shared<KdModelData>();
	m_model->Load("Asset/Models/Kagu/Kagu.gltf");
	if (m_model->Load("Asset/Models/Kagu/Kagu.gltf")) {
		m_pCollider->RegisterCollisionShape("Furniture", m_model, KdCollider::TypeGround);
	}

}

void Furniture::Update()
{

}

void Furniture::DrawLit()
{
	if (!m_model) return;
	KdShaderManager::Instance().m_StandardShader.DrawModel(*m_model, m_mWorld);
}
