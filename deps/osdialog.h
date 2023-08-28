#pragma once

#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>
#include <stddef.h>


#ifndef OSDIALOG_MALLOC
#define OSDIALOG_MALLOC malloc
#endif

#ifndef OSDIALOG_FREE
#define OSDIALOG_FREE free
#endif


char* osdialog_strdup(const char* s);
char* osdialog_strndup(const char* s, size_t n);


typedef enum {
    OSDIALOG_INFO,
    OSDIALOG_WARNING,
    OSDIALOG_ERROR,
} osdialog_message_level;

typedef enum {
    OSDIALOG_OK,
    OSDIALOG_OK_CANCEL,
    OSDIALOG_YES_NO,
} osdialog_message_buttons;

/** Launches a message box.
Returns 1 if the "OK" or "Yes" button was pressed.
*/
int osdialog_message(osdialog_message_level level, osdialog_message_buttons buttons, const char* message);

/** Launches an input prompt with an "OK" and "Cancel" button.
`text` is the default string to fill the input box.
Returns the entered text, or NULL if the dialog was cancelled.
If the returned result is not NULL, caller must free() it.
TODO: Implement on Windows and GTK2.
*/
char* osdialog_prompt(osdialog_message_level level, const char* message, const char* text);


/** Linked list of patterns. */
typedef struct osdialog_filter_patterns {
    char* pattern;
    struct osdialog_filter_patterns* next;
} osdialog_filter_patterns;

/** Linked list of file filters. */
typedef struct osdialog_filters {
    char* name;
    osdialog_filter_patterns* patterns;
    struct osdialog_filters* next;
} osdialog_filters;

/** Parses a filter string.
Example: "Source:c,cpp,m;Header:h,hpp"
Caller must eventually free with osdialog_filters_free().
*/
osdialog_filters* osdialog_filters_parse(const char* str);
void osdialog_filter_patterns_free(osdialog_filter_patterns* patterns);
void osdialog_filters_free(osdialog_filters* filters);


typedef enum {
    OSDIALOG_OPEN,
    OSDIALOG_OPEN_DIR,
    OSDIALOG_SAVE,
} osdialog_file_action;

/** Launches a file dialog and returns the selected path or NULL if nothing was selected.
`path` is the default folder the file dialog will attempt to open in, or NULL for the OS's default.
`filename` is the default text that will appear in the filename input, or NULL for the OS's default. Relevant to save dialog only.
`filters` is a list of patterns to filter the file selection, or NULL.
Returns the selected file, or NULL if the dialog was cancelled.
If the return result is not NULL, caller must free() it.
*/
char* osdialog_file(osdialog_file_action action, const char* path, const char* filename, osdialog_filters* filters);


typedef struct {
    uint8_t r, g, b, a;
} osdialog_color;

/** Launches an RGBA color picker dialog and sets `color` to the selected color.
Returns 1 if "OK" was pressed.
`color` should be set to the initial color before calling. It is only overwritten if the user selects "OK".
`opacity` enables the opacity slider by setting to 1. Not supported on Windows.
TODO Implement on Mac.
*/
int osdialog_color_picker(osdialog_color* color, int opacity);


#ifdef __cplusplus
}
#endif

#if defined(OSDIALOG_IMPLEMENTATION)
char* osdialog_strdup(const char* s) {
    return osdialog_strndup(s, strlen(s));
}

char* osdialog_strndup(const char* s, size_t n) {
    char* d = OSDIALOG_MALLOC(n + 1);
    memcpy(d, s, n);
    d[n] = '\0';
    return d;
}

osdialog_filters* osdialog_filters_parse(const char* str) {
    osdialog_filters* filters_head = OSDIALOG_MALLOC(sizeof(osdialog_filters));
    filters_head->next = NULL;

    osdialog_filters* filters = filters_head;
    osdialog_filter_patterns* patterns = NULL;

    const char* text = str;
    while (1) {
        switch (*str) {
            case ':': {
                filters->name = osdialog_strndup(text, str - text);
                filters->patterns = OSDIALOG_MALLOC(sizeof(osdialog_filter_patterns));
                patterns = filters->patterns;
                patterns->next = NULL;
                text = str + 1;
            } break;
            case ',': {
                assert(patterns);
                patterns->pattern = osdialog_strndup(text, str - text);
                patterns->next = OSDIALOG_MALLOC(sizeof(osdialog_filter_patterns));
                patterns = patterns->next;
                patterns->next = NULL;
                text = str + 1;
            } break;
            case ';': {
                assert(patterns);
                patterns->pattern = osdialog_strndup(text, str - text);
                filters->next = OSDIALOG_MALLOC(sizeof(osdialog_filters));
                filters = filters->next;
                filters->next = NULL;
                patterns = NULL;
                text = str + 1;
            } break;
            case '\0': {
                assert(patterns);
                patterns->pattern = osdialog_strndup(text, str - text);
            } break;
            default: break;
        }
        if (!*str)
            break;
        str++;
    }

    return filters_head;
}

