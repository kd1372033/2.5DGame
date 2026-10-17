#include "inc_KdStandardShader.hlsli"
#include "../inc_KdCommon.hlsli"

// モデル描画用テクスチャ
Texture2D g_baseTex : register(t0); // ベースカラーテクスチャ
Texture2D g_metalRoughTex : register(t1); // メタリック/ラフネステクスチャ
Texture2D g_emissiveTex : register(t2); // 発光テクスチャ
Texture2D g_normalTex : register(t3); // 法線マップ

// 特殊処理用テクスチャ
Texture2D g_dirShadowMap : register(t10); // 平行光シャドウマップ
Texture2D g_dissolveTex : register(t11); // ディゾルブマップ
Texture2D g_environmentTex : register(t12); // 反射景マップ

// サンプラ
SamplerState g_ss : register(s0); // 通常のテクスチャ描画用
SamplerComparisonState g_ssCmp : register(s1); // 補間用比較機能付き

// --------------------------------------------------
// ★ 複数コーンライト用定数バッファ (b11)
// --------------------------------------------------
static const int MAX_CONE_LIGHTS = 20;

struct ConeLightData
{
	float3 Pos;
	float Angle;
	float3 Dir;
	float Range;
	float3 Color;
	float Intensity;
	int IsHit;
	float3 HitPos;
	float3 HitNormal;
	float Padding;
};

cbuffer cbConeLight : register(b11)
{
	int g_ConeLightCount;
	float3 g_ConeLightPadding;
	ConeLightData g_ConeLights[MAX_CONE_LIGHTS];
};

float BlinnPhong(float3 lightDir, float3 vCam, float3 normal, float specPower)
{
	float3 H = normalize(-lightDir + vCam);
	float NdotH = saturate(dot(normal, H));
	float spec = pow(NdotH, specPower);
	return spec * ((specPower + 2) / (2 * 3.1415926535));
}

