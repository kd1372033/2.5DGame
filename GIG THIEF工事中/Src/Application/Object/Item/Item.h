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
	// 初期設定用セッター / ゲッター
	// =========================================================
	void SetPos(const Math::Vector3& pos) { m_pos = pos; }	// プレイヤー位置をセットするために追加
	void SetDir(const Math::Vector3& dir) { m_dir = dir; }
	// 拾いクールダウンタイマーを設定
	void SetPickUpCooldown(float time) { m_pickUpCooldown = time; }

	// 投げられたフラグの取得・設定
	bool HasBeenThrown() const { return m_hasBeenThrown; }
	void SetHasBeenThrown(bool flag) { m_hasBeenThrown = flag; }

	// 投げて床に着地したフラグの取得・設定
	bool IsLandedAfterThrow() const { return m_isLandedAfterThrow; }
	void SetLandedAfterThrow(bool flag) { m_isLandedAfterThrow = flag; }

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
	float         m_pickUpCooldown = 0.2f; // クールタイム

	std::weak_ptr<Player> m_wpOwner; // 持ち主のプレイヤー

	// プレイヤーによって一度投げられたか
	bool          m_hasBeenThrown = false;

	// 投げられた後に床に着地したか（敵の索敵対象フラグ）
	bool          m_isLandedAfterThrow = false;
};