void osdialog_filter_patterns_free(osdialog_filter_patterns* patterns) {
    if (!patterns)
        return;
    OSDIALOG_FREE(patterns->pattern);
    osdialog_filter_patterns* next = patterns->next;
    OSDIALOG_FREE(patterns);
    osdialog_filter_patterns_free(next);
}

void osdialog_filters_free(osdialog_filters* filters) {
    if (!filters)
        return;
    OSDIALOG_FREE(filters->name);
    osdialog_filter_patterns_free(filters->patterns);
    osdialog_filters* next = filters->next;
    OSDIALOG_FREE(filters);
    osdialog_filters_free(next);
}

#if defined(macintosh) || defined(Macintosh) || (defined(__APPLE__) && defined(__MACH__))
#include <AppKit/AppKit.h>
#include <Availability.h>
int osdialog_message(osdialog_message_level level, osdialog_message_buttons buttons, const char* message) {
    @autoreleasepool {

        NSWindow* keyWindow = [[NSApplication sharedApplication] keyWindow];

        NSAlert* alert = [[NSAlert alloc] init];

        switch (level) {
            default:
#ifdef __MAC_10_12
            case OSDIALOG_INFO: [alert setAlertStyle:NSAlertStyleInformational]; break;
            case OSDIALOG_WARNING: [alert setAlertStyle:NSAlertStyleWarning]; break;
            case OSDIALOG_ERROR: [alert setAlertStyle:NSAlertStyleCritical]; break;
#else
            case OSDIALOG_INFO: [alert setAlertStyle:NSInformationalAlertStyle]; break;
            case OSDIALOG_WARNING: [alert setAlertStyle:NSWarningAlertStyle]; break;
            case OSDIALOG_ERROR: [alert setAlertStyle:NSCriticalAlertStyle]; break;
#endif
        }

        switch (buttons) {
            default:
            case OSDIALOG_OK:
                [alert addButtonWithTitle:@"OK"];
                break;
            case OSDIALOG_OK_CANCEL:
                [alert addButtonWithTitle:@"OK"];
                [alert addButtonWithTitle:@"Cancel"];
                break;
            case OSDIALOG_YES_NO:
                [alert addButtonWithTitle:@"Yes"];
                [alert addButtonWithTitle:@"No"];
                break;
        }

        NSString* messageString = [NSString stringWithUTF8String:message];
        [alert setMessageText:messageString];
        // Non-bold text
        // [alert setInformativeText:messageString];

        NSInteger button = [alert runModal];

        [keyWindow makeKeyAndOrderFront:nil];

        return (button == NSAlertFirstButtonReturn);
    } // @autoreleasepool
}


char* osdialog_prompt(osdialog_message_level level, const char* message, const char* text) {
    @autoreleasepool {

        NSWindow* keyWindow = [[NSApplication sharedApplication] keyWindow];

        NSAlert* alert = [[NSAlert alloc] init];

        switch (level) {
            default:
#ifdef __MAC_10_12
            case OSDIALOG_INFO: [alert setAlertStyle:NSAlertStyleInformational]; break;
            case OSDIALOG_WARNING: [alert setAlertStyle:NSAlertStyleWarning]; break;
            case OSDIALOG_ERROR: [alert setAlertStyle:NSAlertStyleCritical]; break;
#else
            case OSDIALOG_INFO: [alert setAlertStyle:NSInformationalAlertStyle]; break;
            case OSDIALOG_WARNING: [alert setAlertStyle:NSWarningAlertStyle]; break;
            case OSDIALOG_ERROR: [alert setAlertStyle:NSCriticalAlertStyle]; break;
#endif
        }

        [alert addButtonWithTitle:@"OK"];
        [alert addButtonWithTitle:@"Cancel"];

        NSString* messageString = [NSString stringWithUTF8String:message];
        [alert setMessageText:messageString];

        NSTextField* input = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 300, 24)];
        [alert setAccessoryView:input];

        if (text) {
            NSString* path_str = [NSString stringWithUTF8String:text];
            [input setStringValue:path_str];
        }

        NSInteger button = [alert runModal];

        char* result = NULL;
        if (button == NSAlertFirstButtonReturn) {
            [input validateEditing];
            NSString* result_str = [input stringValue];
            // Don't use NSString.length because it returns the number of the UTF-16 code units, not the number of bytes.
            result = osdialog_strdup([result_str UTF8String]);
        }

        [keyWindow makeKeyAndOrderFront:nil];

        return result;
    } // @autoreleasepool
}


