#version 430 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNorm;
layout(location = 2) in vec2 aTexCoords;
layout(location = 3) in vec3 aTangent;

uniform vec3 viewPos;
uniform vec3 dirLightDir;
uniform vec3 spotLightPos;
uniform vec3 spotLightDir;
uniform vec3 tubeLightPos;
uniform vec3 tubeP0;
uniform vec3 tubeP1;

uniform mat4 model;
uniform mat3 normMat;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 dirSpaceMat;
uniform mat4 spotSpaceMat;
uniform mat4 tubeSpaceMat;

out VS_OUT {
  vec3 worldFragPos;
  vec2 texCoords;

  vec3 frenetFragPos;
  vec3 frenetViewPos;
  vec3 frenetLightDir;
  vec3 frenetSpotPos;
  vec3 frenetSpotDir;
  vec3 frenetTubePos;
  vec3 frenetP0;
  vec3 frenetP1;

  vec4 fragPosDirSpace;
  vec4 fragPosSpotSpace;
  vec4 fragPosTubeSpace;
}
vs_out;

void main() {
  gl_Position = projection * view * model * vec4(aPos, 1.0);

  vs_out.worldFragPos = vec3(model * vec4(aPos, 1.0));
  vs_out.texCoords = aTexCoords;

  // Fragment position in the view space of the lights.
  vs_out.fragPosDirSpace = dirSpaceMat * vec4(vs_out.worldFragPos, 1.0);
  vs_out.fragPosSpotSpace = spotSpaceMat * vec4(vs_out.worldFragPos, 1.0);
  vs_out.fragPosTubeSpace = tubeSpaceMat * vec4(vs_out.worldFragPos, 1.0);

  // Construct tangent space matrix for normal mapping.
  vec3 T = normalize(normMat * aTangent);
  vec3 N = normalize(normMat * aNorm);
  // Use Gram-Schmidt to re-orthogonalize.
  T = normalize(T - dot(T, N) * N);
  vec3 B = cross(N, T);
  mat3 TBN = transpose(mat3(T, B, N));

  // Send Frenet frame coordinate positions.
  vs_out.frenetFragPos = TBN * vec3(model * vec4(aPos, 1.0));
  vs_out.frenetViewPos = TBN * viewPos;
  vs_out.frenetLightDir = TBN * dirLightDir;
  vs_out.frenetSpotPos = TBN * spotLightPos;
  vs_out.frenetSpotDir = TBN * spotLightDir;
  vs_out.frenetTubePos = TBN * tubeLightPos;
  vs_out.frenetP0 = TBN * tubeP0;
  vs_out.frenetP1 = TBN * tubeP1;
}