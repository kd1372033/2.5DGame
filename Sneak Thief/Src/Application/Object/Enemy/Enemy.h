#pragma once

class Player;
class Enemy : public KdGameObject
{
public:
	Enemy() {}
	~Enemy() {}

	void Init()			override;
	void Update()		override;
	void PostUpdate()	override;
	void GenerateDepthMapFromLight()override;
	void DrawLit()		override;

	Math::Vector3 GetPos() { return m_pos; }
	int GetDir() { return m_dirID; }

	void SetPos(const Math::Vector3& _pos) override { m_pos = _pos; }
	void SetTarget(std::weak_ptr<Player> _target) { m_wpTarget = _target; }
	void SetTargetItem(std::shared_ptr<KdGameObject> pItem)
	{
		m_wpItem = pItem;
		m_itemWaitTimer = 0.0f; // タイマー初期化

		// アイテムが有効なら、その瞬間にアイテム誘惑モードをONにする！
		if (!m_wpItem.expired())
		{
			m_itemFlg = true;
			m_chaseFlg = false; // プレイヤー追尾はキャンセル
		}
	}

	// アイテム側から呼び出される関数
	void AttractTo(const Math::Vector3& itemPos)
	{
		// すでに足止め中や接近中なら無視する

		Math::Vector3 m_targetPos = itemPos;      // 目標座標をアイテムの場所に設定
		m_state = State::Approach;
	}

	void ToggleDebugWire() { s_showDebugWire = !s_showDebugWire; }

	void SetDir(bool dir)
	{
		// どちらのパトロールかを記憶しておく
		m_isHorizontalPatrol = dir;

		if (m_isHorizontalPatrol)
		{
			// 左右パトロール：最初は「右」を向く
			m_dir = { 1.0f, 0.0f, 0.0f };
			m_dirID = 2; // 右用のグラフィックID
		}
		else
		{
			// 上下パトロール：最初は「上」を向く
			m_dir = { 0.0f, 0.0f, 1.0f };
			m_dirID = 3;
			// ※上下移動のグラフィックがなければ、ひとまず右(2)か左(1)のままにします
		}
	}

	static void SetShowDebugWire(bool show) { s_showDebugWire = show; }

	// ★追加：現在のデバッグワイヤーの表示状態を取得するゲッター（反転させる時に便利です）
	static bool GetShowDebugWire() { return s_showDebugWire; }

private:

	std::weak_ptr<KdGameObject> m_target;

	std::weak_ptr<Player>m_wpTarget;

	std::shared_ptr<KdSquarePolygon> m_polygon;

	int m_dirID = 0; // 0:下, 1:左, 2:右, 3:上
	Math::Vector3 m_pos;
	Math::Vector3 m_dir;

	float m_anime = 0;
	float m_speed = 0.01f;
	float m_goal = 0;
	float m_gravity = 0;

	enum class State {
		Walk,   // 移動中
		Approach,	// アイテムに近づく
		Wait    // 1秒停止中
	};
	State m_state; // 初期状態は移動
	float m_timer = 0;    // 停止タイマー

	// 自機との当たり判定用
	bool m_chaseFlg = false;
	float m_searchArea = 0;

	// アイテムとの当たり判定用
	std::weak_ptr<KdGameObject> m_wpItem;     // 投げられたアイテム
	bool m_itemFlg = false; // アイテムに気を取られているか
	float m_itemSearchArea = 0.0f; // アイテム用の索敵半径
	float m_itemWaitTimer = 0.0f; // アイテムの場所で立ち止まるタイマー
	Math::Vector3 m_targetPos = { 0, 0, 0 };

	bool m_isHorizontalPatrol = true;

	const int Run[4][8] = {
		{ 16,17,18,19,20,21,22,23 }, // 0:下
		{ 8,9,10,11,12,13,14,15 },   // 1:左
		{ 0,1,2,3,4,5,6,7 },         // 2:右
		{ 24,25,26,27,28,29,30,31 }  // 3:上
	};

	const int Wait[4][4] = {
		{ 32,33,34,35 }, // 0:下
		{ 40,41,42,43 }, // 1:左
		{ 36,37,38,39 }, // 2:右
		{ 44,45,46,47 }  // 3:上
	};

	static bool s_showDebugWire;
};