char* osdialog_file(osdialog_file_action action, const char* dir, const char* filename, osdialog_filters* filters) {
    @autoreleasepool {

        NSWindow* keyWindow = [[NSApplication sharedApplication] keyWindow];

        NSSavePanel* panel;
        // NSOpenPanel is a subclass of NSSavePanel. Not defined for OSDIALOG_SAVE.
        NSOpenPanel* open_panel;

        if (action == OSDIALOG_OPEN || action == OSDIALOG_OPEN_DIR) {
            panel = open_panel = [NSOpenPanel openPanel];
        }
        else {
            panel = [NSSavePanel savePanel];
        }

        // Bring dialog to front
        // https://stackoverflow.com/a/2402069
        // Thanks Dave!
        [panel setLevel:CGShieldingWindowLevel()];

        if (filters) {
            NSMutableArray* fileTypes = [[NSMutableArray alloc] init];

            for (; filters; filters = filters->next) {
                for (osdialog_filter_patterns* patterns = filters->patterns; patterns; patterns = patterns->next) {
                    NSString* fileType = [NSString stringWithUTF8String:patterns->pattern];
                    [fileTypes addObject:fileType];
                }
            }

            [panel setAllowedFileTypes:fileTypes];
        }

        if (action == OSDIALOG_OPEN || action == OSDIALOG_OPEN_DIR) {
            [open_panel setAllowsMultipleSelection:NO];
        }
        if (action == OSDIALOG_OPEN) {
            [open_panel setCanChooseDirectories:NO];
            [open_panel setCanChooseFiles:YES];
        }
        if (action == OSDIALOG_OPEN_DIR) {
            [open_panel setCanCreateDirectories:YES];
            [open_panel setCanChooseDirectories:YES];
            [open_panel setCanChooseFiles:NO];
        }

        if (dir) {
            NSString* dir_str = [NSString stringWithUTF8String:dir];
            NSURL* dir_url = [NSURL fileURLWithPath:dir_str];
            [panel setDirectoryURL:dir_url];
        }

        if (filename) {
            NSString* filenameString = [NSString stringWithUTF8String:filename];
            [panel setNameFieldStringValue:filenameString];
        }

        char* result = NULL;

        NSModalResponse response = [panel runModal];
#ifdef __MAC_10_9
#define OK NSModalResponseOK
#else
#define OK NSOKButton
#endif
        if (response == OK) {
            NSURL* result_url = [panel URL];
            NSString* result_str = [result_url path];
            // Don't use NSString.length because it returns the number of the UTF-16 code units, not the number of bytes.
            result = osdialog_strdup([result_str UTF8String]);
        }

        [keyWindow makeKeyAndOrderFront:nil];

        return result;
    } // @autoreleasepool
}


int osdialog_color_picker(osdialog_color* color, int opacity) {
    (void) color;
    (void) opacity;
    // Not yet implemented
    return 0;

    @autoreleasepool {

        // TODO I have no idea what I'm doing here
        NSColorPanel* panel = [NSColorPanel sharedColorPanel];
        // [panel setDelegate:self];
        [panel isVisible];

        // if (opacity)
        //     [panel setShowAlpha:YES];
        // else
        //     [panel setShowAlpha:NO];

        // [panel makeKeyAndOrderFront:self];

        return 0;
    } // @autoreleasepool
}
#elif defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) || defined(__WINDOWS__)
#include <string.h>
#include <stdio.h>
#include <windows.h>
#include <commdlg.h>
#include <shlobj.h>

#define DLG_FONT L"MS Shell Dlg"
#define DLG_OK L"&OK"
#define DLG_CANCEL L"&Cancel"
#define LENGTHOF(a) (sizeof(a) / sizeof((a)[0]))

#pragma pack(push, 4)
static struct {
    DWORD style;
    DWORD dwExtendedStyle;
    WORD  cdit; // number of controls
    short x;
    short y;
    short cx;
    short cy;
    WORD menu;
    WORD windowClass;
    WCHAR title[1];
    short pointSize;
    WCHAR typeface[LENGTHOF(DLG_FONT)];

    struct {
        DWORD style;
        DWORD dwExtendedStyle;
        short x;
        short y;
        short cx;
        short cy;
        WORD  id;
        WORD sysClass; // 0xFFFF identifies a system window class
        WORD idClass; // ordinal of a system window class
        WCHAR title[1];
        WORD cbCreationData; // bytes of following creation data
    } edit;

    struct {
        DWORD style;
        DWORD dwExtendedStyle;
        short x;
        short y;
        short cx;
        short cy;
        WORD  id;
        WORD sysClass; // 0xFFFF identifies a system window class
        WORD idClass; // ordinal of a system window class
        WCHAR title[LENGTHOF(DLG_OK)];
        WORD cbCreationData; // bytes of following creation data
    } ok;

