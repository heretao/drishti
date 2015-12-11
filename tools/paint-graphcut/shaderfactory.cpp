#include "shaderfactory.h"

#include <QTextEdit>
#include <QVBoxLayout>
#include <QMessageBox>


bool
ShaderFactory::loadShader(GLhandleARB &progObj,
			  QString shaderString)
{
  GLhandleARB fragObj = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);  
  glAttachObjectARB(progObj, fragObj);

  GLhandleARB vertObj = glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB);  
  glAttachObjectARB(progObj, vertObj);

  {  // vertObj
    QString qstr;
    qstr = "varying vec3 pointpos;\n";
    qstr += "void main(void)\n";
    qstr += "{\n";
    qstr += "  // Transform vertex position into homogenous clip-space.\n";
    qstr += "  gl_FrontColor = gl_Color;\n";
    qstr += "  gl_BackColor = gl_Color;\n";
    qstr += "  gl_Position = ftransform();\n";
    qstr += "  gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;\n";
    qstr += "  gl_TexCoord[1] = gl_TextureMatrix[1] * gl_MultiTexCoord1;\n";
    qstr += "  gl_TexCoord[2] = gl_TextureMatrix[2] * gl_MultiTexCoord2;\n";
    qstr += "  gl_ClipVertex = gl_ModelViewMatrix * gl_Vertex;\n";
    qstr += "  pointpos = gl_Vertex.xyz;\n";
    qstr += "}\n";
    
    int len = qstr.length();
    char *tbuffer = new char[len+1];
    sprintf(tbuffer, qstr.toLatin1().data());
    const char *sstr = tbuffer;
    glShaderSourceARB(vertObj, 1, &sstr, NULL);
    delete [] tbuffer;

    GLint compiled = -1;
    glCompileShaderARB(vertObj);
    glGetObjectParameterivARB(vertObj,
			      GL_OBJECT_COMPILE_STATUS_ARB,
			      &compiled);
    if (!compiled)
      {
	GLcharARB str[1000];
	GLsizei len;
	glGetInfoLogARB(vertObj,
			(GLsizei) 1000,
			&len,
			str);
	
	QMessageBox::information(0,
				 "Error : Vertex Shader",
				 str);
	return false;
    }
  }
    
    
  { // fragObj
    int len = shaderString.length();
    char *tbuffer = new char[len+1];
    sprintf(tbuffer, shaderString.toLatin1().data());
    const char *sstr = tbuffer;
    glShaderSourceARB(fragObj, 1, &sstr, NULL);
    delete [] tbuffer;
  
    GLint compiled = -1;
    glCompileShaderARB(fragObj);
    glGetObjectParameterivARB(fragObj,
			    GL_OBJECT_COMPILE_STATUS_ARB,
			      &compiled);
    if (!compiled)
      {
	GLcharARB str[1000];
	GLsizei len;
	glGetInfoLogARB(fragObj,
			(GLsizei) 1000,
			&len,
			str);
	
	//-----------------------------------
	// display error
	
	//qApp->beep();
	
	QString estr;
	QStringList slist = shaderString.split("\n");
	for(int i=0; i<slist.count(); i++)
	  estr += QString("%1 : %2\n").arg(i+1).arg(slist[i]);
	
	QTextEdit *tedit = new QTextEdit();
	tedit->insertPlainText("-------------Error----------------\n\n");
	tedit->insertPlainText(str);
	tedit->insertPlainText("\n-----------Shader---------------\n\n");
	tedit->insertPlainText(estr);
	
	QVBoxLayout *layout = new QVBoxLayout();
	layout->addWidget(tedit);
	
	QDialog *showError = new QDialog();
	showError->setWindowTitle("Error in Fragment Shader");
	showError->setSizeGripEnabled(true);
	showError->setModal(true);
	showError->setLayout(layout);
	showError->exec();
	//-----------------------------------
	
	return false;
      }
  }

  
  //----------- link program shader ----------------------
  GLint linked = -1;
  glLinkProgramARB(progObj);
  glGetObjectParameterivARB(progObj, GL_OBJECT_LINK_STATUS_ARB, &linked);
  if (!linked)
    {
      GLcharARB str[1000];
      GLsizei len;
      QMessageBox::information(0,
			       "ProgObj",
			       "error linking texProgObj");
      glGetInfoLogARB(progObj,
		      (GLsizei) 1000,
		      &len,
		      str);
      QMessageBox::information(0,
			       "Error",
			       QString("%1\n%2").arg(len).arg(str));
      return false;
    }

  glDeleteObjectARB(fragObj);
  glDeleteObjectARB(vertObj);

  return true;
}

