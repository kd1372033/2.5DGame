#pragma once
#include "../../Scene/SceneManager.h"

#pragma once
#include "../../Scene/SceneManager.h"

// 循環参照を防ぐための前方宣言
class Player;

class Item : public KdGameObject
{
public:
	void Init() override;
	void Update() override;
	void PostUpdate() override;
	void DrawLit() override;

	// =========================================================
	// 初期設定用セッター
	// =========================================================
	void SetPos(const Math::Vector3& pos) { m_pos = pos; }	//プレイヤー位置をセットするために追加
	void SetDir(const Math::Vector3& dir) { m_dir = dir; }

	bool IsHeld() const { return m_isHeld; }
	bool IsThrown() const { return m_isThrown; }

	// 状態変化関数
	void PickUp(std::shared_ptr<Player> pOwner);
	void StartThrow();
	void StartThrow(const Math::Vector3& dir); // 方向指定付き投擲

	// 足止め後に消失
	void OnHit() override
	{
		m_isExpired = true;
	}

private:
	std::shared_ptr<KdSquarePolygon> m_polygon;

	bool          m_isThrown = false;
	bool          m_isHeld = false;
	Math::Vector3 m_pos = { 0,0,0 };
	Math::Vector3 m_vec = { 0,0,0 };
	Math::Vector3 m_dir = { 0,0,0 };
	float         m_hitRadius = 0.0f;

	const float   gravity = -0.005f;
	float         m_throwSafetyTimer = 0.0f;
	float m_pickUpCooldown = 0.2f; // クールタイム

	std::weak_ptr<Player> m_wpOwner; // 持ち主のプレイヤー
};