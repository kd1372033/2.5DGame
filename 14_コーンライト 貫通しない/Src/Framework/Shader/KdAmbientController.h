#pragma once
#include <list>

class KdAmbientController
{
public:
	struct PointLight
	{
		Math::Vector3 Color = { 1.0f, 1.0f, 1.0f };
		float Radius = 1.0f;
		Math::Vector3 Pos = { 0.0f, 0.0f, 0.0f };
		bool IsBright = false;

		PointLight(const Math::Vector3& color, float radius, const Math::Vector3& pos, bool isBright = false)
			: Color(color), Radius(radius), Pos(pos), IsBright(isBright) {
		}
	};

	struct Parameter
	{
		Math::Vector4 m_ambientLightColor = { 1.0f, 1.0f, 1.0f, 1.0f };

		Math::Vector3 m_directionalLightDir = { 0.0f, -1.0f, 0.0f };
		Math::Vector3 m_directionalLightColor = { 1.0f, 1.0f, 1.0f };

		Math::Vector3 m_distanceFogColor = { 1.0f, 1.0f, 1.0f };
		float m_distanceFogDensity = 0.0f;

		Math::Vector3 m_heightFogColor = { 1.0f, 1.0f, 1.0f };
		float m_heightFogTopValue = 0.0f;
		float m_heightFogBottomValue = 0.0f;
		float m_heightFogBeginDistance = 0.0f;

		// コーンライト用パラメータ
		Math::Vector3 m_coneLightPos = { 0.0f, 0.0f, 0.0f };
		Math::Vector3 m_coneLightDir = { 0.0f, 0.0f, 1.0f };
		float m_coneLightAngle = 0.0f;
		float m_coneLightRange = 0.0f;
		Math::Vector3 m_coneLightColor = { 0.0f, 0.0f, 0.0f };

		// 2026/09/10 追加: 遮蔽判定用パラメータ
		bool m_coneLightIsHit = false;
		Math::Vector3 m_coneLightHitPos = { 0.0f, 0.0f, 0.0f };
		Math::Vector3 m_coneLightHitNormal = { 0.0f, 0.0f, 0.0f };
	};

	void Init();
	void Update();
	void Draw();

	void AddPointLight(const Math::Vector3& Color, float Radius, const Math::Vector3& Pos, bool IsBright = false);
	void AddPointLight(const PointLight& pointLight);

	void SetDirLightShadowArea(const Math::Vector2& lightingArea, float dirLightHeight);
	void SetDirLight(const Math::Vector3& dir, const Math::Vector3& col);
	void SetAmbientLight(const Math::Vector4& col);

	void SetFogEnable(bool distance, bool height);
	void SetDistanceFog(const Math::Vector3& col, float density);
	void SetheightFog(const Math::Vector3& col, float topValue, float bottomValue, float distance);

	void SetConeLightEnable(bool enable);

	// 2026/09/10 追加: コーンライト設定
	void SetConeLight(
		const Math::Vector3& pos,
		const Math::Vector3& dir,
		float angle,
		float range,
		const Math::Vector3& col,
		bool isHit = false,
		const Math::Vector3& hitPos = Math::Vector3::Zero,
		const Math::Vector3& hitNormal = Math::Vector3::Zero
	);

	const Parameter& GetParameter() const { return m_parameter; }

private:
	void WriteLightParams();
	void WriteFogParams();

	Parameter m_parameter;
	std::list<PointLight> m_pointLights;

	DirectX::XMMATRIX m_shadowProj;
	float m_dirLightHeight = 0.0f;

	bool m_dirtyLightDir = false;
	bool m_dirtyLightAmb = false;
	bool m_dirtyFogDist = false;
	bool m_dirtyFogHeight = false;
	bool m_dirtyConeLight = false;
};