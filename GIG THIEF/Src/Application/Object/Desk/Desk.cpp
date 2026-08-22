#include "Desk.h"
#include "../Player/Player.h"
#include "../Item/Item.h"

void Desk::Init()
{
	m_model = std::make_shared<KdModelData>();

	if (m_model->Load("Asset/Models/Desk/Desk.gltf"))
	{
		m_modelWork.SetModelData(m_model);

		m_pCollider = std::make_unique<KdCollider>();
		m_pCollider->RegisterCollisionShape("Desk", m_model, KdCollider::TypeBump | KdCollider::TypeGround);
	}

	m_spAnimator = std::make_shared<KdAnimator>();

	m_hasItem = true;
	m_isOpen = false;
}

void Desk::Update()
{
	// 1. Eキー入力で「アニメーション開始」のみ行う
	if (m_hasItem && !m_isOpen)
	{
		auto spPlayer = m_wpPlayer.lock();
		if (spPlayer)
		{
			float dist = (spPlayer->GetPos() - m_pos).Length();

			if (dist <= m_interactRange)
			{
				if (GetAsyncKeyState(VK_RETURN) & 0x8000)
				{
					m_isOpen = true; // 開くアニメーションを開始

					if (m_spAnimator && m_model)
					{
						m_spAnimator->SetAnimation(m_model->GetAnimation("Open"), false);
					}
				}
			}
		}
	}

	// 2. アニメーション更新
	UpdateAnimation();

	// 行列更新
	//Math::Matrix rotMat = Math::Matrix::CreateRotationY(DirectX::XM_PIDIV2);
	Math::Matrix rotMat = Math::Matrix::CreateRotationY(DirectX::XMConvertToRadians(m_rot));
	Math::Matrix transMat = Math::Matrix::CreateTranslation(m_pos);
	m_mWorld = rotMat * transMat;
}

void Desk::DrawLit()
{
	if (!m_model)return;

	KdShaderManager::Instance().m_StandardShader.DrawModel(m_modelWork, m_mWorld);
}

void Desk::DrawBright()
{
	if (!m_model)return;

	// 未開封（!m_isOpen）の時だけ高輝度パスで描画して光らせる
	if (!m_isOpen)
	{
		KdShaderManager::Instance().m_StandardShader.DrawModel(m_modelWork, m_mWorld);
	}
}

void Desk::UpdateAnimation()
{
	if (!m_spAnimator)
	{
		return;
	}

	if (m_isOpen)
	{
		if (m_spAnimator->IsAnimationEnd() == false)
		{
			m_spAnimator->AdvanceTime(m_modelWork.WorkNodes());
			m_modelWork.CalcNodeMatrices();
		}
		else if (m_hasItem)
		{
			auto spPlayer = m_wpPlayer.lock();
			if (spPlayer)
			{
				// ★ 修正：アニメーション終了時点でもう一度プレイヤーとの距離を判定する
				float dist = (spPlayer->GetPos() - m_pos).Length();

				// アニメーション完了時に離れてしまっていた場合は、手ぶら・アイテム所持に関わらず「天板（机の上）」にアイテムを生成して置く
				bool isNear = (dist <= m_interactRange);

				// 新しいアイテムを生成
				auto newItem = std::make_shared<Item>();
				newItem->Init();

				// プレイヤーが手ぶら、かつ「引き出しの近くに留まっている」場合のみ手元に獲得させる
				if (spPlayer->GetHeldItem() == nullptr && isNear)
				{
					// 手ぶら＆近くにいる：プレイヤーに直接持たせる
					newItem->PickUp(spPlayer);
					spPlayer->SetHeldItem(newItem);

					SceneManager::Instance().AddObject(newItem);

					// SE再生
					auto se = KdAudioManager::Instance().Play("Asset/Sounds/Get.wav", false);
					if (se)
					{
						se->SetVolume(0.1f);
					}
				}
				else
				{
					// すでに何か持っている、または「開けた後に遠くへ離れてしまった」場合：机の上に置く
					Math::Vector3 forward = -m_mWorld.Forward();
					Math::Vector3 dropPos = m_pos + forward * 0.15f;
					dropPos.y = m_pos.y + 0.25f; // 机の天板の高さ

					newItem->SetPos(dropPos);

					// ※ 手ぶらで開けたのに離れてしまった場合は、プレイヤーが戻ってきたら拾えるようにクールダウンをつけない（または通常設定にする）
					if (spPlayer->GetHeldItem() != nullptr)
					{
						newItem->SetPickUpCooldown(999999.0f); // 既に持っている場合は拾えないアイテムとして生成
					}

					SceneManager::Instance().AddObject(newItem);
				}
			}

			m_hasItem = false;
		}
	}
}