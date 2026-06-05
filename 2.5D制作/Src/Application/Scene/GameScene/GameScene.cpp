#include "GameScene.h"
#include"../SceneManager.h"

#include "../../Object/Ground/Ground .h"
#include "../../Object/Player/Player.h"
#include "../../Object/Enemy/Enemy.h"


void GameScene::Event()
{
	if (GetAsyncKeyState('T') & 0x8000)
	{
		SceneManager::Instance().SetNextScene
		(
			SceneManager::SceneType::Title
		);
	}

	Math::Vector3 camPos = { 0,2,-4 };
	Math::Matrix transmat = Math::Matrix::CreateTranslation(camPos + m_player->GetPos());
	Math::Matrix rotmat = Math::Matrix::CreateRotationX(DirectX::XMConvertToRadians(30));
	m_camera->SetCameraMatrix(rotmat * transmat);
}

void GameScene::Init()
{
	m_camera = std::make_unique<KdCamera>();

	std::shared_ptr<Ground> ground;
	ground = std::make_shared<Ground>();
	m_objList.push_back(ground);


	m_player = std::make_shared<Player>();
	m_objList.push_back(m_player);

	std::shared_ptr<Enemy> enemy;
	enemy = std::make_shared<Enemy>();
	m_objList.push_back(enemy);
}
