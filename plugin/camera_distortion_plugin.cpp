//==============================================================================
/*
    Software License Agreement (BSD License)
    Copyright (c) 2019-2022, AMBF
    (https://github.com/WPI-AIM/ambf)

    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:

    * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above
    copyright notice, this list of conditions and the following
    disclaimer in the documentation and/or other materials provided
    with the distribution.

    * Neither the name of authors nor the names of its contributors may
    be used to endorse or promote products derived from this software
    without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
    FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
    COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
    BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
    LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
    ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
    POSSIBILITY OF SUCH DAMAGE.

    \author    <amunawar@wpi.edu>
    \author    Adnan Munawar

    \author    <hishida3@jhu.edu>
    \author    Hisashi Ishida
*/
//==============================================================================

#include "camera_distortion_plugin.h"

using namespace std;


//------------------------------------------------------------------------------
// DECLARED FUNCTIONS
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------


afCameraDistortionPlugin::afCameraDistortionPlugin()
{   
    cout << "/*********************************************" << endl;
    cout << "/* AMBF Camera Distortion Plugin" << endl;
    cout << "/*********************************************" << endl;

    // For HTC Vive Pro
    m_width = 1920;
    m_height = 1080;
    m_alias_scaling = 1.0;
}

int afCameraDistortionPlugin::init(const afBaseObjectPtr a_afObjectPtr, const afBaseObjectAttribsPtr a_objectAttribs)
{
    m_camera = (afCameraPtr)a_afObjectPtr;
    YAML::Node specificationDataNode = YAML::Load(a_objectAttribs->getSpecificationData().m_rawData);
    CameraParams params;

    // If there is a configuration file given in the ADF file
    if (specificationDataNode["plugins"][0]["distortion_config"]){
        string m_configPath = specificationDataNode["plugins"][0]["distortion_config"].as<string>();
        cerr << "[INFO!] Reading configuration file: " << m_configPath << endl;
        readCameraParams(m_configPath, params);
    }
    
    // If there is no configuration file given
    else{
        cerr << "WARNING! NO configuration file specified." << endl;
        return -1;
    }

    m_camera->setOverrideRendering(true);

    m_camera->getInternalCamera()->m_stereoOffsetW = 0.0;

    m_frameBuffer = cFrameBuffer::create();
    m_frameBuffer->setup(m_camera->getInternalCamera(), m_width * m_alias_scaling, m_height * m_alias_scaling, true, true, GL_RGBA);

    string file_path = __FILE__;
    m_current_filepath = file_path.substr(0, file_path.rfind("/"));

    afShaderAttributes shaderAttribs;
    shaderAttribs.m_shaderDefined = true;
    shaderAttribs.m_vtxFilepath = specificationDataNode["plugins"][0]["vertex_shader"].as<string>();
    shaderAttribs.m_fragFilepath = specificationDataNode["plugins"][0]["fragment_shader"].as<string>();

    m_shaderPgm = afShaderUtils::createFromAttribs(&shaderAttribs, "TEST", "VR_CAM");
    if (!m_shaderPgm){
        cerr << "ERROR! FAILED TO LOAD SHADER PGM \n";
        return -1;
    }

    m_viewport_scale[0] = 1.0;
    m_viewport_scale[1] = 1.0;

    if (params.distortion_coefs.empty()) {
        for (int i = 0; i < 4; i++) {
            m_distortion_coeffs[i] = params.distortion_coefs[i];
        }
    } else {
        m_distortion_coeffs[0] = 0.0;
        m_distortion_coeffs[1] = 0.0;
        m_distortion_coeffs[2] = 0.0;
        m_distortion_coeffs[3] = 0.0;
    }

    m_aberr_scale[0] = 1.0;
    m_aberr_scale[1] = 1.0;
    m_aberr_scale[2] = 1.0;

    m_lens_center[0] = 0.0;
    m_lens_center[1] = 0.0;     

    m_warp_scale = 1.0;
    m_warp_adj = 1.0;

    m_quadMesh = new cMesh();
    float quad[] = {
        // positions
         -1.0f,  1.0f, 0.0f,
         -1.0f, -1.0f, 0.0f,
          1.0f, -1.0f, 0.0f,
         -1.0f,  1.0f, 0.0f,
          1.0f, -1.0f, 0.0f,
          1.0f,  1.0f, 0.0f,
    };

    for (int vI = 0 ; vI < 2 ; vI++){
        int off = vI * 9;
        cVector3d v0(quad[off + 0], quad[off + 1], quad[off + 2]);
        cVector3d v1(quad[off + 3], quad[off + 4], quad[off + 5]);
        cVector3d v2(quad[off + 6], quad[off + 7], quad[off + 8]);
        m_quadMesh->newTriangle(v0, v1, v2);
    }
    m_quadMesh->m_vertices->setTexCoord(1, 0.0, 0.0, 1.0);
    m_quadMesh->m_vertices->setTexCoord(2, 1.0, 0.0, 1.0);
    m_quadMesh->m_vertices->setTexCoord(0, 0.0, 1.0, 1.0);
    m_quadMesh->m_vertices->setTexCoord(3, 0.0, 1.0, 1.0);
    m_quadMesh->m_vertices->setTexCoord(4, 1.0, 0.0, 1.0);
    m_quadMesh->m_vertices->setTexCoord(5, 1.0, 1.0, 1.0);

    m_quadMesh->computeAllNormals();
    m_quadMesh->m_texture = m_frameBuffer->m_imageBuffer;
    m_quadMesh->setUseTexture(true);

    m_quadMesh->setShaderProgram(m_shaderPgm);
    m_quadMesh->setShowEnabled(true);

    m_distortedWorld = new cWorld();
    m_distortedWorld->addChild(m_quadMesh);

    cerr << m_distortion_coeffs[0] << "," << m_distortion_coeffs[1] << "," << m_distortion_coeffs[2] << "," << m_distortion_coeffs[3] << endl;

    return 1;
}

