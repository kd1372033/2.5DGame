#include "Player.h"

#include "../../Scene/SceneManager.h"
#include "../Item/Item.h"
#include "../Jewelry/Jewelry.h"


void Player::Init()
{
	m_pDebugWire = std::make_unique<KdDebugWireFrame>();

	m_polygon = std::make_shared<KdSquarePolygon>();

	m_polygon->SetMaterial("Asset/Textures/Player.png");

	m_polygon->SetSplit(8, 6);

	m_polygon->SetPivot(KdSquarePolygon::PivotType::Center_Bottom);

	//m_pos = { -10.25,-1.6,-1.75 };
	//m_pos = { 0.0f,0.0f,4.0f };
	//m_pos = { 0.0f,0.0f,4.0f };
	m_pos = { 0.0f,0.0f,-2.90f };


	// 当たり判定登録
	m_pCollider = std::make_unique<KdCollider>();
	m_pCollider->RegisterCollisionShape("Player", { 0,0.27,0 }, 0.25, KdCollider::Type::TypeSight);
}

void Player::Update()
{
	// デバッグ表示切り替え
	if (GetAsyncKeyState('A') & 0x0001)
	{
		m_showDebugWire = !m_showDebugWire;
	}

	bool isMoving = false;

	// 1. キー入力と移動処理
	UpdateInput(isMoving);

	// 2. 移動アニメーションとUV切り替え
	UpdateAnimation(isMoving);

	// 3. アイテム処理（自動拾い・投げ）
	UpdateItemActions();

	// 5. 重力の加算
	m_pos.y -= m_gravity;
	m_gravity += 0.005f;

	// ワールド行列の更新
	Math::Matrix scalemat = Math::Matrix::CreateScale(0.5f);
	Math::Matrix transmat = Math::Matrix::CreateTranslation(m_pos);
	m_mWorld = scalemat * transmat;


	// 端判定
	//if (m_pos.z <= -2.25f)
	//{
	//	m_pos.z = -2.25f;
	//}
	//if (m_pos.z >= -0.3f)
	//{
	//	m_pos.z = -0.3f;
	//}
	if (m_pos.z <= -3.4f)
	{

		if (m_hasJewelry)
		{
			// ★【修正】リザルトへ移行するので、プレイヤーの足音を確実に止める
			if (m_walkSound && m_walkSound->IsPlaying())
			{
				m_walkSound->Stop();
			}

			SceneManager::Instance().SetClearFlag(true);
			SceneManager::Instance().SetNextScene(SceneManager::SceneType::Result);
		}
	}
}

void Player::PostUpdate()
{
	// 1. レイ判定（着地・高さ調整）
	CheckRayCollision();

	// 2. スフィア判定（壁の押し戻し）
	CheckSphereCollision();

	// 補正後の位置をワールド行列へ最終反映
	Math::Matrix scalemat = Math::Matrix::CreateScale(0.5f);
	Math::Matrix transmat = Math::Matrix::CreateTranslation(m_pos);
	m_mWorld = scalemat * transmat;
}

// =============================================================
// 描画
// =============================================================
void Player::GenerateDepthMapFromLight()
{
	KdShaderManager::Instance().m_StandardShader.DrawPolygon(*m_polygon, m_mWorld);

}

void Player::DrawLit()
{
	// プレイヤーモデル / ポリゴンの描画
	if (m_polygon)
	{
		KdShaderManager::Instance().m_StandardShader.DrawPolygon(*m_polygon, m_mWorld);
	}
}

void Player::DrawUnLit()
{
	// ★ Zテスト（深度判定）のみを無効化し、ZWriteはそのままにする
	KdShaderManager::Instance().ChangeDepthStencilState(KdDepthStencilState::ZDisable);

	// プレイヤーモデル / ポリゴンの描画
	if (m_polygon)
	{
		KdShaderManager::Instance().m_StandardShader.DrawPolygon(*m_polygon, m_mWorld);
	}

	// ステートを戻す
	KdShaderManager::Instance().UndoDepthStencilState();
}

// =========================================================
// 投げる処理の修正（生成から所持アイテムの放擲へ変更）
// =========================================================
void Player::ThrowItem()
{
	// 1. 手持ちのアイテムが無いが、所持数（ストック）がある場合は新規生成する
	if (!m_pHeldItem && m_throwableCount > 0)
	{
		auto newItem = std::make_shared<Item>();
		newItem->Init();
		newItem->SetPos(m_pos); // プレイヤーの足元/手元に配置

		// シーンに追加
		SceneManager::Instance().AddObject(newItem);

		// 手元に持っている状態にする
		m_pHeldItem = newItem;
		m_pHeldItem->PickUp(std::dynamic_pointer_cast<Player>(shared_from_this()));

		// ストックを消費
		m_throwableCount--;
	}

	if (!m_pHeldItem)
	{
		return;
	}

	// 2. 向いている方向（m_dirID）からベクトルを作成
	Math::Vector3 throwDir = { 0.0f, 0.0f, 0.0f };
	switch (m_dirID)
	{
	case 0:
	{
		throwDir.z = -1.0f; // 下
		break;
	}
	case 1:
	{
		throwDir.x = -1.0f; // 左
		break;
	}
	case 2:
	{
		throwDir.x = 1.0f;  // 右
		break;
	}
	case 3:
	{
		throwDir.z = 1.0f;  // 上
		break;
	}
	default:
	{
		break;
	}
	}

	// 3. 所持しているアイテムの向きを設定して投げる
	m_pHeldItem->SetDir(throwDir);
	m_pHeldItem->StartThrow();

	// 4. プレイヤーの手元から離す（参照を解除）
	m_pHeldItem = nullptr;
}

