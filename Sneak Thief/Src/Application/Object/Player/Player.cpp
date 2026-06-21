#include "Player.h"

#include "../../Scene/SceneManager.h"
#include "../Item/Item.h"
#include "../Jewelry/Jewelry.h"


void Player::Init()
{
	// デバッグ用のポインタを実体化
	m_pDebugWire = std::make_unique<KdDebugWireFrame>();

	// 実体化
	m_polygon = std::make_shared<KdSquarePolygon>();

	// テクスチャの設定
	m_polygon->SetMaterial("Asset/Textures/Player.png");

	// 画像を分割
	m_polygon->SetSplit(8, 6);

	// 原点変更(真ん中→真ん中)
	m_polygon->SetPivot(KdSquarePolygon::PivotType::Center_Bottom);

	m_pos = { -11,-1.6,-1.75 };
	//m_pos = { 0,-1.6,-1.75 };

	// 当たり判定登録
	m_pCollider = std::make_unique<KdCollider>();
	m_pCollider->RegisterCollisionShape("Player", { 0,0.27,0 }, 0.25, KdCollider::Type::TypeSight);
}

void Player::Update()
{
	if (GetAsyncKeyState('A') & 0x0001)
	{
		m_showDebugWire = !m_showDebugWire; // フラグを反対にする
	}

	m_dir = { 0, 0, 0 }; // 初期化
	bool isMoving = false;

	// 1. 入力検知と移動ベクトルの設定
	if (GetAsyncKeyState(VK_UP) & 0x8000)
	{
		m_dir.z += 1;
		m_dirID = 3; // 上を向く
		isMoving = true;
	}
	if (GetAsyncKeyState(VK_DOWN) & 0x8000)
	{
		m_dir.z += -1;
		m_dirID = 0; // 下を向く
		isMoving = true;
	}
	if (GetAsyncKeyState(VK_LEFT) & 0x8000)
	{
		m_dir.x += -1;
		m_dirID = 1; // 左を向く
		isMoving = true;
	}
	if (GetAsyncKeyState(VK_RIGHT) & 0x8000)
	{
		m_dir.x += 1;
		m_dirID = 2; // 右を向く
		isMoving = true;
	}

	// 2. 移動処理（斜め移動の速度を一定にするために正規化）
	if (isMoving)
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
				m_walkSound->SetVolume(0.1f);
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

	// アイテム投げ
	if (GetAsyncKeyState(VK_SPACE) & 0x0001)
	{
		auto se = KdAudioManager::Instance().Play("Asset/Sounds/Throw.wav", false); // ループ再生

		if (se) {
			se->SetVolume(0.1f); // 音量を50%に
		}
		ThrowItem();
	}

	// 端判定
	if (m_pos.z <= -2.25f)
	{
		m_pos.z = -2.25f;
	}
	if (m_pos.z >= -0.3f)
	{
		m_pos.z = -0.3f;
	}
	if (m_pos.x <= -11.0f)
	{
		m_pos.x = -11.0f;
		if (m_hasJewelry)
		{
			SceneManager::Instance().SetClearFlag(true);
			SceneManager::Instance().SetNextScene(SceneManager::SceneType::Result);
		}
	}
	//重力
	m_pos.y -= m_gravity;
	m_gravity += 0.005f;

	Math::Matrix scalemat = Math::Matrix::CreateScale(0.5f);
	Math::Matrix transmat = Math::Matrix::CreateTranslation(m_pos);

	m_mWorld = scalemat * transmat;
}

