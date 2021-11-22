struct SockMsgAuth {
    enum endianness endian;
    unsigned short protocol_major_version;
    unsigned short protocol_minor_version;
    unsigned short  auth_name_len;
    unsigned short auth_data_len;
};


void Snoop_X_Message(Message &msg) {
    UINT32 i;

    X_message_count++;
    if (message_detail >= Brief_Detail) {
        printf("   Message #%u from X:  Code = %u, Length = %u, Sequence number = %u, Name = ",
               (unsigned int)X_message_count - 1,
               (unsigned int)msg[0],
               (unsigned int)msg.Size(),
               (unsigned int)GetUINT16(msg, 2));
    };
    last_seq_num = GetUINT16(msg, 2);

    if (connect_request_flag && GetUINT16(msg, 2) == 11) {

        connect_request_flag = 0;
        {
            if (msg[28] != 1) {
                fprintf(stderr, "Number of root screens = %u\n", (unsigned int)msg[28]);
                terminate();
            };
            const UINT32 v = GetUINT16(msg, 24);
            const UINT32 n = msg[29];
            const UINT32 m = msg.Size() - 8 * n - v - 40;
            const UINT32 p = (((UINT32)GetUINT16(msg, 6)) - 8 - 2 * n) * 4 - v - m;

            UINT32 root_window_id = GetUINT32(msg, 40 + v + p + 8 * n);

            if (smart_win_on) {
                // *** Create root window and "set it free" (i.e. don't delete it) ***
                Window *win;
                New(win, Window(root_window_id));
            };

            if (message_detail >= Full_Detail) {
                printf("root_window_id = %u\n", (unsigned int)root_window_id);
                printf("m = %u\n", (unsigned int)m);
                printf("n = %u\n", (unsigned int)n);
                printf("p = %u\n", (unsigned int)p);
                printf("v = %u\n", (unsigned int)v);
                printf("Message size in words = %u\n", (unsigned int)GetUINT16(msg, 6));
            };
        };

    } else if (msg.Size() > 0) {
        switch (msg[0]) {

            // *** Key Press ***
            case 2:
                Brief("Key Press\n");
                goto event_handler;
            case 3:
                Brief("Key Release\n");
                goto event_handler;
            case 4:
                Brief("Button Press\n");
                goto event_handler;
            case 5:
                Brief("Button Release\n");
                goto event_handler;
            case 6:
                Brief("Motion Notify\n");
                goto event_handler;
            case 7:
                Brief("Enter Notify\n");
                goto event_handler;
            case 8:
                Brief("Leave Notify\n");
            event_handler:
                if (message_detail >= Full_Detail) {
                    printf("            code = %u\n", (unsigned int)msg[0]);
                    printf("          detail = %u\n", (unsigned int)msg[1]);
                    printf("      sequence # = %u\n", (unsigned int)GetUINT16(msg, 2));
                    printf("       TIMESTAMP = %u\n", (unsigned int)GetUINT32(msg, 4));
                    printf("     root window = %u\n", (unsigned int)GetUINT32(msg, 8));
                    printf("    event window = %u\n", (unsigned int)GetUINT32(msg, 12));

                    if (smart_win_on) {
                        DumpWindow(GetUINT32(msg, 12));
                    };

                    printf("    child window = %u\n", (unsigned int)GetUINT32(msg, 16));

                    if (smart_win_on) {
                        DumpWindow(GetUINT32(msg, 16));
                    };

                    printf("          root-x = %u\n", (unsigned int)GetUINT16(msg, 20));
                    printf("          root-y = %u\n", (unsigned int)GetUINT16(msg, 22));
                    printf("         event-x = %u\n", (unsigned int)GetUINT16(msg, 24));
                    printf("         event-y = %u\n", (unsigned int)GetUINT16(msg, 26));
                    printf(" SETofKEYBUTMASK = %u\n", (unsigned int)GetUINT16(msg, 28));
                    printf("     same-screen = %u\n", (unsigned int)msg[30]);
                    printf("          unused = %u\n", (unsigned int)msg[31]);
                    printf("\n");
                };

                for (i = 0; i <= 31; i++) {
                    event_context.SetDefault(i, msg[i]);
                };

                if (smart_win_on) {
                    printf("%u orphans\n", (unsigned int)orphan_windows.Size());

                    UINT32 wid;
                    for (UINT32 i = 8; i <= 16; i += 4) { // get root, event, child
                        wid = GetUINT32(msg, i);
                        if (wid && !Window::FindWindow(wid)) {
                            AddOrphanWindow(wid);
                        };
                    };
                };

                break;

            default:
                Brief("???\n");
        };
    };
};