// =========================================================
// 拾う処理の実装
// =========================================================
void Player::PickUpItem()
{
	// シーン上の全オブジェクトから「拾えるItem」を探す
	for (auto& obj : SceneManager::Instance().GetObjList())
	{
		auto item = std::dynamic_pointer_cast<Item>(obj);
		if (!item) continue;

		// 既に持たれている、または投げられて飛行中のアイテムは拾わない
		if (item->IsHeld() || item->IsThrown()) continue;

		// プレイヤーとアイテムの距離を計算（重なった判定：0.4〜0.5m程度）
		float dist = (item->GetPos() - m_pos).Length();

		// 重なったら自動で拾う
		if (dist <= 0.25f)
		{
			m_pHeldItem = item;
			m_pHeldItem->PickUp(std::dynamic_pointer_cast<Player>(shared_from_this()));

			// 拾った時のSEなどを鳴らす場合はここに追加
			auto se = KdAudioManager::Instance().Play("Asset/Sounds/Get.wav", false);
			if (se) {
				se->SetVolume(0.1f);
			}

			break; // 1つ拾ったら終了
		}
	}
}

// =============================================================
// Update 内部処理関数
// =============================================================

// --- 1. キー入力・移動ベクトル制御 ---
void Player::UpdateInput(bool& outIsMoving)
{
	m_dir = { 0, 0, 0 }; // 初期化
	outIsMoving = false;

	// 1. 入力検知と移動ベクトルの設定
	if (GetAsyncKeyState(VK_UP) & 0x8000)
	{
		m_dir.z += 1;
		m_dirID = 3; // 上を向く
		outIsMoving = true;
	}
	if (GetAsyncKeyState(VK_DOWN) & 0x8000)
	{
		m_dir.z += -1;
		m_dirID = 0; // 下を向く
		outIsMoving = true;
	}
	if (GetAsyncKeyState(VK_LEFT) & 0x8000)
	{
		m_dir.x += -1;
		m_dirID = 1; // 左を向く
		outIsMoving = true;
	}
	if (GetAsyncKeyState(VK_RIGHT) & 0x8000)
	{
		m_dir.x += 1;
		m_dirID = 2; // 右を向く
		outIsMoving = true;
	}

	// 2. 移動処理（斜め移動の速度を一定にするために正規化）
	if (outIsMoving)
	{
		m_dir.Normalize();
		m_pos += m_dir * m_speed;

		// アニメーションタイマーを進める（移動時は8フレームでループ）
		m_anime += 0.1;
		if (m_anime >= 8.0) m_anime = 0.0;

		if (!m_walkSound || !m_walkSound->IsPlaying())
		{
			m_walkSound = KdAudioManager::Instance().Play("Asset/Sounds/Walk.wav", false);
			if (m_walkSound) {
				m_walkSound->SetVolume(0.05f);
			}
		}
	}
	else
	{
		// 待機時は4フレームでループ
		m_anime += 0.1; // 待機は少しゆっくりめ
		if (m_anime >= 4.0) m_anime = 0.0;
		if (m_walkSound && m_walkSound->IsPlaying())
		{
			m_walkSound->Stop();
		}
	}
}

// --- 2. UVアニメーション切り替え ---
void Player::UpdateAnimation(bool isMoving)
{
	// 現在の状態に応じてUVを設定
	if (isMoving)
	{
		const int Run[4][8] = {
		{ 16,17,18,19,20,21,22,23 }, // 0:下
		{ 8,9,10,11,12,13,14,15 },   // 1:左
		{ 0,1,2,3,4,5,6,7 },         // 2:右
		{ 24,25,26,27,28,29,30,31 }  // 3:上
		};
		int RunFrame = (int)m_anime % 8; // 0～7のフレーム番号
		m_polygon->SetUVRect(Run[m_dirID][RunFrame]);
	}
	else
	{
		const int Wait[4][4] = {
		{ 32,33,34,35 }, // 0:下
		{ 40,41,42,43 }, // 1:左
		{ 36,37,38,39 }, // 2:右
		{ 44,45,46,47 }  // 3:上
		};

		int WaitFrame = (int)m_anime % 4; // 0～3のフレーム番号
		m_polygon->SetUVRect(Wait[m_dirID][WaitFrame]);
	}
}

