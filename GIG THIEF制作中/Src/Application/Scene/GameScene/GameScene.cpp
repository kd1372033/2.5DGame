#include "GameScene.h"
#include "../SceneManager.h"
#include "../../Object/Ground/Ground .h"
#include "../../Object/Back/Back.h"
#include "../../Object/Player/Player.h"
#include "../../Object/Enemy/Enemy.h"
#include "../../Object/Jewelry/Jewelry.h"
#include "../../Object/GameUI/GameUI.h"
#include "../../Object/Item/Item.h"
#include "../../Object/Desk/Desk.h"

void GameScene::Event()
{
	if (GetAsyncKeyState('A') & 0x0001)
	{
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
		m_UI->SetVisibleJewelry(true); // UI

		// 宝石を強制取得させる
		if (m_player)
		{
			m_player->SetHasJewelry(true);
		}
	}

	// キーガイド表示制御
	if (m_player && m_UI)
	{
		Math::Vector3 playerPos = m_player->GetPos();

		// 最初の部屋（手前中央：中心 X=0.0, Z=-1.25）の範囲内判定
		// ※ 部屋の範囲：Xが -1.25 〜 1.25、かつ Zが 0.1 未満（扉手前まで）
		bool isInFirstRoom = (playerPos.z < 0.1f) && (playerPos.x >= -1.25f && playerPos.x <= 1.25f);

		// 最初の部屋にいる場合は表示、出ている場合は非表示
		m_UI->SetVisibleKeyGuide(isInFirstRoom);
	}

	// =============================================================
	// ★ プレイヤーが宝石を獲得した際、全9部屋の残りの敵を出現させる
	// =============================================================
	if (m_player && m_player->HasJewelry() && !m_hasSpawnedEnemies)
	{
		m_UI->SetVisibleJewelry(true);

		// 全9部屋の敵を出現させる（初期スポーン済みの部屋はスキップするか、未出現の枠のみ生成）
		for (int i = 0; i < (int)m_rooms.size(); ++i)
		{
			// 部屋1と部屋4の「最初の1体」は Init() で生成済みのためスキップ
			int startIdx = 0;
			if (i == 1 || i == 4)
			{
				startIdx = 1; // 2体目以降の座標があればそれを生成（1体目はスキップ）
			}

			// 指定したインデックス以降の敵を生成
			for (size_t j = startIdx; j < m_rooms[i].spawnPositions.size(); ++j)
			{
				auto newEnemy = std::make_shared<Enemy>();
				newEnemy->Init();
				newEnemy->SetTarget(m_player);
				newEnemy->SetPos(m_rooms[i].spawnPositions[j]);

				m_objList.push_back(newEnemy);
			}
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

	// 8/26追加
	m_spLightTexture = std::make_shared<KdTexture>();
	m_spLightTexture->Load("Asset/Textures/Light.png");


	m_player = std::make_shared<Player>();
	m_objList.push_back(m_player);

	std::shared_ptr<Ground> ground = std::make_shared<Ground>();
	m_objList.push_back(ground);
	ground->SetPlayer(m_player);

	// =============================================================
	// ★ 全9部屋に1つずつ Desk（引き出し）を配置
	// =============================================================
	struct DeskData
	{
		Math::Vector3 pos;
		float         rot;
	};

	std::vector<DeskData> deskList = {
		// --- 手前列 (Z = -0.70f) ---
		{{ -3.6f, 0.0f, -1.25f } , 90.0f }, // 部屋0 (手前・左)
		{{ 0.0f, 0.0f, -1.25f } , 180.0f }, // 部屋1 (手前・中央)
		{{ 3.5f, 0.0f, -1.25f } , -90.0f }, // 部屋2 (手前・右)

		// --- 中間列 (Z = 2.05f) ---
		{{ -3.5f, 0.0f, 1.50f } , 90.0f }, // 部屋3 (中央・左)
		{{ 0.0f, 0.0f, 1.50f } , 180.0f }, // 部屋4 (中央・中央)
		{{ 3.5f, 0.0f, 1.50f } , -90.0f }, // 部屋5 (中央・右)

		// --- 奥列 (Z = 4.55f) ---
		{ { -3.5f, 0.0f, 4.00f }, 90.0f }, // 部屋6 (奥・左)
		{ { 0.0f, 0.0f, 5.00f }, 180.0f }, // 部屋7 (奥・中央)
		{ { 3.5f, 0.0f, 4.00f }, -90.0f }  // 部屋8 (奥・右)
	};
	/*
		{ -2.5f, 0.0f, -1.25f }
		{ 0.0f, 0.0f, -1.25f }
		{ 2.5f, 0.0f, -1.25f }

		{ -2.5f, 0.0f, 1.50f }
		{ 0.0f, 0.0f, 1.50f }
		{ 2.5f, 0.0f, 1.50f }

		{ -2.5f, 0.0f, 4.00f }
		{ 0.0f, 0.0f, 4.00f }
		{ 2.5f, 0.0f, 4.00f }
	*/
	for (const auto& data : deskList)
	{
		auto desk = std::make_shared<Desk>();
		desk->Init();
		desk->SetPos(data.pos);
		desk->SetRot(data.rot); // ※ SetRotY の場合は SetRotY に変更してください
		desk->SetPlayer(m_player);

		m_objList.push_back(desk);
	}

	std::shared_ptr<Back> back = std::make_shared<Back>();
	m_objList.push_back(back);

	// =============================================================
	// ★ 各部屋の中央座標を基準にした敵の出現位置設定（部屋7は出現なし）
	// =============================================================
	m_rooms.resize(9); // 3x3 = 9部屋分

	// --- 手前列 (Z = -1.25f) ---
	// 部屋 0 (手前・左)
	m_rooms[0].spawnPositions = { { -3.0f, 0.0f, -1.5f }, { -2.0f, 0.0f, -1.0f } };
	// 部屋 1 (手前・中央)
	m_rooms[1].spawnPositions = { {  0.0f, 0.0f, -0.00f }, {  0.0f, 0.0f, -1.5f } };
	// 部屋 2 (手前・右)
	m_rooms[2].spawnPositions = { {  2.0f, 0.0f, -1.5f }, {  3.0f, 0.0f, -1.0f } };

	// --- 中間列 (Z = 1.50f) ---
	// 部屋 3 (中央・左)
	m_rooms[3].spawnPositions = { { -3.0f, 0.0f,  1.2f }, { -2.0f, 0.0f,  1.8f } };
	// 部屋 4 (中央・中央)
	m_rooms[4].spawnPositions = { {  0.0f, 0.0f,  1.5f }, { -0.5f, 0.0f,  1.8f } };
	// 部屋 5 (中央・右)
	m_rooms[5].spawnPositions = { {  2.0f, 0.0f,  1.2f }, {  3.0f, 0.0f,  1.8f } };

	// --- 奥列 (Z = 4.00f) ---
	// 部屋 6 (奥・左)
	m_rooms[6].spawnPositions = { { -3.0f, 0.0f,  3.7f }, { -2.0f, 0.0f,  4.3f } };

	// ★ 部屋 7 (奥・中央) : 敵を出現させない
	m_rooms[7].spawnPositions = {};

	// 部屋 8 (奥・右)
	m_rooms[8].spawnPositions = { {  2.0f, 0.0f,  3.7f }, {  3.0f, 0.0f,  4.3f } };

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
		//{ 0.0f,0.0f,-2.90f },
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

//void GameScene::DrawPostEffect()
//{
//	Math::Matrix viewProj = m_camera->GetCameraViewMatrix() * m_camera->GetProjMatrix();
//	Math::Matrix invViewProj = viewProj.Invert();
//
//	auto& postProcess = KdShaderManager::Instance().m_postProcessShader;
//
//	for (auto& obj : SceneManager::Instance().GetObjList())
//	{
//		if (auto enemy = std::dynamic_pointer_cast<Enemy>(obj))
//		{
//			Math::Matrix lightViewProj = enemy->GetLightViewProjectionMatrix();
//			postProcess.SetLightProjectorMatrix(invViewProj, lightViewProj);
//
//			postProcess.DrawLightProjector(
//				postProcess.GetPostEffectRTTex(),
//				postProcess.GetPostEffectZBuffer(),
//				m_spLightTexture, // ★ ロードした変数を渡す
//				postProcess.GetPostEffectRTTex()
//			);
//		}
//	}
//}

/*
部屋番号,位置,"中央座標 (X, Y, Z)"
部屋 0		手前・左	{ -2.5f, 0.0f, -1.25f }
部屋 1		手前・中央	{  0.0f, 0.0f, -1.25f }
部屋 2		手前・右	{  2.5f, 0.0f, -1.25f }
部屋 3		中間・左	{ -2.5f, 0.0f,  1.50f }
部屋 4		中間・中央	{  0.0f, 0.0f,  1.50f }
部屋 5		中間・右	{  2.5f, 0.0f,  1.50f }
部屋 6		奥・左		{ -2.5f, 0.0f,  4.00f }
部屋 7		奥・中央	{  0.0f, 0.0f,  4.00f }
部屋 8		奥・右		{  2.5f, 0.0f,  4.00f }
*/