#include "Enemy.h"

#include "../../Scene/SceneManager.h"
#include "../Player/Player.h"
#include "../Item/Item.h"


void Enemy::Init()
{
	m_pDebugWire = std::make_unique<KdDebugWireFrame>();
	m_polygon = std::make_shared<KdSquarePolygon>();
	m_polygon->SetMaterial("Asset/Textures/Enemy.png");
	m_polygon->SetSplit(8, 6);
	m_polygon->SetPivot(KdSquarePolygon::PivotType::Center_Bottom);

	// 初期位置
	m_pos = { -8.25f,-1.6f,-2.0f};

	// 移動初期化
	m_dir = { 1.0f, 0.0f, 0.0f }; // 最初は右向き
	m_speed = 0.01f;
	m_anime = 0.0f;
	m_state = State::Walk;          // 最初は歩く
	m_timer = 0.0f;                 // タイマーリセット

	m_chaseFlg = false;

	// サーチ範囲
	m_searchArea = 0.45f;

	m_itemSearchArea = 1.0f;
}

void Enemy::Update()
{
	bool isMoving = false;

	// タイマーを進めるのは「追尾していない（巡回中）のとき」だけ
	if (!m_chaseFlg && !m_itemFlg)
	{
		m_timer += 1.0f;
	}

	// =========================================================
	// 状態に応じた移動処理（PostUpdateで確定したフラグを元に動く）
	// =========================================================

	// ① アイテム誘惑モード
	if (m_itemFlg)
	{
		if (m_wpItem.expired() == false)
		{
			Math::Vector3 itemPos = m_wpItem.lock()->GetPos();
			Math::Vector3 vItem = itemPos - m_pos;
			vItem.y = 0.0f;

			// アイテムに到着している場合（2秒間停止）
			if (vItem.Length() < 0.2f || m_itemWaitTimer > 0.0f)
			{
				isMoving = false;
				if (m_itemWaitTimer == 0.0f) m_itemWaitTimer = 120.0f; // 2秒セット

				m_itemWaitTimer -= 1.0f;

				// 3秒経ったら執着を捨てる
				if (m_itemWaitTimer <= 0.0f)
				{
					m_wpItem.lock()->OnHit();

					m_itemFlg = false;
					m_wpItem.reset();
					m_state = State::Wait; // 少し休憩させてからパトロールへ
					m_timer = 0.0f;
				}
			}
			else
			{
				// まだ離れているならアイテムに向かって進む
				isMoving = true;
				vItem.Normalize();
				m_dir = vItem;
				m_pos.x += m_speed * m_dir.x;
				m_pos.z += m_speed * m_dir.z;
			}
		}
		else
		{
			// アイテムが消滅するなどして無効になったら通常に戻る
			m_itemFlg = false;
		}
	}
	// ② プレイヤー追尾モード（Dash）
	else if (m_chaseFlg)
	{
		isMoving = true;
		float dashSpeed = m_speed * 1.75f;
		m_pos.x += dashSpeed * m_dir.x;
		m_pos.z += dashSpeed * m_dir.z;
		m_anime += 0.05f;
	}
	// ③ 通常のパトロール（巡回）
	else
	{
		switch (m_state)
		{
		case State::Walk:
			isMoving = true;
			m_pos.x += m_speed * m_dir.x;

			if (m_timer >= 120.0f)
			{
				m_state = State::Wait;
				m_timer = 0.0f;
			}
			break;

		case State::Wait:
			isMoving = false;

			if (m_timer >= 60.0f)
			{
				m_dir.x *= -1.0f; // 反転
				m_state = State::Walk;
				m_timer = 0.0f;
			}
			break;
		}
	}

	// ステージの端判定
	if (m_pos.z <= -2.25f)  m_pos.z = -2.25f;
	if (m_pos.z >= -0.3f)   m_pos.z = -0.3f;
	if (m_pos.x <= -11.0f)  m_pos.x = -11.0f;

	// アニメーションタイマーの更新
	if (isMoving)
	{
		m_anime += 0.1f;
		if (m_anime >= 8.0f) m_anime = 0.0f;
	}
	else
	{
		m_anime += 0.05f;
		if (m_anime >= 4.0f) m_anime = 0.0f;
	}

	// 向きの同期
	if (m_dir.x > 0.0f)      m_dirID = 2; // 右
	else if (m_dir.x < 0.0f) m_dirID = 1; // 左

	if (isMoving) m_polygon->SetUVRect(Run[m_dirID][(int)m_anime % 8]);
	else          m_polygon->SetUVRect(Wait[m_dirID][(int)m_anime % 4]);

	// 重力処理
	m_pos.y -= m_gravity;
	m_gravity += 0.005f;

	// 行列更新
	Math::Matrix scalemat = Math::Matrix::CreateScale(0.5f);
	Math::Matrix transmat = Math::Matrix::CreateTranslation(m_pos);
	m_mWorld = scalemat * transmat;
}

