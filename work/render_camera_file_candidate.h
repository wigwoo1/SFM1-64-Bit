#pragma once
#include "render_camera_candidate.h"
#include <fstream>
inline RenderCameraCandidate readRenderCameraFileCandidate(const std::string&path){std::ifstream input(path,std::ios::binary);if(!input)throw std::runtime_error("camera matrix file missing");RenderCameraCandidate result;input.read(reinterpret_cast<char*>(&result),sizeof(result));if(input.gcount()!=sizeof(result)||input.peek()!=std::char_traits<char>::eof())throw std::runtime_error("camera matrix file size");for(float value:result.rows)if(!std::isfinite(value))throw std::runtime_error("camera matrix nonfinite");return result;}
// Perspective W is forward distance; orthographic Z is linear depth. Unlike
// divided depth, these keys remain finite for surfaces crossing the eye plane.
inline double renderCameraSortDistanceCandidate(const RenderCameraCandidate&camera,const std::array<float,3>&position){auto clip=camera.clip(position);bool perspective=camera.rows[12]!=0||camera.rows[13]!=0||camera.rows[14]!=0;return perspective?clip[3]:clip[2];}
