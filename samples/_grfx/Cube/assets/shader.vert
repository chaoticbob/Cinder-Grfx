/* 

Compile:
  glslangValidator.exe --target-env vulkan1.1 -R --hlsl-offsets --amb --aml -S vert -o shader.vert.spv shader.vert

*/
#version 430

//uniform Block0 {
    uniform mat4 ciModelViewProjection;
    uniform mat3 ciNormalMatrix;
//};

in vec4 ciPosition;
in vec2 ciTexCoord0;
in vec3 ciNormal;
//in vec4 ciColor;

out highp vec2	TexCoord;
out lowp  vec4	Color;
out highp vec3	Normal;

void main( void )
{
	gl_Position	= ciModelViewProjection * ciPosition;
	Color 		= vec4(1, 1, 1, 1); //ciColor;
	TexCoord	= ciTexCoord0;
	Normal		= ciNormalMatrix * ciNormal;
}
