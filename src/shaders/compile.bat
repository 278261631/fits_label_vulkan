@echo off
set VULKAN_SDK=D:\VulkanSDK\1.4.335.0
set GLSLC=%VULKAN_SDK%\Bin\glslc.exe

if not exist shaders mkdir shaders

echo Compiling grid vertex shader...
%GLSLC% -fshader-stage=vertex -o shaders/grid.vert.spv grid.vert

echo Compiling grid fragment shader...
%GLSLC% -fshader-stage=fragment -o shaders/grid.frag.spv grid.frag

echo Compiling coord vertex shader...
%GLSLC% -fshader-stage=vertex -o shaders/coord.vert.spv coord.vert

echo Compiling coord fragment shader...
%GLSLC% -fshader-stage=fragment -o shaders/coord.frag.spv coord.frag

echo Compiling demo vertex shader...
%GLSLC% -fshader-stage=vertex -o shaders/demo.vert.spv demo.vert

echo Compiling demo fragment shader...
%GLSLC% -fshader-stage=fragment -o shaders/demo.frag.spv demo.frag

echo Done!