//================================
// ピクセルシェーダ
//================================
float4 main(VSOutput In) : SV_Target0
{
	// ディゾルブによる描画スキップ
	float discardValue = g_dissolveTex.Sample(g_ss, In.UV).r;
	if (discardValue < g_dissolveValue)
	{
		discard;
	}
	
	//------------------------------------------
	// 材質色
	//------------------------------------------
	float4 baseColor = g_baseTex.Sample(g_ss, In.UV) * g_BaseColor * In.Color;
	
	// Alphaテスト
	if (baseColor.a < 0.05f)
	{
		discard;
	}

	// --------------------------------------------------
	// ★ カラースフィア（複数バリア赤色化）処理 (b10 は inc_KdCommon.hlsli で定義済み)
	// --------------------------------------------------
	if (g_colorEnable && g_colorCount > 0)
	{
		int sphereLoop = min(g_colorCount, MAX_EFFECT_SPHERES);
		for (int s = 0; s < sphereLoop; ++s)
		{
			float3 v = g_colorList[s].Pos - In.wPos;
			if (length(v) < g_colorList[s].Radius)
			{
				// 赤色着色（1つでもヒットしたら高速脱出）
				float gray = (baseColor.r + baseColor.g + baseColor.b) / 3.0f;
				baseColor.rgb = float3(gray, 0.0f, 0.0f) + g_colorList[s].Color * 0.5f;
				break;
			}
		}
	}

	// カメラへの方向
	float3 vCam = g_CamPos - In.wPos;
	float camDist = length(vCam);
	vCam = normalize(vCam);

	// 法線マップから法線ベクトル取得
	float3 wN = g_normalTex.Sample(g_ss, In.UV).rgb * 2.0 - 1.0;
	{
		row_major float3x3 mTBN =
		{
			normalize(In.wT),
			normalize(In.wB),
			normalize(In.wN),
		};
		wN = mul(wN, mTBN);
	}
	wN = normalize(wN);

	float4 mr = g_metalRoughTex.Sample(g_ss, In.UV);
	float metallic = mr.b * g_Metallic;
	float roughness = mr.g * g_Roughness;
	float smoothness = 1.0 - roughness;
	float specPower = pow(2, 11 * smoothness);
	
	//------------------------------------------
	// ライティング
	//------------------------------------------
	float3 outColor = 0;
	const float3 baseDiffuse = lerp(baseColor.rgb, float3(0, 0, 0), metallic);
	const float3 baseSpecular = lerp(0.04, baseColor.rgb, metallic);

	// シャドウマッピング
	float shadow = 1;
	float4 liPos = mul(float4(In.wPos, 1), g_DL_mLightVP);
	liPos.xyz /= liPos.w;

	if (abs(liPos.x) <= 1 && abs(liPos.y) <= 1 && liPos.z <= 1)
	{
		float2 uv = liPos.xy * float2(1, -1) * 0.5 + 0.5;
		float z = liPos.z - 0.004;
		
		float w, h;
		g_dirShadowMap.GetDimensions(w, h);
		float tw = 1.0 / w;
		float th = 1.0 / h;
	
		shadow = 0;
		for (int y = -1; y <= 1; y++)
		{
			for (int x = -1; x <= 1; x++)
			{
				shadow += g_dirShadowMap.SampleCmpLevelZero(g_ssCmp, uv + float2(x * tw, y * th), z);
			}
		}
		shadow *= 0.11;
	}
		
	// 平行光 Diffuse & Specular
	{
		float lightDiffuse = saturate(dot(-g_DL_Dir, wN)) / 3.1415926535;
		outColor += (g_DL_Color * lightDiffuse) * baseDiffuse * baseColor.a * shadow;

		float spec = BlinnPhong(g_DL_Dir, vCam, wN, specPower);
		outColor += (g_DL_Color * spec) * baseSpecular * baseColor.a * 0.5 * shadow;
	}

	float totalBrightness = g_AmbientLight.a;

	// 点光
	for (int i = 0; i < g_PointLightNum.x; i++)
	{
		float3 dir = g_PointLights[i].Pos - In.wPos;
		float dist = length(dir);
		dir /= dist;
		
		if (dist < g_PointLights[i].Radius)
		{
			float atte = 1.0 - saturate(dist / g_PointLights[i].Radius);
			totalBrightness += (1 - pow(1 - atte, 2)) * g_PointLights[i].IsBright;
			atte *= atte;
			
			float lightDiffuse = saturate(dot(dir, wN)) * atte / 3.1415926535;
			outColor += (g_PointLights[i].Color * lightDiffuse) * baseDiffuse * baseColor.a;

			float spec = BlinnPhong(-dir, vCam, wN, specPower) * atte;
			outColor += (g_PointLights[i].Color * spec) * baseSpecular * baseColor.a * 0.5;
		}
	}

	// --------------------------------------------------
	// ★ 2.5D コーンライト (複数対応＆最適化) (b11)
	// --------------------------------------------------
	if (g_ConeLightCount > 0)
	{
		static const float EYE_HEIGHT = 0.5f;
		int lightLoop = min(g_ConeLightCount, MAX_CONE_LIGHTS);

		for (int i = 0; i < lightLoop; ++i)
		{
			ConeLightData light = g_ConeLights[i];

			float3 coneDirXZ = float3(light.Dir.x, 0.0f, light.Dir.z);
			if (length(coneDirXZ) > 0.001f)
				coneDirXZ = normalize(coneDirXZ);
			else
				coneDirXZ = float3(1.0f, 0.0f, 0.0f);

			float3 lightVecXZ = In.wPos - light.Pos;
			lightVecXZ.y = 0.0f;
			float distXZ = length(lightVecXZ);
			float deltaY = In.wPos.y - light.Pos.y;

			// --- 1. 床面への照射 ---
			if (distXZ <= light.Range && distXZ > 0.001f && deltaY >= -0.1f && deltaY <= EYE_HEIGHT)
			{
				float3 lightDirXZ = lightVecXZ / distXZ;
				float cosAngle = dot(lightDirXZ, coneDirXZ);
				float halfAngleRad = radians(light.Angle * 0.5f);

				if (cosAngle >= cos(halfAngleRad) && wN.y >= 0.1f)
				{
					bool isBeforeWall = true;
					if (light.IsHit)
					{
						float distToPixel = dot(In.wPos - light.Pos, coneDirXZ);
						float distToWall = dot(light.HitPos - light.Pos, coneDirXZ);
						if (distToPixel > distToWall + 0.05f)
						{
							isBeforeWall = false;
						}
					}

					if (isBeforeWall)
					{
						float maxIntensity = max(light.Color.r, max(light.Color.g, light.Color.b));
						float3 normColor = (maxIntensity > 0.0f) ? (light.Color / maxIntensity) : float3(0, 0, 0);
						outColor += normColor * light.Intensity * baseDiffuse * baseColor.a;
					}
				}
			}

			// --- 2. 壁面へのヒット演出 ---
			if (light.IsHit && wN.y < 0.1f)
			{
				float wallNdotL = dot(wN, -coneDirXZ);
				if (wallNdotL > 0.2f)
				{
					float3 hitToPixel = In.wPos - light.HitPos;
					hitToPixel.y = 0.0f;

					float halfAngleRad = radians(light.Angle * 0.5f);
					float distToHit = length(light.HitPos - light.Pos);
					float wallRadius = distToHit * tan(halfAngleRad);

					if (length(hitToPixel) <= wallRadius + 0.3f)
					{
						float3 groundHitPos = float3(light.HitPos.x, 0.0f, light.HitPos.z);
						float3 hitToPos = In.wPos - groundHitPos;

						float3 N = normalize(light.HitNormal);
						N.y = 0.0f;
						if (length(N) > 0.001f)
							N = normalize(N);
						else
							N = -coneDirXZ;

						float3 Tangent = float3(-N.z, 0.0f, N.x);
						float u = dot(hitToPos, Tangent);
						float localY = In.wPos.y - light.Pos.y;
						float v = localY / 1.5f;

						float distFromGroundCenter = sqrt(u * u + v * v);

						if (In.wPos.y >= -0.05f && localY <= EYE_HEIGHT && distFromGroundCenter <= wallRadius)
						{
							float maxIntensity = max(light.Color.r, max(light.Color.g, light.Color.b));
							float3 normColor = (maxIntensity > 0.0f) ? (light.Color / maxIntensity) : float3(0, 0, 0);
							outColor += normColor * light.Intensity * baseDiffuse * baseColor.a;
						}
					}
				}
			}
		}
	}

	outColor += g_AmbientLight.rgb * baseColor.rgb * baseColor.a;
	
	// 自己発光色
	if (g_OnlyEmissie)
	{
		outColor = g_emissiveTex.Sample(g_ss, In.UV).rgb * g_Emissive * In.Color.rgb;
	}
	else
	{
		outColor += g_emissiveTex.Sample(g_ss, In.UV).rgb * g_Emissive * In.Color.rgb;
	}
	
	// フォグ処理
	if (g_HeightFogEnable && g_FogEnable && In.wPos.y < g_HeightFogTopValue)
	{
		float distRate = pow(saturate(length(In.wPos - g_CamPos) / g_HeightFogDistance), 2.0);
		float heightRate = 1 - saturate((In.wPos.y - g_HeightFogBottomValue) / (g_HeightFogTopValue - g_HeightFogBottomValue));
		outColor = lerp(outColor, g_HeightFogColor, heightRate * distRate);
	}
	
	if (g_DistanceFogEnable && g_FogEnable)
	{
		float f = saturate(1.0 / exp(camDist * g_DistanceFogDensity));
		outColor = lerp(g_DistanceFogColor, outColor, f);
	}
	
	// ディゾルブ
	if (g_dissolveValue > 0 && abs(discardValue - g_dissolveValue) < g_dissolveEdgeRange)
	{
		outColor += g_dissolveEmissive;
	}
	
	outColor *= saturate(totalBrightness);

	//------------------------------------------
	// 出力
	//------------------------------------------
	// 10/5 追加
	if (ColorEnable)
	{
		// 色
		// float4 (x y z w)
		// float4 (float3, float);	←これ採用
		// float4 (float2, float2);
		// outColor：最終的に塗られる色
		return float4(outColor + float3(0, 0, 0), baseColor.a - 0.6f);
	}
	else
	{
		return float4(outColor, baseColor.a);
	}
	//	return float4(outColor, baseColor.a);
}
