// Playing Around - jlb
// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include <librealsense/rs.hpp>
#include "example.hpp"

#include <sstream>
#include <iostream>
#include <iomanip>
#include <thread>
#include <algorithm>
#include <libusb.h>
#include <unistd.h>
#include ".././src/uvc.h"
#include ".././src/context.h"
#include ".././src/device.h"
#include ".././src/libuvc/libuvc.h"

#define VENDOR_ID   0x8086
#define PRODUCT_ID  0x0a80


texture_buffer buffers[RS_STREAM_COUNT];
bool align_depth_to_color = false;
bool align_color_to_depth = false;
bool color_rectification_enabled = false;

#include <memory>

void clearInterfaceHalt ( libusb_device_handle *dev_handle, int bInterfaceNumber, int bEndpointAddress) {
    int result ;
    if (libusb_kernel_driver_active(dev_handle,bInterfaceNumber)) {
        libusb_detach_kernel_driver(dev_handle,bInterfaceNumber) ;
    }

    result = libusb_claim_interface(dev_handle,bInterfaceNumber) ;
    if (result != 0) {
        printf("Could not claim interface bInterfaceNumber %d! %d %s\n",bInterfaceNumber,result,libusb_error_name(result));
    }
    result = libusb_clear_halt(dev_handle,bEndpointAddress) ;
    if (result != 0) {
        printf("Could not clear halt on interface bInterfaceNumber: %d! %d %s\n",bInterfaceNumber,result,libusb_error_name(result));
    }
    result = libusb_release_interface(dev_handle,bInterfaceNumber) ;
    if (result != 0) {
        printf("Could not release interface bInterfaceNumber: %d! %d %s\n",bInterfaceNumber,result,libusb_error_name(result));
    }
    libusb_attach_kernel_driver(dev_handle,bInterfaceNumber) ;
}

void resetDevice ( void ) {
    libusb_context          *context = NULL ;
    libusb_device_handle    *dev_handle = NULL ;
    libusb_device           **devs ;
    int                     rc ;
    ssize_t                 count ; // Number of devices in list
    int ret ;

    rc = libusb_init(&context);
    // TODO Need to check to make sure libusb (rc) was initialized
    count = libusb_get_device_list(context, &devs);
    dev_handle = libusb_open_device_with_vid_pid(context,VENDOR_ID,PRODUCT_ID);
    libusb_free_device_list(devs, 1); //free the list, unref the devices in it
    if (dev_handle) {

        int bpoint = 0 ;
        int success = 0 ;
        do {
            success = libusb_reset_device(dev_handle) ;
            ret = success ;
            ++ bpoint ;
            if (bpoint > 100) {
                success = 1 ;
            }

        } while (success < 0 ) ;
        if (success) {
            printf("Unable to reset camera! %d %s\n", ret,libusb_error_name(ret));
        } else {
            printf("Camera Reset!\n") ;
        }
        libusb_close(dev_handle) ;
    }
    libusb_exit(context);
}

void clearHalts ( void ) {
    libusb_context          *context = NULL ;
    libusb_device_handle    *dev_handle = NULL ;
    libusb_device           **devs ;
    int                     rc ;
    ssize_t                 count ; // Number of devices in list
    int ret ;

    rc = libusb_init(&context);
    // TODO Need to check to make sure libusb (rc) was initialized
    count = libusb_get_device_list(context, &devs);
    dev_handle = libusb_open_device_with_vid_pid(context,VENDOR_ID,PRODUCT_ID);
    libusb_free_device_list(devs, 1); //free the list, unref the devices in it
    if (dev_handle) {
        // Clear halts that might be on the video stream
        // Infrared cameras
        // clearInterfaceHalt(dev_handle,1,0x86) ;
        // clearInterfaceHalt(dev_handle,2,0x82) ;
        // Depth camera stream
        clearInterfaceHalt(dev_handle,3,0x85) ;

        // clearInterfaceHalt(dev_handle,4,0x81) ;
        // Interface Number 5 is the color camera
        clearInterfaceHalt(dev_handle,5,0x87) ;
        libusb_close(dev_handle) ;
     }

    libusb_exit(context);
}


