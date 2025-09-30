#include "Particle.hlsli"

// 頂点データの入力構造体
struct VertexInput
{
    float32_t4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float32_t3 normal : NORMAL0;
};

// ピクセルシェーダーへの出力構造体
struct VertexOutput
{
    float32_t4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float32_t3 normal : NORMAL0;
    float32_t3 worldPosition : WORLDPOSITION0;
};

// 変換行列を格納する定数バッファ
struct TransformationMatrix
{
    matrix WVP; // World * View * Projection
    matrix World; // World
};

StructuredBuffer<TransformationMatrix> gTransformationMatrices : register(t1);

VertexOutput main(VertexInput input,uint32_t instanceId:SV_InstanceID)
{
    VertexOutput output;
    
    // 座標変換
    output.position = mul(input.position, gTransformationMatrices[instanceId].WVP);
    
    // UV座標をそのまま渡す
    output.texcoord = input.texcoord;
    
    // 法線をワールド空間に変換して正規化
    output.normal = normalize(mul(input.normal, (float32_t3x3) gTransformationMatrices[instanceId].World));
    
    // 頂点位置をワールド空間に変換
    output.worldPosition = mul(input.position, gTransformationMatrices[instanceId].World).xyz;
    
    return output;
}
