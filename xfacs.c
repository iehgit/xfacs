#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <unistd.h>
#include <time.h>

#define MAX_FILE_SIZE (LONG_MAX - 1)
#define ABSOLUTE_TIMEOUT_SEC 600  // Terminate if no paste within 600 seconds

int main(int argc, char *argv[]) {
    if (argc != 2) {
        return 1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        return 1;
    }
    
    if (pid > 0) {
        return 0;
    }
    
    setsid();

    FILE *file = fopen(argv[1], "rb");
    if (!file) {
        return 1;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return 1;
    }
    
    long length = ftell(file);
    if (length == -1L || length > MAX_FILE_SIZE) {
        fclose(file);
        return 1;
    }

    rewind(file);
    char *buffer = malloc(length);
    if (!buffer || fread(buffer, 1, length, file) != (size_t)length) {
        fclose(file);
        free(buffer);
        return 1;
    }
    fclose(file);

    // Check if last byte is newline and adjust length if so
    if (length > 0 && buffer[length - 1] == '\n') {
        length--;
    }

    Display *display = XOpenDisplay(NULL);
    if (!display) {
        free(buffer);
        return 1;
    }

    Window window = XCreateSimpleWindow(display, 
        RootWindow(display, DefaultScreen(display)), 
        0, 0, 1, 1, 0, 0, 0);

    Atom clipboard = XInternAtom(display, "CLIPBOARD", False);
    Atom utf8 = XInternAtom(display, "UTF8_STRING", False);
    Atom text = XInternAtom(display, "TEXT", False);
    Atom compound_text = XInternAtom(display, "COMPOUND_TEXT", False);
    Atom string = XA_STRING;
    Atom targets = XInternAtom(display, "TARGETS", False);
    Atom text_plain_utf8 = XInternAtom(display, "text/plain;charset=utf-8", False);

    XSetSelectionOwner(display, clipboard, window, CurrentTime);
    if (XGetSelectionOwner(display, clipboard) != window) {
        XDestroyWindow(display, window);
        XCloseDisplay(display);
        free(buffer);
        return 1;
    }

    XEvent event;
    int done = 0;
    int request_count = 0;
    struct timespec first_request_time = {0};
    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    XSelectInput(display, window, PropertyChangeMask);

    while (!done) {
        XNextEvent(display, &event);
        
        // Check absolute timeout
        struct timespec current_time;
        clock_gettime(CLOCK_MONOTONIC, &current_time);
        long elapsed_seconds = current_time.tv_sec - start_time.tv_sec;
        if (elapsed_seconds >= ABSOLUTE_TIMEOUT_SEC) {
            done = 1;
            break;
        }

        if (event.type == SelectionClear) {
            done = 1;
        }
        else if (event.type == SelectionRequest) {
            XSelectionRequestEvent *req = &event.xselectionrequest;

            XSelectionEvent sel = {
                .type = SelectionNotify,
                .display = req->display,
                .requestor = req->requestor,
                .selection = req->selection,
                .target = req->target,
                .property = req->property,
                .time = req->time
            };

            if (req->target == targets) {
                Atom supported[] = {targets, utf8, string, text, compound_text, text_plain_utf8};
                XChangeProperty(display, req->requestor, req->property,
                               XA_ATOM, 32, PropModeReplace,
                               (unsigned char *)supported, 6);
            }
            else if (req->target == utf8 || 
                     req->target == string || 
                     req->target == text || 
                     req->target == compound_text ||
                     req->target == text_plain_utf8) {
                XChangeProperty(display, req->requestor, req->property,
                               req->target, 8, PropModeReplace,
                               (unsigned char *)buffer, length);
                request_count++;
                
                if (request_count == 1) {
                    clock_gettime(CLOCK_MONOTONIC, &first_request_time);
                } else if (request_count == 2) {
                    done = 1;
                }
            }
            else {
                sel.property = None;
            }

            XSendEvent(display, req->requestor, False, 0, (XEvent *)&sel);
            XSync(display, False);
        }

        if (request_count == 1) {
            clock_gettime(CLOCK_MONOTONIC, &current_time);
            long elapsed_nanoseconds = 
                (current_time.tv_sec - first_request_time.tv_sec) * 1000000000L +
                (current_time.tv_nsec - first_request_time.tv_nsec);
            if (elapsed_nanoseconds >= 10000000) { // 10ms = 10,000,000 nanoseconds
                done = 1;
            }
        }
    }
    
    XSync(display, False);
    XDestroyWindow(display, window);
    XCloseDisplay(display);
    free(buffer);

    return 0;
}