QString
ShaderFactory::genSliceShader()
{
  QString shader;

  shader =  "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "uniform sampler3D dataTex;\n";
  shader += "uniform sampler2D lutTex;\n";
  shader += "void main(void)\n";
  shader += "{\n";

  shader += "float val = texture3D(dataTex, gl_TexCoord[0].xyz).x;\n";
  shader += "vec4 color = texture2D(lutTex, vec2(val,0.0));\n";
  shader += "gl_FragColor = color;\n";    

  shader += "}\n";

  return shader;
}


QString
ShaderFactory::genDepthShader()
{
  QString shader;

  shader =  "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "varying vec3 pointpos;\n";
  shader += "uniform float minZ;\n";
  shader += "uniform float maxZ;\n";
  shader += "uniform vec3 eyepos;\n";
  shader += "uniform vec3 viewDir;\n";
  shader += "void main(void)\n";
  shader += "{\n";

  shader += "  vec3 voxpos = pointpos;\n";
  shader += "  vec3 I = voxpos - eyepos;\n";
  shader += "  float z = dot(I, viewDir);\n";
  shader += "  z = (z-minZ)/(maxZ-minZ);\n";
  shader += "  z = clamp(z, 0.0, 1.0);\n";

  shader += " vec2 rdxy = 2.0*(gl_TexCoord[0].xy-vec2(0.5,0.5));\n";
  shader += " rdxy *= rdxy;\n";
  shader += " float rd = sqrt(rdxy.x + rdxy.y);\n";
  shader += " rd = clamp(rd, 0.0, 1.0);\n";
  
  shader += " gl_FragColor = vec4(z,z,z,smoothstep(rd, 0.9, 0.99));\n";
  shader += "}\n";

  return shader;
}

QString
ShaderFactory::genFinalPointShader()
{
  QString shader;

  shader =  "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "varying vec3 pointpos;\n";
  shader += "uniform sampler2DRect blurTex;\n";
  shader += "uniform float minZ;\n";
  shader += "uniform float maxZ;\n";
  shader += "uniform vec3 eyepos;\n";
  shader += "uniform vec3 viewDir;\n";
  shader += "uniform float dzScale;\n";
  shader += "void main(void)\n";
  shader += "{\n";
  shader += "  gl_FragColor = vec4(0,0,0,0);\n";

  shader += "  vec2 spos = gl_FragCoord.xy;\n";
  shader += "  float depth = texture2DRect(blurTex, spos).x;\n";

  // find depth gradient
  shader += "  float dx = texture2DRect(blurTex, spos+vec2(1.0,0.0)).x - texture2DRect(blurTex, spos-vec2(1.0,0.0)).x;\n";
  shader += "  float dy = texture2DRect(blurTex, spos+vec2(0.0,1.0)).x - texture2DRect(blurTex, spos-vec2(0.0,1.0)).x;\n";
  shader += "  vec3 norm = normalize(vec3(dx, dy, dzScale/(maxZ-minZ)));\n";

  
  // sprite circle radius
  shader += "  vec2 rdxy = 2.0*(gl_TexCoord[0].xy-vec2(0.5,0.5));\n";
  shader += "  rdxy *= rdxy;\n";
  shader += "  float rd = sqrt(rdxy.x + rdxy.y);\n";
  shader += "  rd = clamp(rd, 0.0, 1.0);\n";


  // compute obscurance
  shader += "  float ege1 = 0.01;\n";
  shader += "  float ege2 = 0.03;\n";
  shader += "  float sum = 0.0;\n";
  shader += "  float od = 0.0;\n";

  shader += "  float cx[16] = {-1.5, 1.5, 0.0, 0.0,-2.5,-2.5, 2.5, 2.5,-3.5, 3.5, 0.0, 0.0,-4.5,-4.5, 4.5, 4.5};\n";
  shader += "  float cy[16] = { 0.0, 0.0,-1.5, 1.5,-2.5, 2.5,-2.5, 2.5, 0.0, 0.0,-3.5, 3.5,-4.5, 4.5,-4.5, 4.5};\n";

  shader += "  for(int i=0; i<8; i++)\n";
  shader += "  {\n";
  shader += "    float od = depth - texture2DRect(blurTex, spos+vec2(cx[i],cy[i])).x;\n";
  shader += "    sum += step(ege1, od);\n";
  shader += "  }\n";

  shader += "  for(int i=8; i<16; i++)\n";
  shader += "  {\n";
  shader += "    float od = depth - texture2DRect(blurTex, spos+vec2(cx[i],cy[i])).x;\n";
  shader += "    sum += step(ege2, od);\n";
  shader += "  }\n";

  shader += "  float f = 0.2+0.8*(1.0-sum/16.0);\n";

  shader += "  float alpha = smoothstep(rd, 0.8, 0.99);\n";

  shader += "  gl_FragColor = vec4(alpha*f*norm.z*gl_Color.rgb, alpha);\n";
  //-----------------------

  shader += "}\n";

  return shader;
}

