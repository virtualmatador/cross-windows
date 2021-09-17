//
//  image.h
//  cross
//
//  Created by Ali Asadpoor on 9/21/21.
//  Copyright © 2020 Shaidin. All rights reserved.
//

#ifndef WINDOWS_IMAGE_H
#define WINDOWS_IMAGE_H

#include <mutex>
#include <queue>
#include <vector>

struct LoadImageViewDispatch
{
    const std::int32_t sender;
    const std::int32_t view_info;
    const std::int32_t image_width;
};

class ImageWidget
{
public:
    ImageWidget();
    ~ImageWidget();
    void push_load(const std::int32_t sender, const std::int32_t view_info, const std::int32_t image_width);
    std::uint32_t* get_pixels();
    void release_pixels(std::uint32_t*);
    void refresh_image_view();
    void destroy();
    void pop_load();
    void resize();

private:
    void on_load(const std::int32_t sender, const std::int32_t view_info, const std::int32_t image_width);
    void create_pixels();

private:
    std::vector<std::uint32_t> pixels_;
    std::mutex pixels_lock_;
    int image_width_;
    int image_height_;
    bool resized_;
    std::mutex dispatch_lock_;
    std::queue<LoadImageViewDispatch> dispatch_queue_;
};

#endif // WINDOWS_IMAGE_H
