// Object3D.PS.hlsl
// ピクセルシェーダー

struct PixelShaderOutput
{
    float4 color : SV_TARGET;
};

// マテリアル情報を格納する定数バッファ
struct Material
{
    float4 color;
    int enableLighting;
    float shininess;
    float2 padding;
    matrix uvTransform;
};

// 平行光源の情報を格納する定数バッファ
struct DirectionalLight
{
    float4 color;
    float3 direction;
    float intensity;
};

// ライティングモデルを選択するための定数バッファ
struct LightingSettings
{
    int lightingModel; // 0: Lambert, 1: Half-Lambert
    float3 padding;
};

ConstantBuffer<Material> gMaterial : register(b0);
ConstantBuffer<DirectionalLight> gDirectionalLight : register(b1);
ConstantBuffer<LightingSettings> gLightingSettings : register(b2);

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

// 頂点シェーダーからの入力
struct PixelInput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
    float3 worldPosition : WORLDPOSITION0;
};

PixelShaderOutput main(PixelInput input)
{
    PixelShaderOutput output;

    float4 texColor = float4(1.0, 1.0, 1.0, 1.0); // UV無い時は白

    // UVがあるときだけサンプル（例：shininess >= 0 が目印）
    if (gMaterial.shininess >= 0.0f)
    {
        float2 transformedUV = mul(float4(input.texcoord, 0.0, 1.0), gMaterial.uvTransform).xy;
        texColor = gTexture.Sample(gSampler, transformedUV);
    }

    // マテリアルの色を乗算
    output.color = texColor * gMaterial.color;

    // ライティングが有効な場合
    if (gMaterial.enableLighting != 0)
    {
        float3 N = normalize(input.normal);
        float3 L = normalize(gDirectionalLight.direction);
        float NdotL = dot(N, L);

        float diffuse = 0.0f;
        if (gLightingSettings.lightingModel == 0)
        {
            diffuse = saturate(NdotL);
        }
        else
        {
            diffuse = NdotL * 0.5f + 0.5f;
            diffuse = diffuse * diffuse;
        }

        float3 light = gDirectionalLight.color.rgb * gDirectionalLight.intensity * diffuse;

        // 光を反映
        output.color.rgb *= light;
    }

    return output;
}
