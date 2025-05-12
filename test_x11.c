#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    Display *display;
    Window window;
    XEvent event;
    char *msg = "Hello, X11!";
    int screen;

    /* 打开连接到X服务器 */
    display = XOpenDisplay(NULL);
    if (display == NULL) {
        fprintf(stderr, "Cannot open display\n");
        exit(1);
    }

    /* 获取默认屏幕 */
    screen = DefaultScreen(display);

    /* 创建窗口 */
    window = XCreateSimpleWindow(display, RootWindow(display, screen),
                               10, 10, 200, 100, 1,
                               BlackPixel(display, screen), WhitePixel(display, screen));

    /* 选择窗口事件 */
    XSelectInput(display, window, ExposureMask | KeyPressMask);

    /* 显示窗口 */
    XMapWindow(display, window);

    /* 事件循环 */
    while (1) {
        XNextEvent(display, &event);

        /* 绘制或重绘窗口 */
        if (event.type == Expose) {
            XDrawString(display, window, DefaultGC(display, screen),
                      10, 50, msg, strlen(msg));
        }
        /* 按键退出 */
        if (event.type == KeyPress)
            break;
    }

    /* 关闭连接到X服务器 */
    XCloseDisplay(display);

    return 0;
}
