#include "GameScene.h"
#include "../SceneManager.h"
#include "../../Object/Ground/Ground .h"
#include "../../Object/Back/Back.h"
#include "../../Object/Player/Player.h"
#include "../../Object/Enemy/Enemy.h"
#include "../../Object/Jewelry/Jewelry.h"
#include "../../Object/GameUI/GameUI.h"


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
		SceneManager::Instance().SetNextScene
		(
			SceneManager::SceneType::Title
		);
	}
	if (GetAsyncKeyState('G') & 0x0001)
	{
		Init();
	}
	if (GetAsyncKeyState('R') & 0x8000)
	{
		SceneManager::Instance().SetNextScene
		(
			SceneManager::SceneType::Result
		);
	}if (GetAsyncKeyState(VK_TAB) & 0x8000)
	{
		m_UI->SetVisibleJewelry(true); // UIを表示する（既存の処理）

		// ★追加：プレイヤーに宝石を強制取得させる
		if (m_player)
		{
			m_player->SetHasJewelry(true);
		}
	}

	// プレイヤーのX座標が -6 以下になったらUIを非表示にする
	// =============================================================
	// ★ プレイヤーが一度でも指定位置を超えたら、キーUIを完全に消す
	// =============================================================
	if (m_player && m_UI)
	{
		// すでに一度でも超えたことがあるなら、常に非表示にする
		if (m_hasHiddenKeyGuide)
		{
			m_UI->SetVisibleKeyGuide(false);
		}
		else
		{
			// 1フレーム目の「0.0f」のノイズを回避するため、
			// 初期位置（-8.0f付近）より確実に右に動いた（例: -7.0f）かつ
			// 座標がリセット状態（0.0fぴったり）ではない時だけ非表示フラグを立てる
			float playerX = m_player->GetPos().x;

			if (playerX >= -7.0f && playerX != 0.0f)
			{
				m_UI->SetVisibleKeyGuide(false);
				m_hasHiddenKeyGuide = true;
			}
			else
			{
				// 初期位置にいる間、および1フレーム目のノイズ時は確実に表示する
				m_UI->SetVisibleKeyGuide(true);
			}
		}
	}

	// =============================================================
	// ★ プレイヤーが宝石を獲得し、かつ、まだ1部屋目の敵を出していない場合
	// =============================================================
	if (m_player && m_player->HasJewelry() && !m_hasSpawnedEnemies)
	{
		m_UI->SetVisibleJewelry(true);

		// 宝石取得後、1部屋目の敵（5体）を一括出現させる
		SpawnEnemiesInRoom(0); // 1部屋目の5体
		SpawnEnemiesInRoom(1); // 2部屋目の残り（2体）
		SpawnEnemiesInRoom(2); // 3部屋目の残り（2体）

		// これ以降、毎フレーム湧き続けないようにフラグを true にする
		m_hasSpawnedEnemies = true;
	}

	// カメラの追従・クランプ計算
	Math::Vector3 targetCamPos = Math::Vector3{ 0, 0.75, -2.25 } + Math::Vector3{ m_player->GetPos().x, m_player->GetPos().y, -1.75 };

	float minX = -8.2f;    // ステージの左端の限界
	float maxX = 8.0f;     // ステージの右端の限界
	float minY = 0.0f;     // ステージの下端の限界
	float maxY = 0.3f;     // ステージの上端の限界

	targetCamPos.x = std::clamp(targetCamPos.x, minX, maxX);
	targetCamPos.y = std::clamp(targetCamPos.y, minY, maxY);

	// 移動行列と回転行列を合成してカメラにセット
	Math::Matrix transmat = Math::Matrix::CreateTranslation(targetCamPos);
	Math::Matrix rotmat = Math::Matrix::CreateRotationX(DirectX::XMConvertToRadians(20));
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

	KdShaderManager::Instance().WorkAmbientController().SetFogEnable(true, false);
	KdShaderManager::Instance().WorkAmbientController().SetDistanceFog({ 0.0f,0.0f,0.0f }, 0.3f);

	m_camera = std::make_unique<KdCamera>();

	std::shared_ptr<Ground> ground = std::make_shared<Ground>();
	m_objList.push_back(ground);

	std::shared_ptr<Back> back = std::make_shared<Back>();
	m_objList.push_back(back);

	m_player = std::make_shared<Player>();
	m_objList.push_back(m_player);

	// =============================================================
	// 全部屋の出現座標データを登録
	// =============================================================
	m_rooms.resize(3); // 3部屋分確保

	// 1部屋目 (5体)
	m_rooms[0].spawnPositions = {
		{ -7.25f, -1.6f, -1.25f },
		{ -8.25f, -1.6f, -1.5f  },
		{ -9.5f,  -1.6f, -0.6f  },
		{ -6.75f, -1.6f, -1.75f },
		{ -9.75f, -1.6f, -1.75f }
	};

	// 2部屋目 (3体)
	m_rooms[1].spawnPositions = {
		{ -2.2f, -1.6f, -1.75f },
		{ -4.2f, -1.6f, -0.6f },
		{ -2.0f, -1.6f, -1.0f }
	};

	// 3部屋目 (3体)
	m_rooms[2].spawnPositions = {
		{  2.2f, -1.6f, -0.7f  },
		{  3.2f, -1.6f, -2.0f  },
		{  1.4f, -1.6f, -1.25f }
	};

	// =============================================================
	// ★ 宝石を取得する前：1部屋目は敵なし、2・3部屋目に1体ずつ出現
	// =============================================================

	// リモコン（変数）を1つ宣言
	std::shared_ptr<Enemy> enemy;

	// 【2部屋目】 1回目のインスタンス生成
	enemy = std::make_shared<Enemy>();
	enemy->Init();
	enemy->SetTarget(m_player);
	enemy->SetPos(m_rooms[1].spawnPositions[0]);
	m_objList.push_back(enemy);

	// 【3部屋目】 ★2回目のインスタンス生成（ここで新しい肉体を作る！）
	enemy = std::make_shared<Enemy>();
	enemy->Init();
	enemy->SetTarget(m_player);
	enemy->SetPos(m_rooms[2].spawnPositions[0]);
	m_objList.push_back(enemy);


	std::shared_ptr<Jewelry> jewelry = std::make_shared<Jewelry>();
	jewelry->SetTargetPlayer(m_player);
	m_objList.push_back(jewelry);

	m_UI = std::make_shared<GameUI>();
	m_objList.push_back(m_UI);

	m_hasHiddenKeyGuide = false;
}