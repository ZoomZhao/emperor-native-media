#ifndef STAGE15_GUI_H
#define STAGE15_GUI_H
#include <stddef.h>
#include <stdint.h>
typedef struct s15_gui s15_gui;
#define S15_PROFILE_WORDS 18
s15_gui *s15_gui_new(const uint32_t *explicit_test_fixture);
void s15_gui_delete(s15_gui *);
int s15_gui_call(s15_gui *,uint32_t kind,uint32_t a,uint32_t b,uint32_t *value,uint32_t *error);
int s15_gui_profile(s15_gui *,uint32_t *,size_t);
size_t s15_gui_size(void);
int s15_gui_export(s15_gui *,void *,size_t);
int s15_gui_import(s15_gui *,const void *,size_t);
uint32_t s15_gui_object_kind(s15_gui *,uint32_t);
int s15_gui_brush_pixel(s15_gui *,uint32_t,uint32_t *);
#endif
