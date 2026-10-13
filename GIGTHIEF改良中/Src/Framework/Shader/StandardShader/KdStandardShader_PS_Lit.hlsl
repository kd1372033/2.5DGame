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

float BlinnPhong(float3 lightDir, float3 vCam, float3 normal, float specPower)
{
	float3 H = normalize(-lightDir + vCam);
	float NdotH = saturate(dot(normal, H)); // カメラの角度差(0～1)
	float spec = pow(NdotH, specPower);

	// 正規化Blinn-Phong
	return spec * ((specPower + 2) / (2 * 3.1415926535));
}

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

cbuffer cbConeLight : register(b10) // ※使用中のレジスタ番号に合わせてください
{
	int g_ConeLightCount;
	float3 g_ConeLightPadding;
	ConeLightData g_ConeLights[MAX_CONE_LIGHTS];
};

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
	
	// カメラへの方向
	float3 vCam = g_CamPos - In.wPos;
	float camDist = length(vCam); // カメラ - ピクセル距離
	vCam = normalize(vCam);

	// 法線マップから法線ベクトル取得
	float3 wN = g_normalTex.Sample(g_ss, In.UV).rgb;

	// UV座標（0～1）から 射影座標（-1～1）へ変換
	wN = wN * 2.0 - 1.0;
	
	{
		// 3種の法線から法線行列を作成
		row_major float3x3 mTBN =
		{
			normalize(In.wT),
			normalize(In.wB),
			normalize(In.wN),
		};
	
		// 法線ベクトルをこのピクセル空間へ変換
		wN = mul(wN, mTBN);
	}

	// 法線正規化
	wN = normalize(wN);

	float4 mr = g_metalRoughTex.Sample(g_ss, In.UV);
	// 金属性
	float metallic = mr.b * g_Metallic;
	// 粗さ
	float roughness = mr.g * g_Roughness;
	// ラフネスを逆転させ「滑らか」さにする
	float smoothness = 1.0 - roughness;
	float specPower = pow(2, 11 * smoothness); // 1～2048
	
	//------------------------------------------
	// ライティング
	//------------------------------------------
	// 最終的な色
	float3 outColor = 0;
	
		// 材質の拡散色 非金属ほど材質の色になり、金属ほど拡散色は無くなる
	const float3 baseDiffuse = lerp(baseColor.rgb, float3(0, 0, 0), metallic);
		// 材質の反射色 非金属ほど光の色をそのまま反射し、金属ほど材質の色が乗る
	const float3 baseSpecular = lerp(0.04, baseColor.rgb, metallic);

	//-------------------------------
	// シャドウマッピング(影判定)
	//-------------------------------
	float shadow = 1;

	// ピクセルの3D座標から、DepthMapFromLight空間へ変換
	float4 liPos = mul(float4(In.wPos, 1), g_DL_mLightVP);
	liPos.xyz /= liPos.w;

	// 深度マップの範囲内？
	if (abs(liPos.x) <= 1 && abs(liPos.y) <= 1 && liPos.z <= 1)
	{
		// 射影座標 -> UV座標へ変換
		float2 uv = liPos.xy * float2(1, -1) * 0.5 + 0.5;
		// ライトカメラからの距離
		float z = liPos.z - 0.004; // シャドウアクネ対策
		
		// 画像のサイズからテクセルサイズを求める
		float w, h;
		g_dirShadowMap.GetDimensions(w, h);
		float tw = 1.0 / w;
		float th = 1.0 / h;
	
		// uvの周辺3x3も判定し、平均値を求める
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
		
	//-------------------------
	// 平行光
	//-------------------------
	// Diffuse(拡散光)
	{
		// 光の方向と法線の方向との角度さが光の強さになる
		float lightDiffuse = dot(-g_DL_Dir, wN);
		lightDiffuse = saturate(lightDiffuse); // マイナス値は0にする 0(暗)～1(明)になる

		// 正規化Lambert
		lightDiffuse /= 3.1415926535;

		// 光の色 * 材質の拡散色 * 透明率
		outColor += (g_DL_Color * lightDiffuse) * baseDiffuse * baseColor.a * shadow;
	}

	// Specular(反射色)
	{
		// 反射した光の強さを求める
		// Blinn-Phong NDF
		float spec = BlinnPhong(g_DL_Dir, vCam, wN, specPower);

		// 光の色 * 反射光の強さ * 材質の反射色 * 透明率 * 適当な調整値
		outColor += (g_DL_Color * spec) * baseSpecular * baseColor.a * 0.5 * shadow;
	}

	// 全体の明度：環境光に1が設定されている場合は影響なし
	// 環境光の不透明度を下げる事により、明度ライトの周り以外は描画されなくなる
	float totalBrightness = g_AmbientLight.a;

	//-------------------------
	// 点光
	//-------------------------
	for (int i = 0; i < g_PointLightNum.x; i++)
	{
		// ピクセルから点光への方向
		float3 dir = g_PointLights[i].Pos - In.wPos;
		
		// 距離を算出
		float dist = length(dir);
		
		// 正規化
		dir /= dist;
		
		// 点光の判定以内
		if (dist < g_PointLights[i].Radius)
		{
			// 半径をもとに、距離の比率を求める
			float atte = 1.0 - saturate(dist / g_PointLights[i].Radius);
			
			// 明度の追加
			totalBrightness += (1 - pow(1 - atte, 2)) * g_PointLights[i].IsBright;
			
			// 逆２乗の法則
			atte *= atte;
			
			// Diffuse(拡散光)
			{
				// 光の方向と法線の方向との角度さが光の強さになる
				float lightDiffuse = dot(dir, wN);
				lightDiffuse = saturate(lightDiffuse); // マイナス値は0にする 0(暗)～1(明)になる

				lightDiffuse *= atte; // 減衰

				// 正規化Lambert
				lightDiffuse /= 3.1415926535;

				// 光の色 * 材質の拡散色 * 透明率
				outColor += (g_PointLights[i].Color * lightDiffuse) * baseDiffuse * baseColor.a;
			}

			// Specular(反射色)
			{
				// 反射した光の強さを求める
				// Blinn-Phong NDF
				float spec = BlinnPhong(-dir, vCam, wN, specPower);

				spec *= atte; // 減衰
				
				// 光の色 * 反射光の強さ * 材質の反射色 * 透明率 * 適当な調整値
				outColor += (g_PointLights[i].Color * spec) * baseSpecular * baseColor.a * 0.5;
			}
		}
	}

	// --------------------------------------------------
// 2.5D コーンライト (複数対応ループ描画)
// --------------------------------------------------
	if (g_ConeLightCount > 0)
	{
		for (int i = 0; i < g_ConeLightCount; ++i)
		{
			ConeLightData light = g_ConeLights[i];

			float3 coneDirXZ = float3(light.Dir.x, 0.0f, light.Dir.z);
			if (length(coneDirXZ) > 0.001f)
				coneDirXZ = normalize(coneDirXZ);
			else
				coneDirXZ = float3(1.0f, 0.0f, 0.0f);

		// --- 1. 床面への照射 ---
			float3 lightVecXZ = In.wPos - light.Pos;
			lightVecXZ.y = 0.0f; // XZ平面上の距離計算
			float distXZ = length(lightVecXZ);

			if (distXZ <= light.Range && distXZ > 0.001f)
			{
				float3 lightDirXZ = lightVecXZ / distXZ;

			// 照射方向との角度（内積）判定
				float cosAngle = dot(lightDirXZ, coneDirXZ);
			
				float halfAngleRad = radians(light.Angle * 0.5f);
				float coneCos = cos(halfAngleRad);

			// 扇形の内部 & 上向きポリゴン（床）への描画
				if (cosAngle >= coneCos && wN.y >= 0.1f)
				{
					bool isBeforeWall = true;

				// 壁のレイキャスト遮蔽判定
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

			// --- 2. 壁面へのヒット演出（見た目の比率を 1:1 に補正） ---
			if (light.IsHit && wN.y < 0.1f) // 壁面ポリゴン（垂直面）のみ対象
			{
				// 光の照射方向と壁の法線が正対している表側のみ判定
				float wallNdotL = dot(wN, -coneDirXZ);

				if (wallNdotL > 0.2f)
				{
					float3 hitToPixel = In.wPos - light.HitPos;
					hitToPixel.y = 0.0f; // XZ平面上での距離チェック

					// 扇形の半角ラジアン
					float halfAngleRad = radians(light.Angle * 0.5f);

					// 敵から着弾点までの実際の水平距離
					float distToHit = length(light.HitPos - light.Pos);

					// 着弾地点の壁上における標準半径
					float wallRadius = distToHit * tan(halfAngleRad);

					if (length(hitToPixel) <= wallRadius + 0.3f)
					{
						// 地面（Y=0）を起点とした着弾位置
						float3 groundHitPos = float3(light.HitPos.x, 0.0f, light.HitPos.z);
						float3 hitToPos = In.wPos - groundHitPos;

						// 壁の法線から水平な横方向（Tangent）を算出
						float3 N = normalize(light.HitNormal);
						N.y = 0.0f;
						if (length(N) > 0.001f)
							N = normalize(N);
						else
							N = -coneDirXZ;

						float3 Tangent = float3(-N.z, 0.0f, N.x);

						// 壁面上の 2D 座標 (u: 横幅, v: 高さ)
						float u = dot(hitToPos, Tangent);
					
						// ★ 縦方向のスケール補正（見下ろしカメラの遠近・角度で縦長に見えるのを防ぐ）
						// 1.5 〜 1.6 で割る（または 0.65 程度を乗算）ことで見た目をきれいな半円にします
						float v = In.wPos.y * 1.5f;

						// 地面中心からの距離計算
						float distFromGroundCenter = sqrt(u * u + v * v);

						// 地面以上かつ半径以内なら描画
						if (In.wPos.y >= -0.05f && distFromGroundCenter <= wallRadius)
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
	
	// 自己発光色の適応
	if (g_OnlyEmissie)
	{
		outColor = g_emissiveTex.Sample(g_ss, In.UV).rgb * g_Emissive * In.Color.rgb;
	}
	else
	{
		outColor += g_emissiveTex.Sample(g_ss, In.UV).rgb * g_Emissive * In.Color.rgb;
	}
	
	//------------------------------------------
	// 高さフォグ
	//------------------------------------------
	if (g_HeightFogEnable && g_FogEnable)
	{
		if (In.wPos.y < g_HeightFogTopValue)
		{
			float distRate = length(In.wPos - g_CamPos);
			distRate = saturate(distRate / g_HeightFogDistance);
			distRate = pow(distRate, 2.0);
			
			float heightRange = g_HeightFogTopValue - g_HeightFogBottomValue;
			float heightRate = 1 - saturate((In.wPos.y - g_HeightFogBottomValue) / heightRange);
			
			float fogRate = heightRate * distRate;
			outColor = lerp(outColor, g_HeightFogColor, fogRate);
		}
	}
	
	//------------------------------------------
	// 距離フォグ
	//------------------------------------------
	if (g_DistanceFogEnable && g_FogEnable)
	{
		// フォグ 1(近い)～0(遠い)
		float f = saturate(1.0 / exp(camDist * g_DistanceFogDensity));
		
		// 適用
		outColor = lerp(g_DistanceFogColor, outColor, f);
	}
	
	// ディゾルブ輪郭発光
	if (g_dissolveValue > 0)
	{
		// 閾値とマスク値の差分で、縁を検出
		if (abs(discardValue - g_dissolveValue) < g_dissolveEdgeRange)
		{
			// 輪郭に発光色追加
			outColor += g_dissolveEmissive;
		}
	}
	
	totalBrightness = saturate(totalBrightness);
	outColor *= totalBrightness;
	
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
		return float4(outColor + float3(0, 0, 0), baseColor.a - 0.5f);
	}
	else
	{
		return float4(outColor, baseColor.a);
	}
	//	return float4(outColor, baseColor.a);
}
