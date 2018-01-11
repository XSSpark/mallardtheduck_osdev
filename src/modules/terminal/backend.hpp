#ifndef _BACKEND_HPP
#define _BACKEND_HPP
#include <btos_module.h>
#include <dev/terminal.h>
#include <dev/video_dev.h>

class vterm;

class i_backend{
public:
    virtual size_t display_read(size_t bytes, char *buf)=0;
    virtual size_t display_write(size_t bytes, char *buf)=0;
    virtual size_t display_seek(size_t pos, uint32_t flags)=0;

    virtual void show_pointer()=0;
    virtual void hide_pointer()=0;
    virtual bool get_pointer_visibility()=0;
    virtual void set_pointer_bitmap(bt_terminal_pointer_bitmap *bmp)=0;
    virtual bt_terminal_pointer_info get_pointer_info()=0;
	virtual void set_pointer_autohide(bool val) = 0;
	virtual void freeze_pointer()=0;
	virtual void unfreeze_pointer()=0;
	virtual uint32_t get_pointer_speed()=0;
	virtual void set_pointer_speed(uint32_t speed)=0;
	
	
	virtual void set_text_colours(uint8_t c) = 0;
	virtual uint8_t get_text_colours() = 0;
	virtual size_t get_screen_mode_count() = 0;
	virtual void set_screen_mode(const bt_vidmode &mode) = 0;
	virtual bt_vidmode get_screen_mode(size_t index) = 0;
	virtual bt_vidmode get_current_screen_mode() = 0;
	virtual void set_screen_scroll(bool v) = 0;
	virtual bool get_screen_scroll() = 0;
	virtual void set_text_access_mode(bt_vid_text_access_mode::Enum mode) = 0;
	virtual bt_video_palette_entry get_palette_entry(uint8_t entry) = 0;
	virtual void clear_screen() = 0;
	
	virtual void register_global_shortcut(uint16_t keycode, uint64_t termid) = 0;

    virtual bool is_active(uint64_t id)=0;
    virtual void set_active(uint64_t id)=0;

    virtual void open(uint64_t id)=0;
    virtual void close(uint64_t id)=0;
	virtual void switch_terminal(uint64_t id) = 0;
	virtual bool can_create() = 0;
	virtual void refresh() = 0;

    virtual ~i_backend(){};
};

#endif