QString
ShaderFactory::genRectBlurShaderString(int filter)
{
  QString shader;

  shader = "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "uniform sampler2DRect blurTex;\n";
  shader += "uniform float minZ;\n";
  shader += "uniform float maxZ;\n";
  shader += "\n";
  shader += "void main(void)\n";
  shader += "{\n";
  shader += "  vec4 color;\n";
  shader += "  vec2 spos = gl_FragCoord.xy;\n";
  shader += "\n";
  
  if (filter == 1) // bilateral filter
    {
      shader += "  float cx[8] = {-1.0,-1.0,-1.0, 0.0, 0.0, 1.0, 1.0, 1.0};\n";
      shader += "  float cy[8] = {-1.0, 0.0, 1.0,-1.0, 1.0,-1.0, 0.0, 1.0};\n";
      shader += "  float depth = texture2DRect(blurTex, spos.xy).x;\n";
      shader += "  float odepth = minZ + depth*(maxZ-minZ);\n";
      shader += "  float col = depth;\n";
      shader += "  float sum = 1.0;\n";
      shader += "  for(int i=0; i<8; i++)\n";
      shader += "  {\n";
      shader += "    float z = texture2DRect(blurTex, spos.xy + vec2(cx[i],cy[i])).x;\n";
      shader += "    float oz = minZ + z*(maxZ-minZ);\n";
      shader += "    float fi = (odepth-oz)*0.2;\n";
      shader += "    fi = exp(-fi*fi);\n";
      shader += "    col += z*fi;\n";
      shader += "    sum += fi;\n";
      shader += "  }\n";

      shader += "  gl_FragColor.rgba = vec4(col/sum, color.yz, col/sum);\n";
      //shader += "  gl_FragColor.rgba = vec4(col/sum, color.yz, 1.0);\n";
    }
  else if (filter == 2)
    { // gaussian filter
      shader += "  color  = vec4(4.0,4.0,4.0,4.0)*texture2DRect(blurTex, spos.xy);\n";
      shader += "  color += vec4(2.0,2.0,2.0,2.0)*texture2DRect(blurTex, spos.xy + vec2( 0.0, 1.0));\n";
      shader += "  color += vec4(2.0,2.0,2.0,2.0)*texture2DRect(blurTex, spos.xy + vec2( 0.0,-1.0));\n";
      shader += "  color += vec4(2.0,2.0,2.0,2.0)*texture2DRect(blurTex, spos.xy + vec2( 1.0, 0.0));\n";
      shader += "  color += vec4(2.0,2.0,2.0,2.0)*texture2DRect(blurTex, spos.xy + vec2(-1.0, 0.0));\n";
      shader += "  color += texture2DRect(blurTex, spos.xy + vec2( 1.0, 1.0));\n";
      shader += "  color += texture2DRect(blurTex, spos.xy + vec2( 1.0,-1.0));\n";
      shader += "  color += texture2DRect(blurTex, spos.xy + vec2(-1.0, 1.0));\n";
      shader += "  color += texture2DRect(blurTex, spos.xy + vec2(-1.0,-1.0));\n";
      shader += "  gl_FragColor.rgba = color/16.0;\n";
    }
  else if (filter == 3)
    { // sharpness filter
      shader += "  color = vec4(8.0,8.0,8.0,8.0)*texture2DRect(blurTex, spos.xy);\n";

      shader += "  color += vec4(2.0,2.0,2.0,2.0)*texture2DRect(blurTex, spos.xy + vec2(1.0,0.0));\n";
      shader += "  color += vec4(2.0,2.0,2.0,2.0)*texture2DRect(blurTex, spos.xy - vec2(1.0,0.0));\n";
      shader += "  color += vec4(2.0,2.0,2.0,2.0)*texture2DRect(blurTex, spos.xy + vec2(0.0,1.0));\n";
      shader += "  color += vec4(2.0,2.0,2.0,2.0)*texture2DRect(blurTex, spos.xy - vec2(0.0,1.0));\n";
      shader += "  color += vec4(2.0,2.0,2.0,2.0)*texture2DRect(blurTex, spos.xy + vec2( 1.0, 1.0));\n";
      shader += "  color += vec4(2.0,2.0,2.0,2.0)*texture2DRect(blurTex, spos.xy + vec2(-1.0, 1.0));\n";
      shader += "  color += vec4(2.0,2.0,2.0,2.0)*texture2DRect(blurTex, spos.xy + vec2( 1.0,-1.0));\n";
      shader += "  color += vec4(2.0,2.0,2.0,2.0)*texture2DRect(blurTex, spos.xy + vec2(-1.0,-1.0));\n";

      shader += "  color -= texture2DRect(blurTex, spos.xy + vec2( 2.0, 2.0));\n";
      shader += "  color -= texture2DRect(blurTex, spos.xy + vec2( 2.0, 1.0));\n";
      shader += "  color -= texture2DRect(blurTex, spos.xy + vec2( 2.0, 0.0));\n";
      shader += "  color -= texture2DRect(blurTex, spos.xy + vec2( 2.0,-1.0));\n";
      shader += "  color -= texture2DRect(blurTex, spos.xy + vec2( 2.0,-2.0));\n";
      shader += "  color -= texture2DRect(blurTex, spos.xy + vec2(-2.0, 2.0));\n";
      shader += "  color -= texture2DRect(blurTex, spos.xy + vec2(-2.0, 1.0));\n";
      shader += "  color -= texture2DRect(blurTex, spos.xy + vec2(-2.0, 0.0));\n";
      shader += "  color -= texture2DRect(blurTex, spos.xy + vec2(-2.0,-1.0));\n";
      shader += "  color -= texture2DRect(blurTex, spos.xy + vec2(-2.0,-2.0));\n";
      shader += "  color -= texture2DRect(blurTex, spos.xy + vec2(-1.0, 2.0));\n";
      shader += "  color -= texture2DRect(blurTex, spos.xy + vec2( 0.0, 2.0));\n";
      shader += "  color -= texture2DRect(blurTex, spos.xy + vec2( 1.0, 2.0));\n";
      shader += "  color -= texture2DRect(blurTex, spos.xy + vec2(-1.0,-2.0));\n";
      shader += "  color -= texture2DRect(blurTex, spos.xy + vec2( 0.0,-2.0));\n";
      shader += "  color -= texture2DRect(blurTex, spos.xy + vec2( 1.0,-2.0));\n";

      shader += "  gl_FragColor.rgba = color/8.0;\n";
    }
  else
    shader += "  color = texture2DRect(blurTex, spos.xy);\n";

  shader += "  gl_FragColor = clamp(gl_FragColor, vec4(0.0,0.0,0.0,0.0), vec4(1.0,1.0,1.0,1.0));\n";

  shader += "}\n";

  return shader;
}

