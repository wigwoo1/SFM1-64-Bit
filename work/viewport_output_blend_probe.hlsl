struct DrawInput {
    float4 color;
    uint mode;
};
[[vk::push_constant]] ConstantBuffer<DrawInput> drawInput;
float4 main(float4 position : SV_Position) : SV_Target0 {
    uint x = uint(position.x);
    float4 color = drawInput.color;
    if (drawInput.mode == 1) color.a = float(x) / 255.0;
    if (drawInput.mode == 2) color = float4(float(x) / 255.0,
        float((x + 47) % 256) / 255.0, float((x + 137) % 256) / 255.0,
        float((x + 71) % 256) / 255.0);
    return color;
}