    struct {
        DWORD style;
        DWORD dwExtendedStyle;
        short x;
        short y;
        short cx;
        short cy;
        WORD  id;
        WORD sysClass; // 0xFFFF identifies a system window class
        WORD idClass; // ordinal of a system window class
        WCHAR title[LENGTHOF(DLG_CANCEL)];
        WORD cbCreationData; // bytes of following creation data
    } cancel;

} promptTemplate = {
    WS_POPUP | WS_VISIBLE | WS_CAPTION | WS_SYSMENU | DS_MODALFRAME | DS_CENTER | DS_SHELLFONT,
    0x0, // dwExtendedStyle
    3, // cdit
    0, 0, 5+200+10+50+5+50+5, 5+14+5,
    0, // menu
    0, // windowClass
    L"", // title
    8, // typeface
    DLG_FONT,

    {
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_LEFT | ES_AUTOHSCROLL,
        WS_EX_NOPARENTNOTIFY,
        5, 5, 200, 14,
        42,
        0xFFFF, 0x0081, // Edit
        L"", 0,
    },

    {
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
        WS_EX_NOPARENTNOTIFY,
        5+200+10, 5, 50, 14,
        IDOK,
        0xFFFF, 0x0080, // Button
        DLG_OK, 0,
    },

    {
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        WS_EX_NOPARENTNOTIFY,
        5+200+10+50+5, 5, 50, 14,
        IDCANCEL,
        0xFFFF, 0x0080, // Button
        DLG_CANCEL, 0,
    },
};
#pragma pack(pop)

static wchar_t promptBuffer[1 << 13] = L"";

static INT_PTR CALLBACK promptProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    (void) lParam;

    switch (message) {
        case WM_INITDIALOG: {
            SendDlgItemMessageW(hDlg, 42, WM_SETTEXT, (WPARAM) 0, (LPARAM) promptBuffer);
            return TRUE;
        } break;

        case WM_DESTROY: {
            EndDialog(hDlg, 0);
            return TRUE;
        } break;

        case WM_COMMAND: {
            switch (wParam) {
                case IDOK: {
                    int len = SendDlgItemMessageW(hDlg, 42, WM_GETTEXT, (WPARAM) LENGTHOF(promptBuffer), (LPARAM) promptBuffer);
                    (void) len;
                    EndDialog(hDlg, 1);
                    return TRUE;
                } break;

                case IDCANCEL: {
                    EndDialog(hDlg, 0);
                    return TRUE;
                } break;
            }
        } break;
    }
    return FALSE;
}

char* osdialog_prompt(osdialog_message_level level, const char* message, const char* text) {
    (void) level;
    (void) message;

    promptBuffer[0] = 0;
    if (text) {
        MultiByteToWideChar(CP_UTF8, 0, text, -1, promptBuffer, LENGTHOF(promptBuffer));
    }

    HWND window = GetActiveWindow();
    int res = DialogBoxIndirectParamW(NULL, (LPCDLGTEMPLATEW) &promptTemplate, window, promptProc, (LPARAM) NULL);
    if (res) {
        return wchar_to_utf8(promptBuffer);
    }
    return NULL;
}


static INT CALLBACK browseCallbackProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    (void) wParam;

    if (message == BFFM_INITIALIZED) {
        SendMessageW(hWnd, BFFM_SETEXPANDED, 1, lParam);
    }
    return 0;
}

