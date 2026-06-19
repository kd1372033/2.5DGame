#include "GameScene.h"
#include"../SceneManager.h"

#include "../../Object/Ground/Ground .h"
#include "../../Object/Back/Back.h"
#include "../../Object/Player/Player.h"
#include "../../Object/Enemy/Enemy.h"
#include "../../Object/Jewelry/Jewelry.h"


void GameScene::Event()
{
	if (GetAsyncKeyState('T') & 0x8000)
	{
		SceneManager::Instance().SetNextScene
		(
			SceneManager::SceneType::Result
		);
	}

	// =============================================================
	// ★プレイヤーが宝石を獲得し、かつ、まだ敵を増やしていない場合
	// =============================================================
	if (m_player && m_player->HasJewelry() && !m_hasSpawnedEnemies)
	{
		// 例として、敵を新たに5体追加してみます
		for (int i = 0; i < 5; ++i)
		{
			auto newEnemy = std::make_shared<Enemy>();
			newEnemy->Init();
			newEnemy->SetTarget(m_player); // プレイヤーを追尾ターゲットに設定

			// 出現位置の設定（例：プレイヤーの少し前方など、被らない位置に調整）
			Math::Vector3 spawnPos = m_player->GetPos();
			spawnPos.x += 5.0f + (i * 1.5f); // プレイヤーの右側に等間隔で並べる
			spawnPos.y = -1.6f;             // 地面の高さに合わせる
			newEnemy->SetPos(spawnPos);

			// オブジェクトリストに追加（これで自動的にUpdateやDrawが呼ばれるようになります）
			m_objList.push_back(newEnemy);
		}

		// ★重要：これ以降、毎フレーム敵が増え続けないようにフラグを true にする
		m_hasSpawnedEnemies = true;
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

	//auto bgm = KdAudioManager::Instance().Play("Asset/Sounds/BGM.wav", true); // ループ再生

	//if (bgm) {
	//	bgm->SetVolume(0.5f); // 音量を50%に
	//}

	std::shared_ptr<Ground> ground;
	ground = std::make_shared<Ground>();
	m_objList.push_back(ground);

	std::shared_ptr<Back> back;
	back = std::make_shared<Back>();
	m_objList.push_back(back);

	m_player = std::make_shared<Player>();
	m_objList.push_back(m_player);

	std::shared_ptr<Enemy> enemy;
	for (int i = 0; i < 5; ++i)
	{
		enemy = std::make_shared<Enemy>();
		enemy->Init();
		enemy->SetTarget(m_player);
		Math::Vector3 spawnpos;
		switch (i)
		{
		case 1:
			spawnpos = { -5.0f, 1.0f, -1.0f };
			break;
		case 2:
			spawnpos = { -5.0f, 1.0f, -0.3f };
			break;
		case 3:
			spawnpos = { -5.0f, 1.0f, -1.0f };
			break;
		case 4:
			spawnpos = { -5.0f, 1.0f, -1.0f };
			break;
		case 5:
			spawnpos = { -5.0f, 1.0f, -1.0f };
			break;
		default:
			break;
		}
		enemy->SetPos(spawnpos);
		m_objList.push_back(enemy);
	}

	std::shared_ptr<Jewelry> jewelry;
	jewelry = std::make_shared<Jewelry>();
	jewelry->SetTargetPlayer(m_player);
	m_objList.push_back(jewelry);

}
