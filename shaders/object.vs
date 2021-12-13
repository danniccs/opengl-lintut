#version 430 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNorm;
layout(location = 2) in vec2 aTexCoords;
layout(location = 3) in vec3 aTangent;
layout(location = 5) in mat4 aModel;
layout(location = 9) in mat3 aNormMat;

uniform vec3 viewPos;
uniform vec3 dirLightDir;
uniform vec3 spotLightPos;
uniform vec3 spotLightDir;

uniform mat4 view;
uniform mat4 projection;
uniform mat4 dirSpaceMat;
uniform mat4 spotSpaceMat;

out VS_OUT {
  vec3 worldFragPos;
  vec2 texCoords;

  vec3 frenetFragPos;
  vec3 frenetViewPos;
  vec3 frenetLightDir;
  vec3 frenetSpotPos;
  vec3 frenetSpotDir;

  vec4 fragPosDirSpace;
  vec4 fragPosSpotSpace;
}
vs_out;

void main() {
  gl_Position = projection * view * aModel * vec4(aPos, 1.0);

  vs_out.worldFragPos = vec3(aModel * vec4(aPos, 1.0));
  vs_out.texCoords = aTexCoords;

  // Fragment position in the view space of the lights.
  vs_out.fragPosDirSpace = dirSpaceMat * vec4(vs_out.worldFragPos, 1.0);
  vs_out.fragPosSpotSpace = spotSpaceMat * vec4(vs_out.worldFragPos, 1.0);

  // Construct tangent space matrix for normal mapping.
  vec3 T = normalize(aNormMat * aTangent);
  vec3 N = normalize(aNormMat * aNorm);
  // Use Gram-Schmidt to re-orthogonalize.
  T = normalize(T - dot(T, N) * N);
  vec3 B = cross(N, T);
  mat3 TBN = transpose(mat3(T, B, N));

  // Send Frenet frame coordinate positions.
  vs_out.frenetFragPos = TBN * vec3(aModel * vec4(aPos, 1.0));
  vs_out.frenetViewPos = TBN * viewPos;
  vs_out.frenetLightDir = TBN * dirLightDir;
  vs_out.frenetSpotPos = TBN * spotLightPos;
  vs_out.frenetSpotDir = TBN * spotLightDir;
}