char* osdialog_file(osdialog_file_action action, const char* dir, const char* filename, osdialog_filters* filters) {
    if (action == OSDIALOG_OPEN_DIR) {
        // open directory dialog
        BROWSEINFOW bInfo;
        ZeroMemory(&bInfo, sizeof(bInfo));
        bInfo.hwndOwner = GetActiveWindow();
        bInfo.ulFlags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI;

        // dir
        wchar_t initialDir[MAX_PATH] = L"";
        if (dir) {
            // We need to convert the path to a canonical absolute path with GetFullPathNameW()
            wchar_t* dirW = utf8_to_wchar(dir);
            GetFullPathNameW(dirW, MAX_PATH, initialDir, NULL);
            OSDIALOG_FREE(dirW);
            bInfo.lpfn = (BFFCALLBACK) browseCallbackProc;
            bInfo.lParam = (LPARAM) initialDir;
        }

        PIDLIST_ABSOLUTE lpItem = SHBrowseForFolderW(&bInfo);
        if (!lpItem) {
            return NULL;
        }
        wchar_t szDir[MAX_PATH] = L"";
        SHGetPathFromIDListW(lpItem, szDir);
        return wchar_to_utf8(szDir);
    }
    else {
        // open or save file dialog
        OPENFILENAMEW ofn;
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = GetActiveWindow();
        ofn.Flags = OFN_EXPLORER | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

        // filename
        wchar_t strFile[MAX_PATH] = L"";
        if (filename) {
            wchar_t* filenameW = utf8_to_wchar(filename);
            snwprintf(strFile, MAX_PATH, L"%S", filenameW);
            OSDIALOG_FREE(filenameW);
        }
        ofn.lpstrFile = strFile;
        ofn.nMaxFile = MAX_PATH;

        // dir
        wchar_t strInitialDir[MAX_PATH] = L"";
        if (dir) {
            // We need to convert the dir to a canonical absolute dir with GetFullPathNameW()
            wchar_t* dirW = utf8_to_wchar(dir);
            GetFullPathNameW(dirW, MAX_PATH, strInitialDir, NULL);
            OSDIALOG_FREE(dirW);
            ofn.lpstrInitialDir = strInitialDir;
        }

        // filters
        wchar_t* strFilter = NULL;
        if (filters) {
            char fBuf[4096];
            int fLen = 0;

            for (; filters; filters = filters->next) {
                fLen += snprintf(fBuf + fLen, sizeof(fBuf) - fLen, "%s", filters->name);
                fBuf[fLen++] = '\0';
                for (osdialog_filter_patterns* patterns = filters->patterns; patterns; patterns = patterns->next) {
                    fLen += snprintf(fBuf + fLen, sizeof(fBuf) - fLen, "*.%s", patterns->pattern);
                    if (patterns->next)
                        fLen += snprintf(fBuf + fLen, sizeof(fBuf) - fLen, ";");
                }
                fBuf[fLen++] = '\0';
            }
            fBuf[fLen++] = '\0';

            // Don't use utf8_to_wchar() because this is not a NULL-terminated string.
            strFilter = OSDIALOG_MALLOC(fLen * sizeof(wchar_t));
            MultiByteToWideChar(CP_UTF8, 0, fBuf, fLen, strFilter, fLen);
            ofn.lpstrFilter = strFilter;
            ofn.nFilterIndex = 1;
        }

        BOOL success;
        if (action == OSDIALOG_OPEN) {
            success = GetOpenFileNameW(&ofn);
        }
        else {
            success = GetSaveFileNameW(&ofn);
        }

        // Clean up
        if (strFilter) {
            OSDIALOG_FREE(strFilter);
        }
        if (!success) {
            return NULL;
        }
        return wchar_to_utf8(strFile);
    }
}


int osdialog_color_picker(osdialog_color* color, int opacity) {
    (void) opacity;
    if (!color)
        return 0;

    CHOOSECOLOR cc;
    ZeroMemory(&cc, sizeof(cc));

    COLORREF c = RGB(color->r, color->g, color->b);
    static COLORREF acrCustClr[16];

    cc.lStructSize = sizeof(cc);
    cc.lpCustColors = (LPDWORD) acrCustClr;
    cc.rgbResult = c;
    cc.Flags = CC_FULLOPEN | CC_ANYCOLOR | CC_RGBINIT;

    if (ChooseColor(&cc)) {
        color->r = GetRValue(cc.rgbResult);
        color->g = GetGValue(cc.rgbResult);
        color->b = GetBValue(cc.rgbResult);
        color->a = 255;
        return 1;
    }

    return 0;
}
#elif defined(__gnu_linux__) || defined(__linux__) || defined(__unix__)
#include <string.h>
#include <gtk/gtk.h>

int osdialog_message(osdialog_message_level level, osdialog_message_buttons buttons, const char* message) {
    if (!gtk_init_check(NULL, NULL))
        return 0;

    GtkMessageType messageType;
    switch (level) {
        default:
        case OSDIALOG_INFO: messageType = GTK_MESSAGE_INFO; break;
        case OSDIALOG_WARNING: messageType = GTK_MESSAGE_WARNING; break;
        case OSDIALOG_ERROR: messageType = GTK_MESSAGE_ERROR; break;
    }

    GtkButtonsType buttonsType;
    switch (buttons) {
        default:
        case OSDIALOG_OK: buttonsType = GTK_BUTTONS_OK; break;
        case OSDIALOG_OK_CANCEL: buttonsType = GTK_BUTTONS_OK_CANCEL; break;
        case OSDIALOG_YES_NO: buttonsType = GTK_BUTTONS_YES_NO; break;
    }

    GtkWidget* dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, messageType, buttonsType, "%s", message);

    gint result = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    while (gtk_events_pending())
        gtk_main_iteration();

    return (result == GTK_RESPONSE_OK || result == GTK_RESPONSE_YES);
}


