/*
 *      Copyright (C) 2008-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

// Waveform.vis
// A simple visualisation example by MrC

#include <kodi/addon-instance/Visualization.h>
#include <stdio.h>
#ifdef HAS_OPENGL
#define GL_GLEXT_PROTOTYPES
#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glext.h>
#endif
#endif

typedef struct {
  int TopLeftX;
  int TopLeftY;
  int Width;
  int Height;
  int MinDepth;
  int MaxDepth;
} D3D11_VIEWPORT;
typedef unsigned long D3DCOLOR;

struct Vertex_t
{
  float x, y, z;
  D3DCOLOR  col;
};

class CVisualizationWaveForm
  : public kodi::addon::CAddonBase,
    public kodi::addon::CInstanceVisualization
{
public:
  CVisualizationWaveForm();
  virtual ~CVisualizationWaveForm();

  virtual ADDON_STATUS Create() override;
  virtual void Render() override;
  virtual void AudioData(const float* audioData, int audioDataLength, float *freqData, int freqDataLength) override;

private:
  void* m_device;
  float m_fWaveform[2][512];
  D3D11_VIEWPORT m_viewport;
  GLuint vaID = 0;
  typedef struct {
    float x;
    float y;
  } v2t;

  v2t verts1[256];
  v2t verts2[256];
  GLuint vshader;
  GLuint fshader;
  GLuint program;
};

CVisualizationWaveForm::CVisualizationWaveForm()
{
  m_device = nullptr;
}

//-- Destroy ------------------------------------------------------------------
// Do everything before unload of this add-on
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
CVisualizationWaveForm::~CVisualizationWaveForm()
{
  glDeleteProgram(program);
  glDetachShader(program, vshader);
  glDetachShader(program, fshader);
  glDeleteShader(vshader);
  glDeleteShader(fshader);
  glDeleteBuffers(1, &vaID);
}

const char* vertShader = R"glsl(
#version 110
attribute vec2 position;
void main()
{
  gl_Position = vec4(position, 1.0, 1.0);
}
)glsl";

const char* fragShader = R"glsl(
#version 110
void main()
{
  gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);
}
)glsl";

//-- Create -------------------------------------------------------------------
// Called on load. Addon should fully initalize or return error status
//-----------------------------------------------------------------------------
ADDON_STATUS CVisualizationWaveForm::Create()
{
  m_device = Device();
  m_viewport.TopLeftX = X();
  m_viewport.TopLeftY = Y();
  m_viewport.Width = Width();
  m_viewport.Height = Height();
  m_viewport.MinDepth = 0;
  m_viewport.MaxDepth = 1;

  glGenBuffers(1, &vaID);
  GLenum errcode;
  if ((errcode=glGetError())!=GL_NO_ERROR) {
    printf("Houston, we have a GL problem: %s\n", gluErrorString(errcode));
  }
  vshader = glCreateShader(GL_VERTEX_SHADER);
  if ((errcode=glGetError())!=GL_NO_ERROR) {
    printf("Houston, we have a GL problem: %s\n", gluErrorString(errcode));
  }
  glShaderSource(vshader, 1, &vertShader, nullptr);
  if ((errcode=glGetError())!=GL_NO_ERROR) {
    printf("Houston, we have a GL problem: %s\n", gluErrorString(errcode));
  }
  glCompileShader(vshader);
  if ((errcode=glGetError())!=GL_NO_ERROR) {
    printf("Houston, we have a GL problem: %s\n", gluErrorString(errcode));
  }
  GLint isCompiled=0;
  glGetShaderiv(vshader, GL_COMPILE_STATUS, &isCompiled);
  if (isCompiled == GL_FALSE)
  {
    printf("Error compiling vertex shader\n");
    GLint maxLength = 0;
    glGetShaderiv(vshader, GL_INFO_LOG_LENGTH, &maxLength);
    std::string error;
    error.resize(maxLength);
    glGetShaderInfoLog(vshader, maxLength, &maxLength, &error[0]);
    printf("  %s\n", error.c_str());
  }
  fshader = glCreateShader(GL_FRAGMENT_SHADER);
  if ((errcode=glGetError())!=GL_NO_ERROR) {
    printf("Houston, we have a GL problem: %s\n", gluErrorString(errcode));
  }
  glShaderSource(fshader, 1, &fragShader, nullptr);
  if ((errcode=glGetError())!=GL_NO_ERROR) {
    printf("Houston, we have a GL problem: %s\n", gluErrorString(errcode));
  }
  glCompileShader(fshader);
  glGetShaderiv(fshader, GL_COMPILE_STATUS, &isCompiled);
  if (isCompiled == GL_FALSE)
    printf("Error compiling frag shader\n");
  if ((errcode=glGetError())!=GL_NO_ERROR) {
    printf("Houston, we have a GL problem: %s\n", gluErrorString(errcode));
  }
  program = glCreateProgram();
  if ((errcode=glGetError())!=GL_NO_ERROR) {
    printf("Houston, we have a GL problem: %s\n", gluErrorString(errcode));
  }
  glAttachShader(program, vshader);
  if ((errcode=glGetError())!=GL_NO_ERROR) {
    printf("Houston, we have a GL problem: %s\n", gluErrorString(errcode));
  }
  glAttachShader(program, fshader);
  if ((errcode=glGetError())!=GL_NO_ERROR) {
    printf("Houston, we have a GL problem: %s\n", gluErrorString(errcode));
  }
  glLinkProgram(program);
  if ((errcode=glGetError())!=GL_NO_ERROR) {
    printf("Houston, we have a GL problem: %s\n", gluErrorString(errcode));
  }

  return ADDON_STATUS_OK;
}

//-- Audiodata ----------------------------------------------------------------
// Called by XBMC to pass new audio data to the vis
//-----------------------------------------------------------------------------
void CVisualizationWaveForm::AudioData(const float* pAudioData, int iAudioDataLength, float *pFreqData, int iFreqDataLength)
{
  int ipos=0;
  while (ipos < 512)
  {
    for (int i=0; i < iAudioDataLength; i+=2)
    {
      m_fWaveform[0][ipos] = pAudioData[i  ]; // left channel
      m_fWaveform[1][ipos] = pAudioData[i+1]; // right channel
      ipos++;
      if (ipos >= 512) break;
    }
  }
}

//-- Render -------------------------------------------------------------------
// Called once per frame. Do all rendering here.
//-----------------------------------------------------------------------------
void CVisualizationWaveForm::Render()
{
  // Left channel
  GLenum errcode;
  glDisable(GL_BLEND);
  glPushMatrix();
  glTranslatef(0,0,-1.0);

  for (int i = 0; i < 256; i++)
  {
    verts1[i].x = m_viewport.TopLeftX + ((i / 255.0f) * m_viewport.Width);
    verts1[i].y = m_viewport.TopLeftY + m_viewport.Height * 0.33f + (m_fWaveform[0][i] * m_viewport.Height * 0.15f);
  }
  for (int i = 0; i < 256; i++)
  {
    verts2[i].x = m_viewport.TopLeftX + ((i / 255.0f) * m_viewport.Width);
    verts2[i].y = m_viewport.TopLeftY + m_viewport.Height * 0.66f + (m_fWaveform[1][i] * m_viewport.Height * 0.15f);
  }

  glBindBuffer(GL_ARRAY_BUFFER, vaID);
  glBufferData(GL_ARRAY_BUFFER, 256*2*sizeof(float), verts1, GL_STATIC_DRAW);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2*sizeof(GLfloat), 0);
  glUseProgram(program);

  glDrawArrays(GL_LINE_STRIP, 0, 256);

  glBufferData(GL_ARRAY_BUFFER, 256*2*sizeof(float), verts2, GL_STATIC_DRAW);
  glDrawArrays(GL_LINE_STRIP, 0, 256);

  glEnable(GL_BLEND);
  glPopMatrix();
}

ADDONCREATOR(CVisualizationWaveForm)
