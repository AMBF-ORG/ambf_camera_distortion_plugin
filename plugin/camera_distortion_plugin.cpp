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

afCameraDistortionPlugin::afCameraDistortionPlugin()
{   
    cout << "/*********************************************" << endl;
    cout << "/* AMBF Camera Distortion Plugin" << endl;
    cout << "/*********************************************" << endl;
}

int afCameraDistortionPlugin::init(const afBaseObjectPtr a_afObjectPtr, const afBaseObjectAttribsPtr a_objectAttribs)
{
    m_camera = (afCameraPtr)a_afObjectPtr;
    // m_camera->m_width = 640;
    // m_camera->m_height = 640;
    // cerr << "Camera image: [" << m_camera->m_width << "x" << m_camera->m_height  << "]" << endl;
    
    // // TODO: Use the params defined m_camera and m_width and m_height
    // // Think about the way to dynamically change framebuffer size
    // m_width = m_camera->m_width;
    // m_height = m_camera->m_height;

    YAML::Node specificationDataNode = YAML::Load(a_objectAttribs->getSpecificationData().m_rawData);

    // If there is a configuration file given in the ADF file
    if (specificationDataNode["plugins"][0]["distortion_config"]){
        string m_configPath = specificationDataNode["plugins"][0]["distortion_config"].as<string>();
        cerr << "[INFO!] Reading configuration file: " << m_configPath << endl;
        readCameraParams(m_configPath, m_cameraParams);
    }
    
    // If there is no configuration file given
    else{
        cerr << "WARNING! NO configuration file specified." << endl;
        return -1;
    }

    
    // TODO: Use the params defined m_camera and m_width and m_height
    // Think about the way to dynamically change framebuffer size
    changeScreenSize(500, 500);
    cerr << "Camera image: [" << m_camera->m_width << "x" << m_camera->m_height  << "]" << endl;


    m_camera->setOverrideRendering(true);
    m_frameBuffer = cFrameBuffer::create();

    // Initialize framebuffer (framebuffer store color/depth information)
    m_frameBuffer->setup(m_camera->getInternalCamera(), m_width, m_height, true, true, GL_RGBA);

    // Specify shader files
    afShaderAttributes shaderAttribs;
    shaderAttribs.m_shaderDefined = true;
    shaderAttribs.m_vtxFilepath = specificationDataNode["plugins"][0]["vertex_shader"].as<string>();
    shaderAttribs.m_fragFilepath = specificationDataNode["plugins"][0]["fragment_shader"].as<string>();

    m_shaderPgm = afShaderUtils::createFromAttribs(&shaderAttribs, "CameraDistortion", "CameraDistortion");
    if (!m_shaderPgm){
        cerr << "ERROR! FAILED TO LOAD SHADER PGM \n";
        return -1;
    }

    // Set two sets of trinangles
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

    // Create triangles to set Texture coordinate
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

    updateCameraParams();

    makeFullScreen();

    return 1;
}

void afCameraDistortionPlugin::graphicsUpdate()
{
    glfwMakeContextCurrent(m_camera->m_window);
    m_frameBuffer->renderView();

    afRenderOptions ro;
    ro.m_updateLabels = true;

    // Temporarily switch camera to Distorted world
    cWorld* cachedWorld = m_camera->getInternalCamera()->getParentWorld();
    m_camera->getInternalCamera()->setStereoMode(C_STEREO_DISABLED);
    m_camera->getInternalCamera()->setParentWorld(m_distortedWorld);

    // Render only camera feed distortion
    static cWorld* emptyWorld = new cWorld();
    cWorld* frontLayer = m_camera->getInternalCamera()->m_frontLayer;
    m_camera->getInternalCamera()->m_frontLayer = emptyWorld;
    m_camera->render(ro);
    m_camera->getInternalCamera()->m_frontLayer = frontLayer;

    m_camera->getInternalCamera()->setStereoMode(C_STEREO_DISABLED);
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
    glUniform1i(glGetUniformLocation(id, "WarpTexture"), 2);
    glUniform1i(glGetUniformLocation(id, "DistortionType"), static_cast<int>(m_cameraParams.distortion_type));
    glUniform3fv(glGetUniformLocation(id, "ChromaticAberr"), 1, m_cameraParams.aberr_scale);
    glUniform2fv(glGetUniformLocation(id, "LensCenter"), 1, m_cameraParams.lens_center);
    glUniform2fv(glGetUniformLocation(id, "Image"), 1, m_cameraParams.width); //TODOx
    glUniform4fv(glGetUniformLocation(id, "RadialDistortion"), 1, m_cameraParams.radial_distortion_coeffs);
    glUniform2fv(glGetUniformLocation(id, "TangentialDistortion"), 1, m_cameraParams.tangential_distortion_coeffs);
}

