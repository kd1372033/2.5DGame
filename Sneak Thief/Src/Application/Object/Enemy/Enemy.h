#pragma once

class Player;

// 視界描画用のカスタムポリゴンクラス（Enemy.h 内で定義可能）
class VisionPolygon : public KdPolygon
{
public:
	VisionPolygon()
	{
		// 描画を有効化＆2Dフラグをオフ（3D空間上に描画）
		m_enable = true;
		m_2DObject = false;

		// マテリアルの初期化（デフォルトの白テクスチャなどを設定）
		m_spMaterial = std::make_shared<KdMaterial>();
	}

	// 外部から頂点を設定できるようにする関数
	void SetVertices(const std::vector<Vertex>& vertices)
	{
		m_vertices = vertices;
	}
};

class Enemy : public KdGameObject
{
public:
	Enemy() {}
	~Enemy() {}

	void Init()                       override;
	void Update()                     override;
	void PostUpdate()                 override;
	void GenerateDepthMapFromLight() override;
	void DrawLit()                    override;
	void DrawUnLit()                    override;
	void DrawBright()                 override;

	Math::Vector3 GetPos() { return m_pos; }
	int GetDir() { return m_dirID; }

	void SetPos(const Math::Vector3& _pos) override { m_pos = _pos; }
	void SetTarget(std::weak_ptr<Player> _target) { m_wpTarget = _target; }

	void SetTargetItem(std::shared_ptr<KdGameObject> pItem)
	{
		m_wpItem = pItem;
		m_itemWaitTimer = 0.0f;

		if (!m_wpItem.expired())
		{
			m_itemFlg = true;
			m_chaseFlg = false;
		}
	}

	void AttractTo(const Math::Vector3& itemPos)
	{
		m_targetPos = itemPos;
		m_state = State::Approach;
	}

	void ToggleDebugWire() { s_showDebugWire = !s_showDebugWire; }

	void SetDir(bool dir)
	{
		m_isHorizontalPatrol = dir;

		if (m_isHorizontalPatrol)
		{
			m_dir = { 1.0f, 0.0f, 0.0f }; // 右
			m_dirID = 2;
		}
		else
		{
			m_dir = { 0.0f, 0.0f, 1.0f }; // 上
			m_dirID = 3;
		}
	}

	static void SetShowDebugWire(bool show) { s_showDebugWire = show; }
	static bool GetShowDebugWire() { return s_showDebugWire; }

private:
	// =============================================================
	// ★ 可読性・保守性向上のための分割関数
	// =============================================================
	void UpdatePatrol(bool& isMoving);       // パトロール移動
	void UpdateItemAttract(bool& isMoving);  // アイテム誘惑処理
	void UpdatePlayerChase(bool& isMoving);   // プレイヤー追尾処理
	void UpdateAnimation(bool isMoving);      // UVアニメーション更新

	void CheckCollision();                  // 壁・地面当たり判定
	// 視界チェック
	bool IsPlayerInFieldOfView(const std::shared_ptr<Player>& player);
	// 視界ポリゴンを更新する内部関数
	void UpdateViewPolygon();
	void CheckPlayerSearch();               // プレイヤー索敵
	void CheckItemSearch();                 // アイテム索敵

	// 状態定義
	enum class State {
		Walk,     // 移動中
		Approach, // アイテムに接近
		Wait      // 一時停止中
	};

	// メンバ変数
	std::weak_ptr<KdGameObject> m_target;
	std::weak_ptr<Player>       m_wpTarget;
	std::shared_ptr<KdSquarePolygon> m_polygon;
	// 視界メッシュ描画用のポリゴン
	//std::shared_ptr<KdPolygon> m_viewPolygon = nullptr;
	std::shared_ptr<VisionPolygon> m_viewPolygon = nullptr;
	Math::Vector3 m_pos;
	Math::Vector3 m_dir;
	Math::Vector3 m_targetPos = { 0, 0, 0 };

	int m_dirID = 0; // 0:下, 1:左, 2:右, 3:上
	float m_anime = 0.0f;
	float m_speed = 0.01f;
	float m_gravity = 0.0f;
	float m_timer = 0.0f;

	State m_state = State::Walk;

	// 自機索敵用
	bool m_chaseFlg = false;
	float m_searchArea = 0.45f;

	// アイテム索敵用
	std::weak_ptr<KdGameObject> m_wpItem;
	bool m_itemFlg = false;
	float m_itemSearchArea = 0.75f;
	float m_itemWaitTimer = 0.0f;

	bool m_isHorizontalPatrol = true;

	// UVアニメーション定義テーブル
	const int Run[4][8] = {
		{ 16, 17, 18, 19, 20, 21, 22, 23 }, // 下
		{  8,  9, 10, 11, 12, 13, 14, 15 }, // 左
		{  0,  1,  2,  3,  4,  5,  6,  7 }, // 右
		{ 24, 25, 26, 27, 28, 29, 30, 31 }  // 上
	};

	const int Wait[4][4] = {
		{ 32, 33, 34, 35 }, // 下
		{ 40, 41, 42, 43 }, // 左
		{ 36, 37, 38, 39 }, // 右
		{ 44, 45, 46, 47 }  // 上
	};

	static bool s_showDebugWire;

	// 視界パラメータ
	float m_viewAngle = 50.0f;     // 視野角
	float m_viewDistance = 1.0f;   // 視界の届く最大距離（0.7f から 2.0f〜3.0f に拡張）
	float m_viewRenderDistance = 2.25f; // サーチライトの見た目の長さ
};