QString
ShaderFactory::addLighting()
{
  QString shader;

  shader += " vec3 gx, gy, gz;\n";
  shader += " gx = vec3(1/vsize.x,0,0);\n";
  shader += " gy = vec3(0,1/vsize.y,0);\n";
  shader += " gz = vec3(0,0,1/vsize.z);\n";
  shader += " float vx = texture3D(dataTex, voxelCoord+gx).x - texture3D(dataTex, voxelCoord-gx).x;\n";
  shader += " float vy = texture3D(dataTex, voxelCoord+gy).x - texture3D(dataTex, voxelCoord-gy).x;\n";
  shader += " float vz = texture3D(dataTex, voxelCoord+gz).x - texture3D(dataTex, voxelCoord-gz).x;\n";
  shader += " vec3 grad = vec3(vx, vy, vz);\n";
  shader += " if (length(grad) > 0.1)\n";
  shader += "  {\n";
  shader += "    grad = normalize(grad);\n";
  shader += "    vec3 lightVec = normDir;\n";
  shader += "    float diff = abs(dot(lightVec, grad));\n";
  shader += "    vec3 reflecvec = reflect(lightVec, grad);\n";
  shader += "    float spec = pow(abs(dot(grad, reflecvec)), 512.0);\n";
  shader += "    colorSample.rgb *= (0.6 + 0.4*diff + spec);\n";
  shader += "    if (any(greaterThan(colorSample.rgb,vec3(1.0,1.0,1.0)))) \n";
  shader += "      colorSample.rgb = vec3(1.0,1.0,1.0);\n";
  shader += "  }\n";

  return shader;
}