// --- 3. アイテムの拾う・投げる ---
void Player::UpdateItemActions()
{
	// 自動拾い処理（持っていない時は常に接触チェック）
	if (!m_pHeldItem)
	{
		PickUpItem();
	}

	// アイテム投げ（スペースキー）
	static bool s_isThrowKeyPressed = false;
	bool isCurrentKeyPress = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;

	if (isCurrentKeyPress)
	{
		if (!s_isThrowKeyPressed)
		{
			// 持っているアイテムがある、または机などで獲得したストック（所持数）がある場合
			if (m_pHeldItem || m_throwableCount > 0)
			{
				auto se = KdAudioManager::Instance().Play("Asset/Sounds/Throw.wav", false);
				if (se)
				{
					se->SetVolume(0.05f);
				}

				ThrowItem();
			}

			s_isThrowKeyPressed = true;
		}
	}
	else
	{
		s_isThrowKeyPressed = false;
	}
}

// =============================================================
// PostUpdate 内部処理関数（当たり判定）
// =============================================================

// --- レイ判定（地面判定） ---
void Player::CheckRayCollision()
{
	// ==============
	//	レイ判定（着地・高さ調整）
	// ==============
	KdCollider::RayInfo ray;

	// ★ 足元より少し上から発射
	float enableStepHigh = 0.5f;
	ray.m_pos = m_pos;
	ray.m_pos.y += enableStepHigh;

	ray.m_dir = { 0, -1, 0 }; // 真下

	// ★ 判定距離を十分に持たせる
	ray.m_range = enableStepHigh + m_gravity + 0.2f;
	ray.m_type = KdCollider::TypeGround;

	if (m_showDebugWire)
	{
		m_pDebugWire->AddDebugLine(ray.m_pos, ray.m_dir, ray.m_range);
	}

	std::list<KdCollider::CollisionResult> retRayList;
	for (auto& obj : SceneManager::Instance().GetObjList())
	{
		if (obj.get() == this) continue; // 自分自身を除外
		if (std::dynamic_pointer_cast<Item>(obj)) continue;

		obj->Intersects(ray, &retRayList);
	}

	float maxOverLap = 0;
	Math::Vector3 hitPos;
	bool hit = false;

	for (auto& ret : retRayList)
	{
		if (maxOverLap < ret.m_overlapDistance)
		{
			maxOverLap = ret.m_overlapDistance;
			hitPos = ret.m_hitPos;
			hit = true;
		}
	}

	if (hit == true)
	{
		// ★【一番の修正ポイント！】
		// 1. hitPos.y（地面の高さ）をそのまます直代入する（-0.1f などのオフセットは入れない！）
		// 2. X, Z 座標はプレイヤー自身の移動位置を維持する（hitPosで上書きしない）
		m_pos.y = hitPos.y;

		// 着地したので重力をリセット
		m_gravity = 0;
	}
}

// --- スフィア判定（壁押し戻し） ---
void Player::CheckSphereCollision()
{
	// ==============
	//	球(スフィア)判定（壁との押し戻し）
	// ==============
	KdCollider::SphereInfo sphere;
	sphere.m_sphere.Center = m_pos;
	sphere.m_sphere.Center.y += 0.27f; // 中心座標を少し高めに設定
	sphere.m_sphere.Radius = 0.1f;
	sphere.m_type = KdCollider::Type::TypeGround; // そのままでOK

	if (m_showDebugWire)
	{
		m_pDebugWire->AddDebugSphere(sphere.m_sphere.Center, sphere.m_sphere.Radius);
	}

	std::list<KdCollider::CollisionResult> retSphereList;
	for (auto& obj : SceneManager::Instance().GetObjList())
	{
		if (obj.get() == this) continue;
		obj->Intersects(sphere, &retSphereList);
	}

	float maxOverLap = 0.0f;
	bool hit = false;
	Math::Vector3 hitDir = Math::Vector3::Zero;

	for (auto& ret : retSphereList)
	{
		// 修正 押し戻し方向の Y 成分（上下）を無視して水平方向の長さだけを評価する
		Math::Vector3 dir = ret.m_hitDir;
		dir.y = 0.0f;

		// 水平方向の押し戻し成分が存在する場合のみ処理
		if (dir.LengthSquared() > 0.0001f)
		{
			if (maxOverLap < ret.m_overlapDistance)
			{
				maxOverLap = ret.m_overlapDistance;
				dir.Normalize(); // Yを0にしてから正規化する
				hitDir = dir;
				hit = true;
			}
		}
	}

	if (hit == true)
	{
		// 修正 水平方向のみ位置を押し戻す（Y軸は変更しないため地面下に埋まらない）
		m_pos.x += hitDir.x * maxOverLap;
		m_pos.z += hitDir.z * maxOverLap;
	}
}