void Player::PostUpdate()
{
	// 当たり判定(レイ判定)

	// 当たり判定を実装するときは「当たる側」「当たられる側」が存在する
	// プレイヤーは「当たる側」の判定

	// ==============
	//	レイ判定
	// ==============
	// レイ判定用の変数を作成
	KdCollider::RayInfo ray;
	// レイの発射位置を設定
	ray.m_pos = m_pos;
	// ちょっと上からの位置にする
	//ray.m_pos.y += 0.1;
	// 
	float enableStepHigh = 0.2f;
	ray.m_pos.y += enableStepHigh;

	// レイの発射方向を設定
	ray.m_dir = { 0, -1, 0 };
	// レイの長さを設定
	ray.m_range = m_gravity + enableStepHigh;
	// 当たり判定をしたいタイプを設定
	ray.m_type = KdCollider::TypeGround;
	// デバッグ
	//m_pDebugWire->AddDebugLine(ray.m_pos, ray.m_dir, ray.m_range);

	// レイに当たったオブジェクト情報を格納するリスト
	std::list<KdCollider::CollisionResult> retRayList;
	// 全オブジェクトと当たり判定を行う
	for (auto& obj : SceneManager::Instance().GetObjList())
	{
		if (std::dynamic_pointer_cast<Item>(obj)) continue;
		// レイと当たり判定!!!!!!
		obj->Intersects(ray, &retRayList);
	}

	// レイに当たったリストから一番近いオブジェクトを探す
	// どうやってわかる？→一番遠い距離からの余りを算出して比較する
	//		↓レイの時は余った長さ
	float maxOverLap = 0;	// 初期化！
	Math::Vector3 hitPos;
	bool hit = false;		// 初期化！
	for (auto& ret : retRayList)
	{
		// レイを遮断しオーバーした長さが一番長いものを探す
		if (maxOverLap < ret.m_overlapDistance)
		{
			// 更新
			maxOverLap = ret.m_overlapDistance;
			hitPos = ret.m_hitPos;
			hit = true;
		}
	}

	if (hit == true)
	{
		// 当たっていたら当たった座標をプレイヤー座標にセット
		m_pos = hitPos + Math::Vector3(0, -0.1, 0);
		m_gravity = 0;	//重力を無効化
	}

	// ==============
	//	球(スフィア)判定
	// ==============
	// 球判定用の変数を作成
	KdCollider::SphereInfo sphere;
	// 球の中心座標
	sphere.m_sphere.Center = m_pos;
	sphere.m_sphere.Center.y += 0.27;
	// 球の半径を設定
	sphere.m_sphere.Radius = 0.25;
	// 当たり判定をしたいタイプを設定
	sphere.m_type = KdCollider::Type::TypeGround;

	// デバッグ
	if (m_showDebugWire == true)
	{
		m_pDebugWire->AddDebugSphere(sphere.m_sphere.Center, sphere.m_sphere.Radius);
	}

	// 球に当たったオブジェクト情報を格納するリスト
	std::list<KdCollider::CollisionResult> retSphereList;
	// 全オブジェクトと当たり判定を行う
	for (auto& obj : SceneManager::Instance().GetObjList())
	{
		// 球と当たり判定!!!!!!
		obj->Intersects(sphere, &retSphereList);
	}

	// 球に当たったリストから一番近いオブジェクトを探す！
	//	↓こいつ　レイ判定の時に宣言してるので使いまわす
	maxOverLap = 0;	// 球の時はめり込んだ長さ
	//	↓こいつ　レイ判定の時に宣言してるので使いまわす
	hit = false;
	// 当たった方向を格納する変数
	Math::Vector3 hitDir;
	for (auto& ret : retSphereList)
	{
		// 球にめり込んだ長さが一番長いものを探す
		if (maxOverLap < ret.m_overlapDistance)
		{
			// 更新
			maxOverLap = ret.m_overlapDistance;
			hitDir = ret.m_hitDir;
			hit = true;
		}
	}

	if (hit == true)
	{
		// Z方向への押し戻し無効
		hitDir.z = 0;
		// 正規化（長さを１にする）
		hitDir.Normalize();
		// 押し戻し処理			↓めり込んだ長さ
		m_pos += hitDir * maxOverLap;
		//		↑当たった方向(方向ベクトルは長さ１)
	}

}

void Player::GenerateDepthMapFromLight()
{
	KdShaderManager::Instance().m_StandardShader.DrawPolygon(*m_polygon, m_mWorld);
}

void Player::DrawLit()
{
	KdShaderManager::Instance().m_StandardShader.DrawPolygon(*m_polygon, m_mWorld);
}

// プレイヤー側の攻撃・投擲処理のイメージ
void Player::ThrowItem()
{
	auto newItem = std::make_shared<Item>();
	newItem->Init();

	// 1. 座標のセット（キャラクターの手元付近から投げるため、Y座標を少し上げる）
	Math::Vector3 throwStartPos = m_pos;
	throwStartPos.y += 0.4f; // お腹・手元の高さに調整
	newItem->SetPos(throwStartPos);

	// 2. 向き（m_dir）のセット
	// 移動入力がない時でも、現在の「向いている方向(m_dirID)」から正しいベクトルを作って渡す
	Math::Vector3 throwDir = { 0.0f, 0.0f, 0.0f };
	switch (m_dirID)
	{
	case 0: throwDir.z = -1.0f; break; // 下（手前）を向いている時
	case 1: throwDir.x = -1.0f; break; // 左を向いている時
	case 2: throwDir.x = 1.0f; break; // 右を向いている時
	case 3: throwDir.z = 1.0f; break; // 上（奥）を向いている時
	}
	newItem->SetDir(throwDir);

	// 3. 投げる処理を起動
	newItem->StartThrow();

	// 4. ★【超重要】シーンにオブジェクトを登録（これで Update や PostUpdate が動くようになります）
	SceneManager::Instance().AddObject(newItem);
}