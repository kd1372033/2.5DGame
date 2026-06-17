#include "Enemy.h"

#include "../../Scene/SceneManager.h"
#include "../Player/Player.h"


void Enemy::Init()
{
	m_pDebugWire = std::make_unique<KdDebugWireFrame>();
	m_polygon = std::make_shared<KdSquarePolygon>();
	m_polygon->SetMaterial("Asset/Textures/Enemy.png");
	m_polygon->SetSplit(8, 6);
	m_polygon->SetPivot(KdSquarePolygon::PivotType::Center_Bottom);

	// 初期位置
	m_pos = { -2.0f, 1.0f, -1.0f };

	// 移動初期化
	m_dir = { 1.0f, 0.0f, 0.0f }; // 最初は右向き
	m_speed = 0.01f;
	m_anime = 0.0f;
	m_state = State::Walk;          // 最初は歩く
	m_timer = 0.0f;                 // タイマーリセット

	m_chaseFlg = false;

	// サーチ範囲
	m_searchArea = 0.5f;
}

void Enemy::Update()
{
	bool isMoving = false;

	// タイマーを進めるのは「追尾していない（巡回中）のとき」だけ
	if (!m_chaseFlg)
	{
		m_timer += 1.0f;
	}

	// =========================================================
	// プレイヤーとの距離判定（索敵・追尾フラグの更新）
	// =========================================================
	float offsetDistance = 0.3f; // 前方にズラす距離
	Math::Vector3 searchCenter = m_pos + Math::Vector3(0, 0, 0);

	// 追尾中でない（巡回中）の時だけ、向いている前方にサークルを押し出す
	if (!m_chaseFlg)
	{
		searchCenter.x += m_dir.x * offsetDistance;
	}

	Math::Vector3 targetPos = {};
	if (m_wpTarget.expired() == false)
	{
		targetPos = m_wpTarget.lock()->GetPos();

		// ① 索敵サークルの中心（searchCenter）からプレイヤーへの距離を測る（これは合っています）
		Math::Vector3 v = targetPos - searchCenter;
		v.y = 0.0f;

		if (v.Length() < m_searchArea)
		{
			m_chaseFlg = true;

			// ★修正ポイント：方向を決めるベクトルは、サークルの中心ではなく
			// 「エネミーの本来の位置（m_pos）」から「プレイヤー（targetPos）」への純粋な左右関係で計算する！
			Math::Vector3 chaseVec = targetPos - m_pos;
			chaseVec.y = 0.0f;
			chaseVec.Normalize();

			// 安全対策：もし完全に重なってゼロベクトルになった場合は、現在の向きを維持
			if (chaseVec.Length() > 0.0f)
			{
				m_dir = chaseVec;
			}
		}
		else
		{
			if (m_chaseFlg)
			{
				m_chaseFlg = false;
				m_state = State::Wait;
				m_timer = 0.0f;
				// 見失った時は、その時向いていた方向（m_dir.xが正なら右、負なら左）を維持して巡回に戻す
				m_dir = (m_dir.x > 0.0f) ? Math::Vector3{ 1.0f, 0.0f, 0.0f } : Math::Vector3{ -1.0f, 0.0f, 0.0f };
			}
		}
	}
	// =========================================================
	// 状態に応じた移動処理（追尾 ON / OFF で分岐）
	// =========================================================
	if (m_chaseFlg)
	{
		isMoving = true;
		float dashSpeed = m_speed * 3.0f;
		m_pos.x += dashSpeed * m_dir.x;
		m_anime += 0.05f;
	}
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
				m_dir.x *= -1.0f;
				m_state = State::Walk;
				m_timer = 0.0f;
			}
			break;
		}
	}

	// ★修正ポイント③：デバッグ用の球も、ずらした中心座標（searchCenter）を使って描画する
	m_pDebugWire->AddDebugSphere
	(
		searchCenter,
		m_searchArea,
		kGreenColor
	);

	// =========================================================
	// アニメーションタイマーの更新
	// =========================================================
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

	// =========================================================
	// 向き（m_dirID）の同期とUVRectの設定
	// =========================================================
	if (m_dir.x > 0.0f)      m_dirID = 2; // 右向き
	else if (m_dir.x < 0.0f) m_dirID = 1; // 左向き

	if (isMoving)
	{
		int RunFrame = (int)m_anime % 8;
		m_polygon->SetUVRect(Run[m_dirID][RunFrame]);
	}
	else
	{
		int WaitFrame = (int)m_anime % 4;
		m_polygon->SetUVRect(Wait[m_dirID][WaitFrame]);
	}

	// =========================================================
	// 重力処理・行列更新
	// =========================================================
	// ※スペースキーによるジャンプ処理はエネミーの挙動をバグらせるため削除しました
	m_pos.y -= m_gravity;
	m_gravity += 0.005f;

	Math::Matrix scalemat = Math::Matrix::CreateScale(0.5f);
	Math::Matrix transmat = Math::Matrix::CreateTranslation(m_pos);
	m_mWorld = scalemat * transmat;
}

void Enemy::PostUpdate()
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
	m_pDebugWire->AddDebugLine(ray.m_pos, ray.m_dir, ray.m_range);

	// レイに当たったオブジェクト情報を格納するリスト
	std::list<KdCollider::CollisionResult> retRayList;
	// 全オブジェクトと当たり判定を行う
	for (auto& obj : SceneManager::Instance().GetObjList())
	{
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
	sphere.m_sphere.Center.y += 0.3;
	// 球の半径を設定
	sphere.m_sphere.Radius = 0.3;
	// 当たり判定をしたいタイプを設定
	sphere.m_type = KdCollider::Type::TypeGround;

	// デバッグ
	m_pDebugWire->AddDebugSphere(sphere.m_sphere.Center, sphere.m_sphere.Radius);

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

void Enemy::GenerateDepthMapFromLight()
{
	KdShaderManager::Instance().m_StandardShader.DrawPolygon(*m_polygon, m_mWorld);
}

void Enemy::DrawLit()
{
	KdShaderManager::Instance().m_StandardShader.DrawPolygon(*m_polygon, m_mWorld);
}
