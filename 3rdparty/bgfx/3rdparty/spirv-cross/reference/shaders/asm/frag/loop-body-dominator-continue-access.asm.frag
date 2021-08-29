#version 450

layout(binding = 0, std140) uniform Foo
{
    layout(row_major) mat4 lightVP[64];
    uint shadowCascadesNum;
    int test;
} _11;

layout(location = 0) in vec3 fragWorld;
layout(location = 0) out int _entryPointOutput;

mat4 GetClip2TexMatrix()
{
    if (_11.test == 0)
    {
        return mat4(vec4(0.5, 0.0, 0.0, 0.0), vec4(0.0, 0.5, 0.0, 0.0), vec4(0.0, 0.0, 0.5, 0.0), vec4(0.0, 0.0, 0.0, 1.0));
    }
    return mat4(vec4(1.0, 0.0, 0.0, 0.0), vec4(0.0, 1.0, 0.0, 0.0), vec4(0.0, 0.0, 1.0, 0.0), vec4(0.0, 0.0, 0.0, 1.0));
}

int GetCascade(vec3 fragWorldPosition)
{
    for (uint cascadeIndex = 0u; cascadeIndex < _11.shadowCascadesNum; cascadeIndex++)
    {
        mat4 worldToShadowMap = GetClip2TexMatrix() * _11.lightVP[cascadeIndex];
        vec4 fragShadowMapPos = worldToShadowMap * vec4(fragWorldPosition, 1.0);
        if ((((fragShadowMapPos.z >= 0.0) && (fragShadowMapPos.z <= 1.0)) && (max(fragShadowMapPos.x, fragShadowMapPos.y) <= 1.0)) && (min(fragShadowMapPos.x, fragShadowMapPos.y) >= 0.0))
        {
            return int(cascadeIndex);
        }
    }
    return -1;
}

int _main(vec3 fragWorld_1)
{
    vec3 param = fragWorld_1;
    return GetCascade(param);
}

void main()
{
    vec3 fragWorld_1 = fragWorld;
    vec3 param = fragWorld_1;
    _entryPointOutput = _main(param);
}

