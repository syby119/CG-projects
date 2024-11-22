#ifndef __COMMON_GLSL__
#define __COMMON_GLSL__

// Display the material field in UI
//     layout(location = 0) vec3 MTL_FILED(myvec3);
// Use the decorated name in glsl code
//     vec3 ret = MTL_FILED(myvec3);
#define MTL_FIELD(name) _MATERIAL_FIELD_##name

// Display the material field in UI, with color picker
//     layout(location = 0) vec3 MTL_FIELD_COLOR(baseColor);
// Use the decorated name in glsl code
//     vec3 color = MTL_FIELD_COLOR(baseColor);
#define MTL_FIELD_COLOR(name) _MATERIAL_FIELD_COLOR_##name

#endif /* __COMMON_GLSL__ */