int main(int argc, char * argv[]) try
{
    // rs::log_to_console(rs::log_severity::warn);
    //rs::log_to_file(rs::log_severity::debug, "librealsense.log");
    if( access( "/tmp/reset-realsense", F_OK ) != -1 ) {
        // file exists
        unlink("/tmp/reset-realsense") ;
        resetDevice();
    } else {
        // file doesn't exist ; means we've already run the program once since reboot
        // Clear interface halts that may have accumulated on video streams
        clearHalts();
    }


    rs::context ctx;
    if(ctx.get_device_count() == 0) throw std::runtime_error("No device detected. Is it plugged in?");
    rs::device & dev = *ctx.get_device(0);


   //     dev.enable_stream(rs::stream::depth, rs::preset::best_quality);
   //     dev.enable_stream(rs::stream::color, rs::preset::best_quality);
   //     dev.enable_stream(rs::stream::infrared, rs::preset::best_quality);

    dev.enable_stream(rs::stream::depth, rs::preset::largest_image);
    dev.enable_stream(rs::stream::color, rs::preset::best_quality);
    // dev.enable_stream(rs::stream::infrared, rs::preset::largest_image);
    // try { dev.enable_stream(rs::stream::infrared2, 0, 0, rs::format::any, 0); } catch(...) {}
    bool aee = dev.get_option(rs::option::r200_lr_auto_exposure_enabled);


    printf("Startup: autoexposure: %d\n",aee) ;
    bool ee =  dev.get_option(rs::option::r200_emitter_enabled);
    printf("Startup: Emitter: %d\n", ee) ;
    dev.set_option(rs::option::r200_lr_auto_exposure_enabled, true);


    // Compute field of view for each enabled stream
    for(int i = 0; i < 4; ++i)
    {
        auto stream = rs::stream(i);
        if(!dev.is_stream_enabled(stream)) continue;
        auto intrin = dev.get_stream_intrinsics(stream);
        std::cout << "Capturing " << stream << " at " << intrin.width << " x " << intrin.height;
        std::cout << std::setprecision(1) << std::fixed << ", fov = " << intrin.hfov() << " x " << intrin.vfov() << ", distortion = " << intrin.model() << std::endl;
    }
    
    // Start our device
    dev.start();

    // Capture 30 frames to give autoexposure, etc. a chance to settle
    for (int i = 0; i < 30; ++i) dev.wait_for_frames();


    // Open a GLFW window
    glfwInit();
    std::ostringstream ss; ss << "A Test Example (" << dev.get_name() << ")";
    GLFWwindow * win = glfwCreateWindow(1280, 960, ss.str().c_str(), 0, 0);
    glfwSetWindowUserPointer(win, &dev);
    glfwSetKeyCallback(win, [](GLFWwindow * win, int key, int scancode, int action, int mods) 
    { 
        auto dev = reinterpret_cast<rs::device *>(glfwGetWindowUserPointer(win));
        if(action != GLFW_RELEASE) switch(key)
        {
        case GLFW_KEY_R: color_rectification_enabled = !color_rectification_enabled; break;
        case GLFW_KEY_C: align_color_to_depth = !align_color_to_depth; break;
        case GLFW_KEY_D: align_depth_to_color = !align_depth_to_color; break;
        case GLFW_KEY_E:
            if(dev->supports_option(rs::option::r200_emitter_enabled))
            {
                int value = !dev->get_option(rs::option::r200_emitter_enabled);
                std::cout << "Setting emitter to " << value << std::endl;
                dev->set_option(rs::option::r200_emitter_enabled, value);
            }
            break;
        case GLFW_KEY_A:
            if(dev->supports_option(rs::option::r200_lr_auto_exposure_enabled))
            {
                int value = !dev->get_option(rs::option::r200_lr_auto_exposure_enabled);
                std::cout << "Setting auto exposure to " << value << std::endl;
                dev->set_option(rs::option::r200_lr_auto_exposure_enabled, value);
            }
            break;
        }
    });
    glfwMakeContextCurrent(win);

    while (!glfwWindowShouldClose(win))
    {
        // Wait for new images
        glfwPollEvents();
        dev.wait_for_frames();

        // Clear the framebuffer
        int w,h;
        glfwGetFramebufferSize(win, &w, &h);
        glViewport(0, 0, w, h);
        glClear(GL_COLOR_BUFFER_BIT);

        // Draw the images        
        glPushMatrix();
        glfwGetWindowSize(win, &w, &h);
        glOrtho(0, w, h, 0, -1, +1);
        buffers[0].show(dev, align_color_to_depth ? rs::stream::color_aligned_to_depth : (color_rectification_enabled ? rs::stream::rectified_color : rs::stream::color), 0, 0, w/2, h/2);
        buffers[1].show(dev, align_depth_to_color ? (color_rectification_enabled ? rs::stream::depth_aligned_to_rectified_color : rs::stream::depth_aligned_to_color) : rs::stream::depth, w/2, 0, w-w/2, h/2);
        buffers[2].show(dev, rs::stream::infrared, 0, h/2, w/2, h-h/2);
        buffers[3].show(dev, rs::stream::infrared2, w/2, h/2, w-w/2, h-h/2);
        glPopMatrix();
        glfwSwapBuffers(win);
    }

    glfwDestroyWindow(win);
    glfwTerminate();
    return EXIT_SUCCESS;
}
catch(const rs::error & e)
{
    std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
    return EXIT_FAILURE;
}
catch(const std::exception & e)
{
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}