void Snoop_PROG_Message(Message &msg) {

    PROG_message_count++;
    if (message_detail >= Brief_Detail) {
        printf("   Message #%u from PROG:  Code = %u, Length = %u, Name = ",
               (unsigned int)PROG_message_count - 1,
               (unsigned int)msg[0],
               (unsigned int)msg.Size());
    };

    int i;

    if (msg.Size() > 0) {
        switch (msg[0]) {

            // *** Connection Setup ***
            case 'B':
            case 'l':
                if (PROG_message_count == 1) {
                    Brief("Connection Request\n");

                    connect_request_flag = 1;

                    // Verify X11
                    if (GetUINT16(msg, 2) != 11) {
                        fprintf(stderr, "Test program is using X version %u\n",
                                (unsigned int)GetUINT16(msg, 2));
                        //	getchar();
                        //	fprintf(stderr, "Error: Test program is not using X version 11\n");
                        //	terminate();
                    };
                } else {
                    Brief("???\n");
                };
                break;

                // *** Create Window ***
            case 1:
                Brief("Create Window\n");
                if (message_detail >= Full_Detail) {
                    printf("         window id = %u\n", (unsigned int)GetUINT32(msg, 4));
                    printf("            parent = %u\n", (unsigned int)GetUINT32(msg, 8));
                    printf("                 x = %i\n", (int)(INT32)GetUINT32(msg, 12));
                    printf("                 y = %i\n", (int)(INT32)GetUINT32(msg, 16));
                    printf("             width = %u\n", (unsigned int)GetUINT16(msg, 18));
                    printf("            height = %u\n", (unsigned int)GetUINT16(msg, 20));
                    printf("      border-width = %u\n", (unsigned int)GetUINT16(msg, 22));
                };

//      printf("Before:\n");
//      Window::Dump();

                if (smart_win_on) {
                    Window *window;
                    New(window, Window(msg));
                };

//      printf("After:\n");
//      Window::Dump();
//      getchar();
                break;

            case 2:
                Brief("ChangeWindowAttributes\n");
                if (message_detail >= Full_Detail) {
                    printf("            window = %u\n", (unsigned int)GetUINT32(msg, 4));
                    printf("        value-mask = %u\n", (unsigned int)GetUINT32(msg, 8));
                };
                if (smart_win_on) {
                    Window *window;
                    UINT32 wid = GetUINT32(msg, 4);
                    window = Window::FindWindow(wid);
#ifdef WIN_STOP
                    if (!window) {
	  fprintf(stderr, "(CWA) PROG specified non-existing window id: %u\n",
		  (unsigned int)wid);
	  terminate();
	};
#endif
                    if (window) {
                        window->ChangeWindowAttributes(msg);
                    } else {
                        AddOrphanWindow(wid);
                    };
                };
                break;

            case 7:
                Brief("ReparentWindow");
                if (message_detail >= Full_Detail) {
                    printf("            window = %u\n", (unsigned int)GetUINT32(msg, 4));
                };
                if (smart_win_on) {
                    Window *window;
                    UINT32 wid = GetUINT32(msg, 4);
                    window = Window::FindWindow(wid);
#ifdef WIN_STOP
                    if (!window) {
	  fprintf(stderr, "PROG specified non-existing parent window id: %u\n",
		  (unsigned int)wid);
	  terminate();
	};
#endif
                    if (window) {
                        window->ReparentWindow(msg);
                    } else {
                        AddOrphanWindow(wid);
                    };
                };
                break;

            case 12:
                Brief("ConfigureWindow");
                if (message_detail >= Full_Detail) {
                    printf("            window = %u\n", (unsigned int)GetUINT32(msg, 4));
                };
                if (smart_win_on) {
                    Window *window;
                    UINT32 wid = GetUINT32(msg, 4);
                    window = Window::FindWindow(wid);
#ifdef WIN_STOP
                    if (!window) {
	  fprintf(stderr, "PROG specified non-existing parent window id: %u\n",
		  (unsigned int)wid);
	  terminate();
	};
#endif
                    if (window) {
                        window->ConfigureWindow(msg);
                    } else {
                        AddOrphanWindow(wid);
                    };
                };
                break;

                // *** InternAtom ***
            case 16:
                Brief("InternAtom\n");
                if (message_detail >= Full_Detail) {
                    UINT32 n = (UINT32)GetUINT16(msg, 4);
                    UINT32 p = (((UINT32)GetUINT16(msg, 2)) - 2) * 4 - n;

                    printf("      only-if-exists = %u\n", (unsigned int)msg[1]);
                    printf("      request length = %u\n", (unsigned int)GetUINT16(msg, 2));
                    printf("            (padding = %u\n)", (unsigned int)p);
                    printf("         name length = %u\n", (unsigned int)n);
                    printf("              unused = %u\n", (unsigned int)GetUINT16(msg, 6));

                    printf("                name = '");
                    for (UINT32 i = 0; i < n; i++) {
                        printf("%c", (char)msg[8 + i]);
                    };
                    printf("'\n");
                };
                break;

            case 45:
                Brief("Open Font\n");
                break;

            case 38: {
                Brief("QueryPointer\n");
                // *** Create message containing response to QueryPointer ***
                Event msg_qry_pointer(event_context, PROG_message_count - 1);   // sequence number

                SendMessage(msg_qry_pointer, No_Mod, PROG);
                // *** Change PROG message into NoOperation to   ***
                // *** keep X server's sequence number on track. ***
                PutUINT8(msg, 0, 127);
                PutUINT16(msg, 2, 1);
                msg.SetSize(4);
                break;
            }
            case 39: {
                Brief("GetMotionEvents");
                // *** Create message containing response to GetMotionEvents ***
                Event get_motion_event(event_context,
                                       PROG_message_count - 1, // sequence number
                                       GetUINT32(msg, 8),      // start time
                                       GetUINT32(msg, 12)      // stop time
                );

                SendMessage(get_motion_event,
                            No_Mod,
                            PROG);
                // *** Change PROG message into NoOperation to   ***
                // *** keep X server's sequence number on track. ***
                PutUINT8(msg, 0, 127);
                PutUINT16(msg, 2, 1);
                msg.SetSize(4);
                break;
            }
            case 55:
                Brief("CreateGC\n");

                if (message_detail >= Full_Detail) {
                    printf("      cid = %u\n", (unsigned int)GetUINT32(msg, 4));
                };
                break;

            case 76:
                Brief("ImageText8\n");

                if (message_detail >= Full_Detail) {

                    printf("Length = %i.  Content = '\n", (unsigned int)msg[1]);
                    for (i = 0; i < msg[1]; i++) {
                        printf("%c", msg[16 + i]);
                    };
                    printf("'  Values =\n");
                    for (i = 0; i < msg[1]; i++) {
                        printf("%u -> (%u, %u)  ", (unsigned int)msg[16 + i],
                               (unsigned int)GetUINT16(msg, 12),
                               (unsigned int)GetUINT16(msg, 14));
                    };
                    printf("\n");
                };
                break;

            default:
                Brief("???\n");
        }; /* end switch */
    }; /* end-if size>0 */
};
