/**
 * Copyright (c) 2012 by Lukas Heidemann <lukasheidemann@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <screen.h>
#include <stdint.h>
#include <ui.h>

ui_page_t ui_pages[UI_PAGE_COUNT];

size_t ui_page_current = 0;
size_t ui_line_current = 0;

static char *ui_get_line(char *string, size_t line) {
    size_t line_cur = 0;
    size_t i;

    for (i = 0; '\0' != string[i] && line_cur < line; ++i) {
        if ('\n' == string[i]) {
            ++line_cur;
        }
    }

    return &string[i];
}

static void ui_display_header(ui_page_t *page) {
    screen_write("H2 Test Utility - ", 0, 0);
    screen_write(page->title, 18, 0);
    screen_write("----------------------------------------", 0, 1);
    screen_write("----------------------------------------", 40, 1);
}

static void ui_display_body(ui_page_t *page, size_t line) {
    char *body = ui_get_line(page->body, line);
    screen_write(body, 0, 2);
}

void ui_display(size_t page_nr, size_t line)
{
    ui_page_t *page = &ui_pages[page_nr % UI_PAGE_COUNT];

    screen_clear();
    ui_display_header(page);
    ui_display_body(page, line);
}

void ui_switch_left(void)
{
    if (0 != ui_page_current) {
        ui_line_current = 0;
        --ui_page_current;
        ui_display(ui_page_current, ui_line_current);
    }
}

void ui_switch_right(void)
{
    if (ui_page_current < UI_PAGE_COUNT - 1) {
        ui_line_current = 0;
        ++ui_page_current;
        ui_display(ui_page_current, ui_line_current);
    }
}

void ui_scroll_up(void)
{
    if (0 != ui_line_current) {
        ui_display(ui_page_current, --ui_line_current);
    }
}

void ui_scroll_down(void)
{
    ui_display(ui_page_current, ++ui_line_current);
}