char* osdialog_prompt(osdialog_message_level level, const char* message, const char* text) {
    if (!gtk_init_check(NULL, NULL))
        return 0;

    GtkMessageType messageType;
    switch (level) {
        default:
        case OSDIALOG_INFO: messageType = GTK_MESSAGE_INFO; break;
        case OSDIALOG_WARNING: messageType = GTK_MESSAGE_WARNING; break;
        case OSDIALOG_ERROR: messageType = GTK_MESSAGE_ERROR; break;
    }

    GtkWidget* dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, messageType, GTK_BUTTONS_OK_CANCEL, "%s", message);

    GtkWidget* entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entry), text);

    GtkWidget* content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_add(GTK_CONTAINER(content_area), entry);
    gtk_widget_show_all(dialog);

    gint button = gtk_dialog_run(GTK_DIALOG(dialog));
    const char* result_str = gtk_entry_get_text(GTK_ENTRY(entry));

    char* result = NULL;
    if (button == GTK_RESPONSE_OK) {
        result = osdialog_strndup(result_str, strlen(result_str));
    }
    gtk_widget_destroy(dialog);

    while (gtk_events_pending())
        gtk_main_iteration();

    return result;
}


char* osdialog_file(osdialog_file_action action, const char* dir, const char* filename, osdialog_filters* filters) {
    if (!gtk_init_check(NULL, NULL))
        return 0;

    GtkFileChooserAction gtkAction;
    const char* title;
    const char* acceptText;
    if (action == OSDIALOG_OPEN) {
        title = "Open File";
        acceptText = "Open";
        gtkAction = GTK_FILE_CHOOSER_ACTION_OPEN;
    }
    else if (action == OSDIALOG_OPEN_DIR) {
        title = "Open Folder";
        acceptText = "Open Folder";
        gtkAction = GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER;
    }
    else {
        title = "Save File";
        acceptText = "Save";
        gtkAction = GTK_FILE_CHOOSER_ACTION_SAVE;
    }

    GtkWidget* dialog = gtk_file_chooser_dialog_new(
                          title,
                          NULL,
                          gtkAction,
                          "_Cancel", GTK_RESPONSE_CANCEL,
                          acceptText, GTK_RESPONSE_ACCEPT,
                          NULL);

    for (; filters; filters = filters->next) {
        GtkFileFilter* fileFilter = gtk_file_filter_new();
        gtk_file_filter_set_name(fileFilter, filters->name);
        for (osdialog_filter_patterns* patterns = filters->patterns; patterns; patterns = patterns->next) {
            char patternBuf[1024];
            snprintf(patternBuf, sizeof(patternBuf), "*.%s", patterns->pattern);
            gtk_file_filter_add_pattern(fileFilter, patternBuf);
        }
        gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), fileFilter);
    }

    if (action == OSDIALOG_SAVE)
        gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);

    if (dir)
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), dir);

    if (action == OSDIALOG_SAVE && filename)
        gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), filename);

    char* chosen_filename = NULL;
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        chosen_filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
    }
    gtk_widget_destroy(dialog);

    char* result = NULL;
    if (chosen_filename) {
        result = osdialog_strndup(chosen_filename, strlen(chosen_filename));
        g_free(chosen_filename);
    }

    while (gtk_events_pending())
        gtk_main_iteration();
    return result;
}


int osdialog_color_picker(osdialog_color* color, int opacity) {
    if (!color)
        return 0;
    if (!gtk_init_check(NULL, NULL))
        return 0;

#ifdef OSDIALOG_GTK3
    GtkWidget* dialog = gtk_color_chooser_dialog_new("Color", NULL);
    GtkColorChooser* colorsel = GTK_COLOR_CHOOSER(dialog);
    gtk_color_chooser_set_use_alpha(colorsel, opacity);
#else
    GtkWidget* dialog = gtk_color_selection_dialog_new("Color");
    GtkColorSelection* colorsel = GTK_COLOR_SELECTION(gtk_color_selection_dialog_get_color_selection(GTK_COLOR_SELECTION_DIALOG(dialog)));
    gtk_color_selection_set_has_opacity_control(colorsel, opacity);
#endif

    int result = 0;
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
#ifdef OSDIALOG_GTK3
        GdkRGBA c;
        gtk_color_chooser_get_rgba(colorsel, &c);
        color->r = c.red * 65535 + 0.5;
        color->g = c.green * 65535 + 0.5;
        color->b = c.blue * 65535 + 0.5;
        color->a = c.alpha * 65535 + 0.5;
#else
        GdkColor c;
        gtk_color_selection_get_current_color(colorsel, &c);
        color->r = c.red >> 8;
        color->g = c.green >> 8;
        color->b = c.blue >> 8;
        color->a = gtk_color_selection_get_current_alpha(colorsel) >> 8;
#endif

        result = 1;
    }

    gtk_widget_destroy(dialog);

    while (gtk_events_pending())
        gtk_main_iteration();
    return result;
}
#else
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h> // for waitpid

