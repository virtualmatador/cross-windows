//
//  image.cpp
//  cross
//
//  Created by Ali Asadpoor on 10/12/20.
//  Copyright © 2020 Shaidin. All rights reserved.
//

#include <sstream>

#include "extern/core/src/cross.h"

#include "window.h"

#include "image.h"

ImageWidget::ImageWidget()
    : image_width_{ 0 }
    , image_height_{ 0 }
    , resized_{ false }
{
}

ImageWidget::~ImageWidget()
{
    destroy();
}

void ImageWidget::push_load(const std::int32_t sender, const std::int32_t view_info, const std::int32_t image_width)
{
    dispatch_lock_.lock();
    dispatch_queue_.push({sender, view_info, image_width});
    dispatch_lock_.unlock();
    PostMessageA(Window::window_->hwnd_, Window::WM_LOAD_IMAGE_, 0, 0);
}

/*
bool ImageWidget::on_draw(const Cairo::RefPtr<Cairo::Context>& cr)
{
    auto allocation = get_allocation();
    cr->scale((double)allocation.get_width() / (double)pixels_->get_width(), (double)allocation.get_height() / (double)pixels_->get_height());
    pixels_lock_.lock();
    Gdk::Cairo::set_source_pixbuf(cr, pixels_);
    cr->paint();
    pixels_lock_.unlock();
    return true;
}

bool ImageWidget::on_button_press_event(GdkEventButton* button_event)
{
    std::stringstream is;
    is << button_event->x * pixels_->get_width() / image_view_width_;
    is << " ";
    is << button_event->y * pixels_->get_height() / image_view_height_;
    cross::Handle("body", "touch-begin", is.str().c_str());
    return true;
}

bool ImageWidget::on_motion_notify_event(GdkEventMotion* motion_event)
{
    std::stringstream is;
    is << motion_event->x * pixels_->get_width() / image_view_width_;
    is << " ";
    is << motion_event->y * pixels_->get_height() / image_view_height_;
    cross::Handle("body", "touch-move", is.str().c_str());
    return true;
}

bool ImageWidget::on_button_release_event(GdkEventButton* release_event)
{
    std::stringstream is;
    is << release_event->x * pixels_->get_width() / image_view_width_;
    is << " ";
    is << release_event->y * pixels_->get_height() / image_view_height_;
    cross::Handle("body", "touch-end", is.str().c_str());
    return true;
}
*/

std::uint32_t* ImageWidget::get_pixels()
{
    pixels_lock_.lock();
    return pixels_.data();
}

void ImageWidget::release_pixels(std::uint32_t*)
{
    pixels_lock_.unlock();
}

void ImageWidget::refresh_image_view()
{
    // consider double buffer
    if (!resized_)
    {
        PostMessageA(Window::window_->hwnd_, WM_PAINT, 0, 0);
    }
    else
    {
        create_pixels();
        std::ostringstream info;
        info << image_width_ << " " << image_height_;
        cross::Handle("body", "resize", info.str().c_str());
    }
}

void ImageWidget::destroy()
{
    // TODO release hwnd
    pixels_.resize(0);
}

void ImageWidget::pop_load()
{
    dispatch_lock_.lock();
    auto dispatch_info = dispatch_queue_.front();
    dispatch_queue_.pop();
    dispatch_lock_.unlock();
    on_load(dispatch_info.sender, dispatch_info.view_info, dispatch_info.image_width);
}

void ImageWidget::resize()
{
    resized_ = true;
}

void ImageWidget::create_pixels()
{
    float scale = (float)image_width_ / (float)Window::window_->width_;
    image_height_ = (std::int32_t)(Window::window_->height_ * scale);
    pixels_.resize(4 * image_width_ * image_height_);
    resized_ = false;
}

void ImageWidget::on_load(const std::int32_t sender, const std::int32_t view_info,
    const std::int32_t image_width)
{
    Window::window_->prepare_image();
    Window::window_->load_view(sender, view_info);
    image_width_ = image_width;
    create_pixels();
    std::ostringstream info;
    info << image_width_ / 10 << " " << image_width_ << " " << image_height_ << " " << 0x02010003;
    cross::HandleAsync(Window::window_->sender_, "body", "ready", info.str().c_str());
}