void afCameraDistortionPlugin::graphicsUpdate()
{
    glfwMakeContextCurrent(m_camera->m_window);
    m_frameBuffer->renderView();
    updateCameraParams();

    afRenderOptions ro;
    ro.m_updateLabels = true;

    // Temporarily switch camera to VR world
    cWorld* cachedWorld = m_camera->getInternalCamera()->getParentWorld();
    m_camera->getInternalCamera()->setStereoMode(C_STEREO_DISABLED);
    m_camera->getInternalCamera()->setParentWorld(m_distortedWorld);

    // Render only camera feed distortion
    static cWorld* emptyWorld = new cWorld();
    cWorld* frontLayer = m_camera->getInternalCamera()->m_frontLayer;
    m_camera->getInternalCamera()->m_frontLayer = emptyWorld;
    m_camera->render(ro);
    m_camera->getInternalCamera()->m_frontLayer = frontLayer;

    m_camera->getInternalCamera()->setStereoMode(C_STEREO_PASSIVE_LEFT_RIGHT);
    m_camera->getInternalCamera()->setParentWorld(cachedWorld);
}

void afCameraDistortionPlugin::physicsUpdate(double dt)
{

}

void afCameraDistortionPlugin::reset(){

}

bool afCameraDistortionPlugin::close()
{
    return true;
}

void afCameraDistortionPlugin::updateCameraParams()
{
    GLint id = m_shaderPgm->getId();
    //    cerr << "INFO! Shader ID " << id << endl; // Shader ID is always 1 in my case
    glUseProgram(id);
    glUniform1i(glGetUniformLocation(id, "warpTexture"), 2);
    glUniform2fv(glGetUniformLocation(id, "ViewportScale"), 1, m_viewport_scale);
    glUniform3fv(glGetUniformLocation(id, "aberr"), 1, m_aberr_scale);
    glUniform1f(glGetUniformLocation(id, "WarpScale"), m_warp_scale*m_warp_adj);
    glUniform2fv(glGetUniformLocation(id, "LensCenter"), 1, m_lens_center);
    glUniform4fv(glGetUniformLocation(id, "HmdWarpParam"), 1, m_distortion_coeffs);
}

void afCameraDistortionPlugin::makeFullScreen()
{
    const GLFWvidmode* mode = glfwGetVideoMode(m_camera->m_monitor);
    int w = 2880;
    int h = 1600;
    int x = mode->width - w;
    int y = mode->height - h;
    int xpos, ypos;
    glfwGetMonitorPos(m_camera->m_monitor, &xpos, &ypos);
    x += xpos; y += ypos;
    glfwSetWindowPos(m_camera->m_window, x, y);
    glfwSetWindowSize(m_camera->m_window, w, h);
    m_camera->m_width = w;
    m_camera->m_height = h;
    glfwSwapInterval(0);
    cerr << "\t Making " << m_camera->getName() << " fullscreen \n" ;
}

// Function to read YAML file and extract camera parameters
int afCameraDistortionPlugin::readCameraParams(const string &filename, CameraParams &params) {
    try {
        YAML::Node config = YAML::LoadFile(filename);

        // Read camera type
        if (!config["type"] || !config["type"].IsScalar()) {
            cerr << "Error: Missing or invalid 'type' field." << endl;
            return 0;
        }
        params.camera_type = config["type"].as<string>();
        cout << params.camera_type << endl;

        // Read intrinsic parameters
        if (!config["intrinsic"] || !config["intrinsic"].IsMap()) {
            cerr << "Error: Missing or invalid 'intrinsic' field." << endl;
            return 0;
        }

        try {
            params.fx = config["intrinsic"]["fx"].as<double>();
            params.fy = config["intrinsic"]["fy"].as<double>();
            params.cx = config["intrinsic"]["cx"].as<double>();
            params.cy = config["intrinsic"]["cy"].as<double>();
        } catch (const YAML::Exception &e) {
            cerr << "Error reading intrinsic parameters: " << e.what() << endl;
            return 0;
        }

        // Read distortion coefficients
        if (!config["distortion_coeffs"] || !config["distortion_coeffs"].IsSequence()) {
            cerr << "Error: Missing or invalid 'distortion_coeffs' field." << endl;
            return 0;
        }

        params.distortion_coefs.clear();
        for (const auto &val : config["distortion_coeffs"]) {
            params.distortion_coefs.push_back(val.as<double>());
        }

        // Validate coefficient count based on type
        if ((params.camera_type == "fisheye" && params.distortion_coefs.size() != 4) ||
            (params.camera_type == "pinhole" && params.distortion_coefs.size() != 5)) {
            cerr << "Error: Incorrect number of distortion coefficients for " << params.camera_type << " camera." << endl;
            return 0;
        }

        return 1;
    } catch (const YAML::Exception &e) {
        cerr << "YAML parse error: " << e.what() << endl;
        return 0;
    }
}