static const char zenityBin[] = "zenity";


static void string_list_clear(char** list) {
    while (*list) {
        OSDIALOG_FREE(*list);
        *list = NULL;
        list++;
    }
}


static int string_list_exec(const char* path, const char* const* args, char* outBuf, size_t outLen, char* errBuf, size_t errLen) {
    int outStream[2];
    if (outBuf)
        if (pipe(outStream) == -1)
            return -1;

    int errStream[2];
    if (errBuf)
        if (pipe(errStream) == -1)
            return -1;

    // The classic fork-and-exec routine
    pid_t pid = fork();
    if (pid < 0) {
        return -1;
    }
    else if (pid == 0) {
        // child process
        // Route stdout to outStream
        if (outBuf) {
            while ((dup2(outStream[1], STDOUT_FILENO) == -1) && (errno == EINTR)) {}
            close(outStream[0]);
            close(outStream[1]);
        }
        // Route stdout to outStream
        if (errBuf) {
            while ((dup2(errStream[1], STDERR_FILENO) == -1) && (errno == EINTR)) {}
            close(errStream[0]);
            close(errStream[1]);
        }
        // POSIX guarantees that execvp does not modify the args array, so it's safe to remove const with a cast.
        int err = execvp(path, (char* const*) args);
        if (err)
            exit(0);
        // Will never reach
        exit(0);
    }

    // parent process
    // Close the pipe inputs because the parent doesn't need them
    if (outBuf)
        close(outStream[1]);
    if (errBuf)
        close(errStream[1]);
    // Wait for child process to close
    int status = -1;
    int options = 0;
    waitpid(pid, &status, options);
    // Read streams
    if (outBuf) {
        ssize_t count = read(outStream[0], outBuf, outLen - 1);
        if (count < 0)
            count = 0;
        outBuf[count] = '\0';
        close(outStream[0]);
    }
    if (errBuf) {
        ssize_t count = read(errStream[0], errBuf, errLen - 1);
        if (count < 0)
            count = 0;
        errBuf[count] = '\0';
        close(errStream[0]);
    }
    return status;
}


int osdialog_message(osdialog_message_level level, osdialog_message_buttons buttons, const char* message) {
    char* args[32];
    int argIndex = 0;

    args[argIndex++] = osdialog_strdup(zenityBin);
    // The API doesn't provide a title, so just make it blank.
    args[argIndex++] = osdialog_strdup("--title");
    args[argIndex++] = osdialog_strdup("");

    args[argIndex++] = osdialog_strdup("--no-markup");

    args[argIndex++] = osdialog_strdup("--width");
    args[argIndex++] = osdialog_strdup("500");

    if (buttons == OSDIALOG_OK_CANCEL) {
        args[argIndex++] = osdialog_strdup("--question");
        args[argIndex++] = osdialog_strdup("--ok-label");
        args[argIndex++] = osdialog_strdup("OK");
        args[argIndex++] = osdialog_strdup("--cancel-label");
        args[argIndex++] = osdialog_strdup("Cancel");
    }
    else if (buttons == OSDIALOG_YES_NO) {
        args[argIndex++] = osdialog_strdup("--question");
        args[argIndex++] = osdialog_strdup("--ok-label");
        args[argIndex++] = osdialog_strdup("Yes");
        args[argIndex++] = osdialog_strdup("--cancel-label");
        args[argIndex++] = osdialog_strdup("No");
    }
    else if (level == OSDIALOG_INFO) {
        args[argIndex++] = osdialog_strdup("--info");
    }
    else if (level == OSDIALOG_WARNING) {
        args[argIndex++] = osdialog_strdup("--warning");
    }
    else if (level == OSDIALOG_ERROR) {
        args[argIndex++] = osdialog_strdup("--error");
    }

    args[argIndex++] = osdialog_strdup("--text");
    args[argIndex++] = osdialog_strdup(message);

    args[argIndex++] = NULL;
    int ret = string_list_exec(zenityBin, (const char* const*) args, NULL, 0, NULL, 0);
    string_list_clear(args);
    return ret == 0;
}


