#pragma once

#include "Framework/KdFramework.h"

class Player;

class Jewelry : public KdGameObject
{
public:
	Jewelry() { Init(); }
	virtual ~Jewelry() override {}

	void Init() override;
	void Update() override;
	void PostUpdate() override;
	void DrawLit() override;
	void GenerateDepthMapFromLight() override;
	void DrawBright() override;
	void OnHit() override;

	void SetPlayer(std::shared_ptr<Player> player) { m_wpplayer = player; }
	void SetCanGet(bool canGet) { m_canGet = canGet; }
	bool CanGet() const { return m_canGet; }

private:
	std::shared_ptr<KdModelData> m_model = nullptr;
	std::weak_ptr<Player> m_wpplayer;

	Math::Vector3 m_pos = Math::Vector3::Zero;
	float m_alpha = 1.0f;
	bool m_canGet = false;           // PC未破壊時は false
	bool m_isAlertTriggered = false;  // 侵入警戒フラグ
	bool m_isFadeOut = false;        // ★ 追加: 消滅フェードアウト用フラグ

	float m_barrierRadius = 0.7f;    // カラースフィアの半径
};