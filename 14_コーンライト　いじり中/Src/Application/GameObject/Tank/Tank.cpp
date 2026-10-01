#include "Tank.h"
#include "../Camera/TPSCamera/TPSCamera.h"
#include "../Ground/Ground.h"
#include "../../Scene/SceneManager.h"

void Tank::Init()
{
	m_pDebugWire = std::make_unique<KdDebugWireFrame>();
	m_model = std::make_shared<KdModelData>();
	m_model->Load("Asset/Models/tank/tank.gltf");

	KdShaderManager::Instance().WorkAmbientController().SetConeLightEnable(true);

	m_pos = { 0, 5, 0 };
}

void Tank::Update()
{
	Math::Matrix camRotYMat;
	if (m_camera.expired() == false)
	{
		camRotYMat = m_camera.lock()->GetRotationYMatrix();
	}

	Math::Vector3 dir;
	bool moveFlg = false;

	if (GetAsyncKeyState('W') & 0x8000) {
		dir += Math::Vector3::TransformNormal({ 0,0,1 }, camRotYMat);
		moveFlg = true;
	}
	if (GetAsyncKeyState('D') & 0x8000) {
		dir += Math::Vector3::TransformNormal({ 1,0,0 }, camRotYMat);
		moveFlg = true;
	}
	if (GetAsyncKeyState('S') & 0x8000) {
		dir += Math::Vector3::TransformNormal({ 0,0,-1 }, camRotYMat);
		moveFlg = true;
	}
	if (GetAsyncKeyState('A') & 0x8000) {
		dir += Math::Vector3::TransformNormal({ -1,0,0 }, camRotYMat);
		moveFlg = true;
	}

	if (moveFlg == true)
	{
		dir.Normalize();

		Math::Matrix nowRotMat = Math::Matrix::CreateRotationY(DirectX::XMConvertToRadians(m_angle));
		Math::Vector3 nowDir = Math::Vector3::TransformNormal(Math::Vector3(0, 0, 1), nowRotMat);

		Math::Vector3 toDir = dir;

		float dot = nowDir.Dot(toDir);
		float angle = DirectX::XMConvertToDegrees(acos(dot));

		if (angle >= 0.1f)
		{
			if (angle > 5)
			{
				angle = 5;
			}

			Math::Vector3 cross = nowDir.Cross(toDir);
			if (cross.y >= 0.0f) {
				m_angle += angle;
				if (m_angle > 360) { m_angle -= 360; }
			}
			else {
				m_angle -= angle;
				if (m_angle < 0) { m_angle += 360; }
			}
		}

		KdDebugGUI::Instance().AddLog("%f\n", m_angle);
		m_pos += dir * 0.3f;
	}

	if (!m_isGrounded)
	{
		m_gravity += m_gravityAccel;
		if (m_gravity > m_maxGravity)
		{
			m_gravity = m_maxGravity;
		}
	}
	m_pos.y -= m_gravity;

	Math::Matrix rotMat = Math::Matrix::CreateRotationY(DirectX::XMConvertToRadians(m_angle));
	Math::Matrix transMat = Math::Matrix::CreateTranslation(m_pos);
	m_mWorld = rotMat * transMat;

	CheckGroundCollision();

	if (GetAsyncKeyState(VK_SPACE) & 0x8000)
	{
		m_isRayFixed = false;
	}

	if (GetAsyncKeyState(VK_UP) & 0x8000)
	{
		m_maxLightRange += 0.5f;
	}
	if (GetAsyncKeyState(VK_DOWN) & 0x8000)
	{
		m_maxLightRange -= 0.5f;
	}
	m_maxLightRange = std::clamp(m_maxLightRange, 1.0f, 100.0f);

	if (GetAsyncKeyState(VK_RIGHT) & 0x8000)
	{
		m_coneLightAngle += 0.5f;
	}
	if (GetAsyncKeyState(VK_LEFT) & 0x8000)
	{
		m_coneLightAngle -= 0.5f;
	}
	m_coneLightAngle = std::clamp(m_coneLightAngle, m_minConeAngle, m_maxConeAngle);
}