char* osdialog_prompt(osdialog_message_level level, const char* message, const char* text) {
    (void) level;
    char* args[32];
    int argIndex = 0;

    args[argIndex++] = osdialog_strdup(zenityBin);
    args[argIndex++] = osdialog_strdup("--title");
    args[argIndex++] = osdialog_strdup("");
    args[argIndex++] = osdialog_strdup("--entry");
    args[argIndex++] = osdialog_strdup("--text");
    args[argIndex++] = osdialog_strdup(message ? message : "");
    args[argIndex++] = osdialog_strdup("--entry-text");
    args[argIndex++] = osdialog_strdup(text ? text : "");
    // Unfortunately the level is ignored

    args[argIndex++] = NULL;
    char outBuf[4096 + 1];
    int ret = string_list_exec(zenityBin, (const char* const*) args, outBuf, sizeof(outBuf), NULL, 0);
    string_list_clear(args);
    if (ret != 0)
        return NULL;

    // Remove trailing newline
    size_t outLen = strlen(outBuf);
    if (outLen > 0)
        outBuf[outLen - 1] = '\0';
    return osdialog_strdup(outBuf);
}


char* osdialog_file(osdialog_file_action action, const char* dir, const char* filename, osdialog_filters* filters) {
    char* args[32];
    int argIndex = 0;

    args[argIndex++] = osdialog_strdup(zenityBin);
    args[argIndex++] = osdialog_strdup("--title");
    args[argIndex++] = osdialog_strdup("");
    args[argIndex++] = osdialog_strdup("--file-selection");
    if (action == OSDIALOG_OPEN) {
        // This is the default
    }
    else if (action == OSDIALOG_OPEN_DIR) {
        args[argIndex++] = osdialog_strdup("--directory");
    }
    else if (action == OSDIALOG_SAVE) {
        args[argIndex++] = osdialog_strdup("--save");
        args[argIndex++] = osdialog_strdup("--confirm-overwrite");
    }

    if (dir || filename) {
        args[argIndex++] = osdialog_strdup("--filename");
        char buf[4096];
        if (dir) {
            // If we don't add a slash, the open dialog will open in the parent directory.
            // If a slash is already present, a second one will have no effect.
            snprintf(buf, sizeof(buf), "%s/%s", dir, filename ? filename : "");
        }
        else {
            snprintf(buf, sizeof(buf), "%s", filename);
        }
        args[argIndex++] = osdialog_strdup(buf);
    }

    for (osdialog_filters* filter = filters; filter; filter = filter->next) {
        args[argIndex++] = osdialog_strdup("--file-filter");

        // Set pattern name
        char patternBuf[1024];
        char* patternPtr = patternBuf;
        const char* patternEnd = patternBuf + sizeof(patternBuf);
        patternPtr += snprintf(patternPtr, patternEnd - patternPtr, "%s |", filter->name);

        // Append pattern
        for (osdialog_filter_patterns* pattern = filter->patterns; pattern; pattern = pattern->next) {
            if (patternPtr >= patternEnd)
                break;
            patternPtr += snprintf(patternPtr, patternEnd - patternPtr, " *.%s", pattern->pattern);
        }
        args[argIndex++] = osdialog_strdup(patternBuf);
    }

    args[argIndex++] = NULL;
    char outBuf[4096 + 1];
    int ret = string_list_exec(zenityBin, (const char* const*) args, outBuf, sizeof(outBuf), NULL, 0);
    string_list_clear(args);
    if (ret != 0)
        return NULL;

    // Remove trailing newline
    size_t outLen = strlen(outBuf);
    if (outLen > 0)
        outBuf[outLen - 1] = '\0';
    return osdialog_strdup(outBuf);
}


int osdialog_color_picker(osdialog_color* color, int opacity) {
    char* args[32];
    int argIndex = 0;

    args[argIndex++] = osdialog_strdup(zenityBin);
    args[argIndex++] = osdialog_strdup("--title");
    args[argIndex++] = osdialog_strdup("");
    args[argIndex++] = osdialog_strdup("--color-selection");

    if (!opacity) {
        color->a = 255;
    }

    // Convert osdialog_color to string
    char buf[128];
    snprintf(buf, sizeof(buf), "rgba(%d,%d,%d,%f)", color->r, color->g, color->b, color->a / 255.f);
    args[argIndex++] = osdialog_strdup("--color");
    args[argIndex++] = osdialog_strdup(buf);

    args[argIndex++] = NULL;
    int ret = string_list_exec(zenityBin, (const char* const*) args, buf, sizeof(buf), NULL, 0);
    string_list_clear(args);
    if (ret != 0)
        return 0;

    // Convert string to osdialog_color
    int r = 0, g = 0, b = 0;
    float a = 1.f;
    if (buf[3] == 'a') {
        sscanf(buf, "rgba(%d,%d,%d,%f)", &r, &g, &b, &a);
    }
    else {
        sscanf(buf, "rgb(%d,%d,%d)", &r, &g, &b);
    }
    color->r = r;
    color->g = g;
    color->b = b;
    color->a = (int) (a * 255.f);

    if (!opacity) {
        color->a = 255;
    }

    return 1;
}
#endif
#endif