QString
ShaderFactory::genRaycastShader(int maxSteps, bool firstHit, bool nearest)
{
  QString shader;

  shader =  "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "uniform sampler3D maskTex;\n";
  shader += "uniform sampler1D tagTex;\n";
  shader += "uniform sampler3D dataTex;\n";
  shader += "uniform sampler2D lutTex;\n";
  shader += "uniform sampler2DRect exitTex;\n";
  shader += "uniform float stepSize;\n";
  shader += "uniform vec3 eyepos;\n";
  shader += "uniform vec3 viewDir;\n";
  shader += "uniform vec3 vcorner;\n";
  shader += "uniform vec3 vsize;\n";
  shader += "uniform float minZ;\n";
  shader += "uniform float maxZ;\n";  
  shader += "uniform bool saveCoord;\n";
  shader += "uniform int skipLayers;\n";

  shader += "void main(void)\n";
  shader += "{\n";

  shader += "vec3 exitPoint = texture2DRect(exitTex, gl_FragCoord.st).rgb;\n";
  shader += "vec3 entryPoint = gl_Color.rgb;\n";

  shader += "vec3 dir = (exitPoint-entryPoint);\n";
  shader += "float len = length(dir);\n";
  shader += "if (len < 0.001) discard;\n";

  shader += "vec3 normDir = normalize(dir);\n";
  shader += "vec3 deltaDir = normalize(dir)*stepSize;\n";
  shader += "float deltaDirLen = length(deltaDir);\n";

  shader += "vec3 voxelCoord = entryPoint;\n";
  shader += "vec4 colorAcum = vec4(0.0);\n"; // The dest color
  shader += "float lengthAcum = 0.0;\n";

  // backgroundColor
  shader += "vec4 bgColor = vec4(0.0, 0.0, 0.0, 0.0);\n";
  
  shader += "bool gotFirstHit = false;\n";
  shader += "int nskipped = 0;\n"; 
  shader += "bool solid = false;\n";
//  shader += "bool phaseChanged = false;\n";
  shader += QString("for(int i=0; i<%1; i++)\n").arg(maxSteps);
  shader += "{\n";

  // -- get exact texture coordinate so we don't get tag interpolation --
  shader += "  vec3 vC = voxelCoord*vsize;\n";
  //shader += "  vC = (floor(vC)+vec3(0.5))/vsize;\n";
  shader += "  bvec3 vclt = lessThan(round(vC), vC);\n";
  shader += "  vC += ivec3(vclt)*vec3(0.5);\n";
  shader += "  vC -= ivec3(not(vclt))*vec3(0.5);\n";
  shader += "  vC /= vsize;\n";

  if (nearest)
    shader += "  float val = texture3D(dataTex, vC).x;\n";
  else
    shader += "  float val = texture3D(dataTex, voxelCoord).x;\n";

  shader += "  vec4 colorSample = texture2D(lutTex, vec2(val,0.0));\n";

  shader += "  float tag = texture3D(maskTex, vC).x;\n";
  shader += "  tag += 0.5/256.0;\n";
  shader += "  vec4 tagcolor = texture1D(tagTex, tag);\n";
  shader += "  if (tag < 0.001) tagcolor.rgb = colorSample.rgb;\n";

  shader += "  colorSample.rgb = mix(colorSample.rgb, tagcolor.rgb, 0.5);\n";

  // so that we can use tag opacity to hide certain tagged regions
  // tagcolor.a should either 0 or 1
  shader += "  colorSample *= tagcolor.a;\n";

  shader += "  if (!gotFirstHit && colorSample.a > 0.001) gotFirstHit = true;\n";  

  shader += "  if (gotFirstHit && nskipped > skipLayers)\n";
  shader += "  {\n";

  shader += "    if (saveCoord && colorSample.a > 0.001)";
  shader += "      {\n";
  shader += "        gl_FragColor = vec4(vC,1.0);\n";
  shader += "        return;\n";
  shader += "      }\n";

  if (firstHit)
    {
      shader += "  if (colorSample.a > 0.001 )\n";
      shader += "    {\n";  
      shader += "      vec3 voxpos = vcorner + voxelCoord*vsize;";
      shader += "      vec3 I = voxpos - eyepos;\n";
      shader += "      float z = dot(I, normalize(dir));\n";
      shader += "      z = (z-minZ)/(maxZ-minZ);\n";
      shader += "      z = clamp(z, 0.0, 1.0);\n";
      shader += "      gl_FragColor = vec4(z,tag,1.0-z,1.0);\n";
      shader += "      return;\n";
      shader += "    }\n";
    }

  shader += addLighting();

  shader += "    colorSample.rgb *= colorSample.a;\n";

//  shader += "    if (phaseChanged)\n"; // catch only boundaries
//  shader += "      colorSample *= 1.3;\n";
//  shader += "    else\n";
//  shader += "    {\n";
//  shader += "      colorSample.a *= 0.02;\n";
//  shader += "      colorSample.rgb *= 0.03;\n";
//  shader += "    }\n";

  shader += "    colorAcum += (1.0 - colorAcum.a) * colorSample;\n";

  shader += "  }\n"; // gotfirsthit && nskipped > skipLayers

  shader += "  if (lengthAcum >= len )\n";
  shader += "    {\n";
  shader += "      colorAcum.rgb = colorAcum.rgb*colorAcum.a + (1 - colorAcum.a)*bgColor.rgb;\n";
  shader += "      break;\n";  // terminate if opacity > 1 or the ray is outside the volume	
  shader += "    }\n";
  shader += "  else if (colorAcum.a > 1.0)\n";
  shader += "    {\n";
  shader += "      colorAcum.a = 1.0;\n";
  shader += "      break;\n";
  shader += "    }\n";

  shader += "  voxelCoord += deltaDir;\n";
  shader += "  lengthAcum += deltaDirLen;\n";

  shader += "  if (gotFirstHit) \n";
  shader += "   {\n";
//  shader += "     phaseChanged = false;\n";
  shader += "     if (colorSample.a > 0.001)\n";
  shader += "      {\n";  
  shader += "         if (!solid)\n";
  shader += "           {\n";    
  shader += "             solid = true;\n";
  shader += "             nskipped++;\n";  
//  shader += "             phaseChanged = true;\n";
  shader += "           }\n";  
  shader += "      }\n";
  shader += "     else\n";
  shader += "      {\n";  
  shader += "         if (solid)\n";
  shader += "           {\n";    
//  shader += "             phaseChanged = true;\n";
  shader += "             solid = false;\n";
  shader += "           }\n";  
  shader += "      }\n";
  shader += "   }\n";

  shader += "}\n";

  shader += "gl_FragColor = colorAcum;\n";    

  shader += "}\n";

  return shader;
}