void Tank::PostUpdate()
{
	// 【変更点1】光およびレイの発生源を「自機（戦車）の中心」に変更
	// ※ Y軸の +0.5f は地面に埋まらないための高さ調整です
	Math::Vector3 rayPos = m_pos + Math::Vector3(0.0f, 0.5f, 0.0f);
	Math::Vector3 rayDir = m_mWorld.Backward();
	float maxRayDist = m_maxLightRange;

	m_isHitWall = false;
	m_hitCenterPos = rayPos + rayDir * maxRayDist;
	m_hitNormal = -rayDir;

	// --- 【高速化】中央1本のみレイ判定を行って着弾点・法線を取得 ---
	if (auto ground = m_wpStage.lock())
	{
		KdCollider::RayInfo rayInfo(
			KdCollider::TypeGround | KdCollider::TypeBump,
			rayPos,
			rayDir,
			maxRayDist
		);

		std::list<KdCollider::CollisionResult> retHitList;
		ground->Intersects(rayInfo, &retHitList);

		float minOverlap = maxRayDist;
		for (const auto& result : retHitList)
		{
			// 【変更点2】自機中心から発射するため、自機メッシュへのヒット除外距離を 0.8f に変更
			if (result.m_overlapDistance < 0.8f) { continue; }

			if (result.m_overlapDistance < minOverlap)
			{
				minOverlap = result.m_overlapDistance;
				m_isHitWall = true;
				m_hitCenterPos = result.m_hitPos;
				m_hitNormal = result.m_hitDir;
			}
		}
	}

	m_hitNormal.Normalize();

	// --- デバッグ描画（1本のみ） ---
	if (m_pDebugWire)
	{
		if (m_isHitWall)
		{
			m_pDebugWire->AddDebugLine(rayPos, m_hitCenterPos, kGreenColor);
		}
		else
		{
			m_pDebugWire->AddDebugLine(rayPos, rayPos + (rayDir * maxRayDist), kRedColor);
		}
	}

	// 描画データセット
	m_raypos = rayPos;
	m_tohitvector = rayDir;
	m_dynamicangle = m_coneLightAngle;
	m_renderlightdist = maxRayDist;
	m_searchlightcolor = { 10.0f, 0.0f, 0.0f };
}

void Tank::DrawLit()
{
	// 遮蔽パラメータ（m_isHitWall, 着弾座標, 着弾法線）をシェーダーへ渡す
	KdShaderManager::Instance().WriteCBConeLight(
		m_raypos,
		m_tohitvector,
		DirectX::XMConvertToRadians(m_dynamicangle),
		m_renderlightdist,
		m_searchlightcolor,
		m_isHitWall,
		m_hitCenterPos,
		m_hitNormal
	);
	KdShaderManager::Instance().m_StandardShader.DrawModel(*m_model, m_mWorld);
}

void Tank::DrawBright()
{

}

void Tank::CheckGroundCollision()
{
	Math::Vector3 rayPos = m_pos + Math::Vector3(0.0f, 2.0f, 0.0f);
	Math::Vector3 rayDir = -Math::Vector3::Up;
	float rayDist = 5.0f;

	KdCollider::RayInfo rayInfo(
		KdCollider::TypeGround,
		rayPos,
		rayDir,
		rayDist
	);

	std::list<KdCollider::CollisionResult> retHitList;

	if (auto ground = m_wpStage.lock())
	{
		ground->Intersects(rayInfo, &retHitList);
	}

	float minDist = rayDist;
	m_isGrounded = false;

	for (const auto& result : retHitList)
	{
		if (result.m_overlapDistance < minDist)
		{
			minDist = result.m_overlapDistance;

			m_pos.y = result.m_hitPos.y;
			m_isGrounded = true;
		}
	}

	if (m_isGrounded)
	{
		m_gravity = 0.0f;

		Math::Matrix rotMat = Math::Matrix::CreateRotationY(DirectX::XMConvertToRadians(m_angle));
		Math::Matrix transMat = Math::Matrix::CreateTranslation(m_pos);
		m_mWorld = rotMat * transMat;
	}
}