void afCameraDistortionPlugin::makeFullScreen()
{
    const GLFWvidmode* mode = glfwGetVideoMode(m_camera->m_monitor);
    int w = 1920;
    int h = 1080;
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

void afCameraDistortionPlugin::changeScreenSize(int w, int h)
{
    const GLFWvidmode* mode = glfwGetVideoMode(m_camera->m_monitor);
    int x = mode->width - w;
    int y = mode->height - h;
    int xpos, ypos;
    glfwGetMonitorPos(m_camera->m_monitor, &xpos, &ypos);
    x += xpos; y += ypos;
    glfwSetWindowPos(m_camera->m_window, x, y);
    glfwSetWindowSize(m_camera->m_window, w, h);
    m_camera->m_width = w;
    m_camera->m_height = h;
    m_width = m_camera->m_width;
    m_height = m_camera->m_height;
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
        if (config["type"].as<string>() == "pinhole"){
            params.distortion_type = DistortionType::PINHOLE;
            cout << "distortion_type: PINHOLE " << static_cast<int>(DistortionType::PINHOLE)<< endl;
        }
        else if (config["type"].as<string>() == "fisheye"){
            params.distortion_type = DistortionType::FISHEYE;
            cout << "distortion_type: FISHEYE " << static_cast<int>(DistortionType::FISHEYE)<< endl;
        }
        else if (config["type"].as<string>() == "panotool"){
            params.distortion_type = DistortionType::PANOTOOL;
            cout << "distortion_type: PANOTOOL " << static_cast<int>(DistortionType::PANOTOOL)<< endl;
        }

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


            if (config["image_size"]){
                params.width = config["image_size"].as<vector<double>>()[0];
                params.height = config["image_size"].as<vector<double>>()[1];
                params.lens_center[0] = params.cx/params.width;
                params.lens_center[1] = params.cy/params.height;

                cout << "image_size: [" << params.width << "," << params.height << ']' << endl;
                cout << "len center: [" << params.lens_center[0]  << "," << params.lens_center[1] << ']' << endl;
            }  

            else{
                params.lens_center[0] = 0.5;
                params.lens_center[1] = 0.5;
            }


        } catch (const YAML::Exception &e) {
            cerr << "Error reading intrinsic parameters: " << e.what() << endl;
            return 0;
        }

        // Read radial distortion coefficients
        if (!config["radial_distortion_coeffs"] || !config["radial_distortion_coeffs"].IsSequence()) {
            cerr << "[CAUTION!] Missing or invalid 'radial distortion_coeffs' field." << endl;
            for (size_t i = 0 ; i < 4; i++){
                params.radial_distortion_coeffs[i] = 0.0;
            }
        }

        else{
            for (size_t i = 0 ; i < 4; i++) {
                params.radial_distortion_coeffs[i] = config["radial_distortion_coeffs"].as<vector<float>>()[i];
            }
        }

        // Read radial distortion coefficients
        if (!config["tangential_distortion_coeffs"] || !config["tangential_distortion_coeffs"].IsSequence()) {
            cerr << "[CAUTION!] Missing or invalid 'tangential distortion_coeffs' field." << endl;
            for (size_t i = 0 ; i < 2; i++){
                params.tangential_distortion_coeffs[i] = 0.0;
            }
        }

        else{
            for (size_t i = 0 ; i < 2; i++) {
                params.tangential_distortion_coeffs[i] = config["tangential_distortion_coeffs"].as<vector<float>>()[i];
            }
        }

        cerr << "Distortion Coefficient:" << endl;
        cerr << "Radial: " << 
                m_cameraParams.radial_distortion_coeffs[0] << "," << 
                m_cameraParams.radial_distortion_coeffs[1] << "," << 
                m_cameraParams.radial_distortion_coeffs[2] << "," << 
                m_cameraParams.radial_distortion_coeffs[3] << endl;

        cerr << "Tangential: " << 
                m_cameraParams.tangential_distortion_coeffs[0] << "," << 
                m_cameraParams.tangential_distortion_coeffs[1] << endl;


        // Read chromatic distortion coefficient
        if (!config["chromatic_distortion"] || !config["chromatic_distortion"].IsSequence()) {
            cerr << "[CAUTION!] Missing or invalid 'chromatic_distortion' field." << endl;
            for (size_t i = 0 ; i < 3; i++){
                params.aberr_scale[i] = 1.0;
            }
        }
    
        else{
            for (size_t i = 0 ; i < 3; i++) {
                params.aberr_scale[i] = config["chromatic_distortion"].as<vector<float>>()[i];
            }
        }

        cerr << "Chromatic distortion Coefficient:" <<
                m_cameraParams.aberr_scale[0] << "," << 
                m_cameraParams.aberr_scale[1] << "," << 
                m_cameraParams.aberr_scale[2] << endl;

        // Validate coefficient count based on type
        // if ((params.camera_type == "fisheye" && params.distortion_coefs.size() != 4) ||
        //     (params.camera_type == "pinhole" && params.distortion_coefs.size() != 5)) {
        //     cerr << "Error: Incorrect number of distortion coefficients for " << params.camera_type << " camera." << endl;
        //     return 0;
        // }

        return 1;
    } catch (const YAML::Exception &e) {
        cerr << "YAML parse error: " << e.what() << endl;
        return 0;
    }
}