QString
ShaderFactory::genEdgeEnhanceShader()
{
  QString shader;

  shader =  "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "varying vec3 pointpos;\n";
  shader += "uniform sampler1D tagTex;\n";
  shader += "uniform sampler2DRect blurTex;\n";
  shader += "uniform float minZ;\n";
  shader += "uniform float maxZ;\n";
  shader += "uniform vec3 eyepos;\n";
  shader += "uniform vec3 viewDir;\n";
  shader += "uniform float dzScale;\n";
  shader += "void main(void)\n";
  shader += "{\n";
  shader += "  gl_FragColor = vec4(0.0);\n";

  shader += "  vec2 spos = gl_FragCoord.xy;\n";
  shader += "  vec3 dval = texture2DRect(blurTex, spos).xyw;\n";

  //---------------------
  shader += "  float alpha = dval.z;\n";
  shader += "  if (alpha < 0.01) discard;\n";
  //---------------------

  shader += "  float depth = dval.x;\n";
  shader += "  float tag = dval.y;\n";

  //---------------------
  shader += "  vec4 color = vec4(0.0);\n";
  shader += "  color = texture1D(tagTex, tag);\n";
  shader += "  if (tag < 0.001) color.rgb = gl_Color.rgb;\n";
  // so that we can use tag opacity to hide certain tagged regions
  // tagcolor.a should either 0 or 1
  shader += "  if (color.a < 0.001) discard;\n";
  //---------------------

  // find depth gradient
  shader += "  float dx = texture2DRect(blurTex, spos+vec2(1.0,0.0)).x - texture2DRect(blurTex, spos-vec2(1.0,0.0)).x;\n";
  shader += "  float dy = texture2DRect(blurTex, spos+vec2(0.0,1.0)).x - texture2DRect(blurTex, spos-vec2(0.0,1.0)).x;\n";
  shader += "  vec3 norm = normalize(vec3(dx, dy, dzScale/(maxZ-minZ)));\n";
  
  // compute obscurance
  shader += "  float ege1 = 0.01;\n";
  shader += "  float ege2 = 0.03;\n";
  shader += "  float sum = 0.0;\n";
  shader += "  float od = 0.0;\n";

  shader += "  float cx[16] = {-1.5, 1.5, 0.0, 0.0,-2.5,-2.5, 2.5, 2.5,-3.5, 3.5, 0.0, 0.0,-4.5,-4.5, 4.5, 4.5};\n";
  shader += "  float cy[16] = { 0.0, 0.0,-1.5, 1.5,-2.5, 2.5,-2.5, 2.5, 0.0, 0.0,-3.5, 3.5,-4.5, 4.5,-4.5, 4.5};\n";

  shader += "  for(int i=0; i<8; i++)\n";
  shader += "  {\n";
  shader += "    float od = depth - texture2DRect(blurTex, spos+vec2(cx[i],cy[i])).x;\n";
  shader += "    sum += step(ege1, od);\n";
  shader += "  }\n";

  shader += "  for(int i=8; i<16; i++)\n";
  shader += "  {\n";
  shader += "    float od = depth - texture2DRect(blurTex, spos+vec2(cx[i],cy[i])).x;\n";
  shader += "    sum += step(ege2, od);\n";
  shader += "  }\n";

  shader += "  float f = 0.2+0.8*(1.0-sum/16.0);\n";
  shader += "  gl_FragColor = vec4(f*norm.z*color.rgb, 1.0);\n";

  shader += "}\n";

  return shader;
}