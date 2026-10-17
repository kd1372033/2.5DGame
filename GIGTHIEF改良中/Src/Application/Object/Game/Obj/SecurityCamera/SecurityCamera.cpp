//#include "SecurityCamera.h"
//#include "../Player/Player.h"
//#include "../../../../Scene/SceneManager.h"
//
//void SecurityCamera::Init()
//{
//	m_pDebugWire = std::make_unique<KdDebugWireFrame>();
//
//	// デフォルト位置と半径
//	m_pos = { 0.0f, 0.5f, 0.0f };
//	m_detectionRadius = 1.5f;
//	m_isPlayerDetected = false;
//}
//
//void SecurityCamera::Update()
//{
//	// 位置固定の場合は行列更新のみ
//	Math::Matrix transMat = Math::Matrix::CreateTranslation(m_pos);
//	m_mWorld = transMat;
//}
//
//void SecurityCamera::PostUpdate()
//{
//	// プレイヤーの侵入・遮蔽チェック
//	CheckPlayerDetection();
//}
//
//void SecurityCamera::CheckPlayerDetection()
//{
//	m_isPlayerDetected = false;
//
//	auto player = m_wpPlayer.lock();
//	if (!player) return;
//
//	// 1. プレイヤーとの距離判定（3D空間距離）
//	float dist = Math::Vector3::Distance(m_pos, player->GetPos());
//
//	if (dist <= m_detectionRadius)
//	{
//		// 2. 壁遮蔽判定（スフィアの中心からプレイヤーへレイを発射）
//		Math::Vector3 eyePos = m_pos;
//		Math::Vector3 targetPos = player->GetPos() + Math::Vector3(0.0f, 0.3f, 0.0f);
//		Math::Vector3 rayDir = targetPos - eyePos;
//		float rayRange = rayDir.Length();
//
//		if (rayRange > 0.001f)
//		{
//			rayDir.Normalize();
//
//			KdCollider::RayInfo ray(KdCollider::TypeBump, eyePos, rayDir, rayRange);
//			std::list<KdCollider::CollisionResult> retList;
//
//			bool isBlocked = false;
//			for (auto& obj : SceneManager::Instance().GetObjList())
//			{
//				if (obj.get() == this || obj == player) continue;
//
//				if (obj->Intersects(ray, &retList))
//				{
//					if (!retList.empty())
//					{
//						isBlocked = true; // 間に壁（TypeBump）があれば検知しない
//						break;
//					}
//				}
//			}
//
//			// 壁に遮られていなければ発見！
//			if (!isBlocked)
//			{
//				m_isPlayerDetected = true;
//
//				// ★ 発見時のリザルト遷移（必要に応じて有効化）
//				// SceneManager::Instance().SetClearFlag(false);
//				// SceneManager::Instance().SetNextScene(SceneManager::SceneType::Result);
//			}
//		}
//	}
//}
//
//void SecurityCamera::DrawUnLit()
//{
//	if (!m_pDebugWire) return;
//
//	// --------------------------------------------------
//	// ★ 半透明カラースフィア描画（ZWriteDisable + Alpha）
//	// --------------------------------------------------
//	KdShaderManager::Instance().ChangeBlendState(KdBlendState::Alpha);
//	KdShaderManager::Instance().ChangeDepthStencilState(KdDepthStencilState::ZWriteDisable);
//
//	// 状態に応じた色の切り替え
//	Math::Color color = m_isPlayerDetected ? m_alertColor : m_normalColor;
//
//	// スフィアの描画登録
//	m_pDebugWire->AddDebugSphere(m_pos, m_detectionRadius, color);
//	m_pDebugWire->Draw();
//
//	// ステートを復元
//	KdShaderManager::Instance().UndoDepthStencilState();
//	KdShaderManager::Instance().UndoBlendState();
//}