#include "GameScene.h"
#include "../SceneManager.h"
#include "../../Object/Ground/Ground .h"
#include "../../Object/Back/Back.h"
#include "../../Object/Player/Player.h"
#include "../../Object/Enemy/Enemy.h"
#include "../../Object/Jewelry/Jewelry.h"
#include "../../Object/GameUI/GameUI.h"
#include "../../Object/Item/Item.h"

void GameScene::Event()
{
	if (GetAsyncKeyState('A') & 0x0001)
	{
		// 現在の状態（Get）を反転（!）させた値をセット（Set）する
		bool nextState = !Enemy::GetShowDebugWire();
		Enemy::SetShowDebugWire(nextState);
	}

	if (GetAsyncKeyState('T') & 0x8000)
	{
		SceneManager::Instance().SetNextScene(SceneManager::SceneType::Title);
	}
	if (GetAsyncKeyState('G') & 0x0001)
	{
		Init();
	}
	if (GetAsyncKeyState('R') & 0x8000)
	{
		SceneManager::Instance().SetNextScene(SceneManager::SceneType::Result);
	}

	if (GetAsyncKeyState(VK_TAB) & 0x8000)
	{
		m_UI->SetVisibleJewelry(true); // UIを表示する

		// ★追加：プレイヤーに宝石を強制取得させる
		if (m_player)
		{
			m_player->SetHasJewelry(true);
		}
	}

	// キーガイド表示制御
	if (m_player && m_UI)
	{
		if (m_hasHiddenKeyGuide)
		{
			m_UI->SetVisibleKeyGuide(false);
		}
		else
		{
			float playerX = m_player->GetPos().x;

			if (playerX >= -1.0f && playerX != 0.0f) // マップサイズに合わせて調整
			{
				m_UI->SetVisibleKeyGuide(false);
				m_hasHiddenKeyGuide = true;
			}
			else
			{
				m_UI->SetVisibleKeyGuide(true);
			}
		}
	}

	// =============================================================
	// ★ プレイヤーが宝石を獲得した際、全9部屋の残りの敵を出現させる
	// =============================================================
	if (m_player && m_player->HasJewelry() && !m_hasSpawnedEnemies)
	{
		m_UI->SetVisibleJewelry(true);

		// 全9部屋（0〜8）の敵を一気に生成
		for (int i = 0; i < (int)m_rooms.size(); ++i)
		{
			SpawnEnemiesInRoom(i);
		}

		// これ以降、毎フレーム湧き続けないようにフラグを true にする
		m_hasSpawnedEnemies = true;
	}

	// カメラの追従・クランプ計算
	Math::Vector3 targetCamPos = Math::Vector3{ 0, 1.75f, -0.25f } + Math::Vector3{ m_player->GetPos().x, m_player->GetPos().y, m_player->GetPos().z - 1.75f };

	float minX = -100.0f;
	float maxX = 100.0f;
	float minY = 0.0f;
	float maxY = 100.3f;

	targetCamPos.x = std::clamp(targetCamPos.x, minX, maxX);
	targetCamPos.y = std::clamp(targetCamPos.y, minY, maxY);

	// 移動行列と回転行列を合成してカメラにセット
	Math::Matrix transmat = Math::Matrix::CreateTranslation(targetCamPos);
	Math::Matrix rotmat = Math::Matrix::CreateRotationX(DirectX::XMConvertToRadians(30));
	m_camera->SetCameraMatrix(rotmat * transmat);
}

void GameScene::SpawnEnemiesInRoom(int roomIndex)
{
	// 部屋番号が配列の範囲外なら何もしない（安全対策）
	if (roomIndex < 0 || roomIndex >= m_rooms.size()) return;

	// 指定された部屋のすべての座標に敵を生成する
	for (const auto& pos : m_rooms[roomIndex].spawnPositions)
	{
		auto newEnemy = std::make_shared<Enemy>();
		newEnemy->Init();
		newEnemy->SetTarget(m_player);
		newEnemy->SetPos(pos);

		m_objList.push_back(newEnemy);
	}
}

