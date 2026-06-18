#include "GameScene.h"
#include"../SceneManager.h"

#include "../../Object/Ground/Ground .h"
#include "../../Object/Back/Back.h"
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

	Math::Vector3 targetCamPos = Math::Vector3{ 0, 0.5, -2 } + m_player->GetPos();

	float minX = -8.2;     // ステージの左端の限界
	float maxX = 10.0f;   // ステージの右端の限界
	float minY = 0.0f;     // ステージの下端の限界 (落とし穴などがある場合)
	float maxY = 0.3f;    // ステージの上端の限界

	targetCamPos.x = std::clamp(targetCamPos.x, minX, maxX);
	targetCamPos.y = std::clamp(targetCamPos.y, minY, maxY);
	// サイドビューなので Z軸(奥行き) は固定でOK

	// 3. クランプされた座標を使って移動行列を作成
	Math::Matrix transmat = Math::Matrix::CreateTranslation(targetCamPos);

	// 4. 回転行列を作成 (30度見下ろし)
	Math::Matrix rotmat = Math::Matrix::CreateRotationX(DirectX::XMConvertToRadians(20));
	
	// 5. 合成してカメラにセット
	m_camera->SetCameraMatrix(rotmat * transmat);
}

void GameScene::Init()
{
	KdShaderManager::Instance().WorkAmbientController().SetFogEnable(true, false);
	KdShaderManager::Instance().WorkAmbientController().SetDistanceFog({ 0,0,0 }, 0.25);


	m_camera = std::make_unique<KdCamera>();

	std::shared_ptr<Ground> ground;
	ground = std::make_shared<Ground>();
	m_objList.push_back(ground);

	std::shared_ptr<Back> back;
	back = std::make_shared<Back>();
	m_objList.push_back(back);

	m_player = std::make_shared<Player>();
	m_objList.push_back(m_player);

	std::shared_ptr<Enemy> enemy;
	for (int i = 0; i < 10; ++i)
	{
		enemy = std::make_shared<Enemy>();
		enemy->Init();
		enemy->SetTarget(m_player);
		Math::Vector3 spawnpos;
		enemy->SetPos(spawnpos);
		m_objList.push_back(enemy);
	}
	
}