void Enemy::PostUpdate()
{
	// ==============
	//	レイ判定（既存の地面押し戻し）
	// ==============
	KdCollider::RayInfo ray;
	ray.m_pos = m_pos;
	float enableStepHigh = 0.2f;
	ray.m_pos.y += enableStepHigh;
	ray.m_dir = { 0, -1, 0 };
	ray.m_range = m_gravity + enableStepHigh;
	ray.m_type = KdCollider::TypeGround;
	m_pDebugWire->AddDebugLine(ray.m_pos, ray.m_dir, ray.m_range);

	std::list<KdCollider::CollisionResult> retRayList;
	for (auto& obj : SceneManager::Instance().GetObjList())
	{
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
		m_pos = hitPos + Math::Vector3(0.0f, -0.1f, 0.0f);
		m_gravity = 0;
	}

	// ==============
	//	球(スフィア)判定（既存の壁押し戻し）
	// ==============
	KdCollider::SphereInfo sphere;
	sphere.m_sphere.Center = m_pos;
	sphere.m_sphere.Center.y += 0.3f;
	sphere.m_sphere.Radius = 0.3f;
	sphere.m_type = KdCollider::Type::TypeGround;
	m_pDebugWire->AddDebugSphere(sphere.m_sphere.Center, sphere.m_sphere.Radius);

	std::list<KdCollider::CollisionResult> retSphereList;
	for (auto& obj : SceneManager::Instance().GetObjList())
	{
		obj->Intersects(sphere, &retSphereList);
	}

	maxOverLap = 0;
	hit = false;
	Math::Vector3 hitDir;
	for (auto& ret : retSphereList)
	{
		if (maxOverLap < ret.m_overlapDistance)
		{
			maxOverLap = ret.m_overlapDistance;
			hitDir = ret.m_hitDir;
			hit = true;
		}
	}
	if (hit == true)
	{
		hitDir.z = 0;
		hitDir.Normalize();
		m_pos += hitDir * maxOverLap;
	}

	float offsetDistance = 0.3f;
	Math::Vector3 playerSearchCenter = m_pos;
	if (!m_chaseFlg)
	{
		playerSearchCenter.x += m_dir.x * offsetDistance;
	}

	// 自機の索敵範囲
	KdCollider::SphereInfo playerSearchSphere;
	playerSearchSphere.m_sphere.Center = playerSearchCenter;
	playerSearchSphere.m_sphere.Radius = m_searchArea;
	playerSearchSphere.m_type = KdCollider::TypeSight; // ★ タイプを設定

	// 緑色のデバッグ球でプレイヤー索敵範囲を描画
	m_pDebugWire->AddDebugSphere(playerSearchSphere.m_sphere.Center, playerSearchSphere.m_sphere.Radius, kGreenColor);

	std::list<KdCollider::CollisionResult> retPlayerSearchList;
	for (auto& obj : SceneManager::Instance().GetObjList())
	{
		// ターゲットがプレイヤーの場合のみ、コライダー判定を行う
		if (std::dynamic_pointer_cast<Player>(obj))
		{
			obj->Intersects(playerSearchSphere, &retPlayerSearchList);
		}
	}

	// ==========================================
	// 2. 全オブジェクトからプレイヤーを探す
	// ==========================================
	for (auto& obj : SceneManager::Instance().GetObjList())
	{
		if (auto player = std::dynamic_pointer_cast<Player>(obj))
		{
			std::list<KdCollider::CollisionResult> retPlayerList;

			// プレイヤーが索敵スフィア内に入っているか？
			if (player->Intersects(playerSearchSphere, &retPlayerList))
			{
				// ─── 索敵成功！ここからベクターでの判定 ───

				// 3. 敵からプレイヤーへのベクトルを計算
				Math::Vector3 vToPlayer = player->GetPos() - m_pos;

				// 4. ベクトルの長さ（2点間の距離）を算出
				float distance = vToPlayer.Length();

				// 5. 完全に重なった（至近距離になった）か判定
				// 0.2f などの閾値は、キャラのサイズに合わせて調整してください
				if (distance < 0.2f)
				{
					// ゲームオーバー判定
					SceneManager::Instance().SetClearFlag(false);
					SceneManager::Instance().SetNextScene(SceneManager::SceneType::Result);
					return;
				}

				// (参考) ここに「プレイヤーを追尾する処理」などを書くこともできます
			}
		}
	}

	// ★ 地面判定と同じ仕組み：該当する結果（めり込み）がリストに入っているかチェック
	float maxPlayerOverlap = 0.0f;
	bool hitPlayer = false;

	for (auto& ret : retPlayerSearchList)
	{
		// 該当していたらフラグを立てる（一番めり込みが大きい＝近いものを採用）
		if (maxPlayerOverlap < ret.m_overlapDistance)
		{
			maxPlayerOverlap = ret.m_overlapDistance;
			hitPlayer = true;
		}
	}

	// ★ 該当していたら（ヒットしていたら）処理を行う
	if (hitPlayer && !m_itemFlg)
	{
		m_chaseFlg = true; // プレイヤー追尾モードON

		if (m_wpTarget.expired() == false)
		{
			Math::Vector3 targetPos = m_wpTarget.lock()->GetPos();
			Math::Vector3 chaseVec = targetPos - m_pos;
			chaseVec.y = 0.0f;
			chaseVec.Normalize();
			if (chaseVec.Length() > 0.0f) m_dir = chaseVec; // プレイヤーの方向を向く
		}
	}
	else
	{
		// 該当しなくなったら（範囲外に出たら）追尾終了
		if (m_chaseFlg)
		{
			m_chaseFlg = false;
			m_state = State::Wait;
			m_timer = 0.0f;
		}
	}

	// アイテムの索敵範囲
	KdCollider::SphereInfo itemSearchSphere;
	itemSearchSphere.m_sphere.Center = m_pos;
	itemSearchSphere.m_sphere.Radius = m_itemSearchArea;
	itemSearchSphere.m_type = KdCollider::TypeSight;

	// 赤色のデバッグ球で、エネミー自身のアイテム知覚範囲を描画
	m_pDebugWire->AddDebugSphere(itemSearchSphere.m_sphere.Center, itemSearchSphere.m_sphere.Radius, kRedColor);

	// 見つけたアイテムを記憶しておくための変数
	std::shared_ptr<Item> pFoundItem = nullptr;
	float maxItemOverlap = 0.0f;
	bool hitItem = false;

	for (auto& obj : SceneManager::Instance().GetObjList())
	{
		// ターゲットがアイテムの場合のみチェック
		auto itemObj = std::dynamic_pointer_cast<Item>(obj);
		if (itemObj)
		{
			std::list<KdCollider::CollisionResult> retItemSearchList;

			// 当たり判定を実行
			if (itemObj->Intersects(itemSearchSphere, &retItemSearchList))
			{
				// リストの中身をチェックして、一番めり込みが深いものを探す
				for (auto& ret : retItemSearchList)
				{
					if (maxItemOverlap < ret.m_overlapDistance)
					{
						maxItemOverlap = ret.m_overlapDistance;
						hitItem = true;

						// 今まさに判定が当たったこのアイテム(itemObj)をキープする！
						pFoundItem = itemObj;
					}
				}
			}
		}
	}

	// ★ アイテムが範囲内に該当していたら（見つけたら）処理を行う！
	if (hitItem && pFoundItem)
	{
		if (!m_itemFlg)
		{
			m_itemFlg = true;
			m_chaseFlg = false;       // プレイヤーの追尾はキャンセル
			m_wpItem = pFoundItem;    // 見つけたアイテムをターゲットとして記憶
			m_itemWaitTimer = 0.0f;   // タイマーリセット
		}
	}
	else
	{
		// ★ アイテム誘惑モード中なのに、範囲内に該当しなくなったら（見失ったら）処理を行う
		if (m_itemFlg && m_itemWaitTimer <= 0.0f)
		{
			m_itemFlg = false;
			m_wpItem.reset(); // アイテムを諦めてパトロールに戻る
		}
	}
}

void Enemy::GenerateDepthMapFromLight()
{
	KdShaderManager::Instance().m_StandardShader.DrawPolygon(*m_polygon, m_mWorld);
}

void Enemy::DrawLit()
{
	KdShaderManager::Instance().m_StandardShader.DrawPolygon(*m_polygon, m_mWorld);
}