void GameScene::Init()
{
	m_objList.clear();
	m_rooms.clear();
	m_hasSpawnedEnemies = false;

	KdShaderManager::Instance().WorkAmbientController().SetFogEnable(true, false);
	KdShaderManager::Instance().WorkAmbientController().SetDistanceFog({ 0.0f,0.0f,0.0f }, 0.3f);

	m_camera = std::make_unique<KdCamera>();

	m_player = std::make_shared<Player>();
	m_objList.push_back(m_player);

	std::shared_ptr<Ground> ground = std::make_shared<Ground>();
	m_objList.push_back(ground);
	ground->SetPlayer(m_player);

	std::shared_ptr<Back> back = std::make_shared<Back>();
	m_objList.push_back(back);

	// =============================================================
	// 全9部屋の出現座標データを登録 (X: -2.2〜2.2 / Z: -1.5〜4.0)
	// =============================================================
	m_rooms.resize(9); // 3x3 = 9部屋分確保

	// --- 手前列 (Z: -1.5 〜 0.3) ---
	// 部屋 0 (手前・左)
	m_rooms[0].spawnPositions = { { -1.5f, 0.5f, -0.6f }, { -0.8f, 0.5f, -1.2f } };
	// 部屋 1 (手前・中央)
	m_rooms[1].spawnPositions = { {  0.0f, 0.5f, -0.6f } };
	// 部屋 2 (手前・右)
	m_rooms[2].spawnPositions = { {  1.5f, 0.5f, -0.6f }, {  1.8f, 0.5f, -1.2f } };

	// --- 中間列 (Z: 0.3 〜 2.1) ---
	// 部屋 3 (中央・左)
	m_rooms[3].spawnPositions = { { -1.5f, 0.5f,  1.2f } };
	// 部屋 4 (中央・中央)
	m_rooms[4].spawnPositions = { {  0.0f, 0.5f,  1.2f }, { -0.5f, 0.5f,  1.8f } };
	// 部屋 5 (中央・右)
	m_rooms[5].spawnPositions = { {  1.5f, 0.5f,  1.2f } };

	// --- 奥列 (Z: 2.1 〜 4.0) ---
	// 部屋 6 (奥・左)
	m_rooms[6].spawnPositions = { { -1.5f, 0.5f,  3.0f }, { -0.8f, 0.5f,  3.5f } };
	// 部屋 7 (奥・中央)
	//m_rooms[7].spawnPositions = { {  0.0f, 0.5f,  3.0f } };
	// 部屋 8 (奥・右)
	m_rooms[8].spawnPositions = { {  1.5f, 0.5f,  3.0f }, {  1.8f, 0.5f,  3.5f } };

	// =============================================================
	// ★ 宝石を取得する前：手前中央(部屋1)と中間中央(部屋4)に1体ずつ配置
	// =============================================================
	std::vector<int> initialSpawnRooms = { 1, 4 }; // ★ 1:手前中央, 4:中間中央
	for (int roomIdx : initialSpawnRooms)
	{
		if (!m_rooms[roomIdx].spawnPositions.empty())
		{
			auto enemy = std::make_shared<Enemy>();
			enemy->Init();
			enemy->SetTarget(m_player);
			enemy->SetPos(m_rooms[roomIdx].spawnPositions[0]);
			m_objList.push_back(enemy);
		}
	}

	// =============================================================
	// ★ アイテムと宝石の生成・配置
	// =============================================================
	std::vector<Math::Vector3> itemPositions = {
		{ 0.0f,0.0f,-2.90f },
		//{ 0.0f, 0.2f,  1.5f }
	};

	for (const auto& pos : itemPositions)
	{
		auto item = std::make_shared<Item>();
		item->Init();
		item->SetPos(pos);
		m_objList.push_back(item);
	}

	std::shared_ptr<Jewelry> jewelry = std::make_shared<Jewelry>();
	jewelry->SetTargetPlayer(m_player);
	m_objList.push_back(jewelry);

	m_UI = std::make_shared<GameUI>();
	m_objList.push_back(m_UI);

	m_hasHiddenKeyGuide = false;
}