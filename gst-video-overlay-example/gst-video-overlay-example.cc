/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/ 

#include <glib-unix.h>
#include <gst/gst.h>
#include <glib.h>

#define GST_APP_SUMMARY "GStreamer Sample application Text Overlay Example \
                         \n usage: gst-video-overlay -i <H264>.mp4 -t textonvideo"

//static gchar *video_file = NULL;
//static gchar *overlay_text = NULL;

 /**
  * GstAppContext:
  * @pipeline: Pipeline connecting all the elements for Use Case.
  * @plugins : List of all the plugins used in Pipeline.
  * @mloop   : Main Loop for the Application.
  *
  * Application context to pass information between the functions.
  */
typedef struct
{
  // Pointer to the pipeline
  GstElement *pipeline;
  // Pointer to the mainloop
  GMainLoop  *loop;
  //Input sources
  gchar *video_file;
  gchar *overlay_text;
} GstAppContext;

/**
  * Create and initialize application context:
  *
  * @param NULL
  */
static GstAppContext *
gst_app_context_new ()
{
    // Allocate memory for the new context
    GstAppContext *ctx = (GstAppContext *) g_new0 (GstAppContext, 1);

    if (NULL == ctx) {
      g_printerr ("\n Unable to create App Context\n");
      return NULL;
    }

    // Initialize the context fields
    ctx->pipeline = NULL;
    ctx->loop = NULL;
    ctx->video_file = NULL;
    ctx->overlay_text = NULL;

    return ctx;
}

/**
  * Free Application context:
  *
  * @param appctx application context object
  */
static void
gst_app_context_free (GstAppContext * appctx)
{
    // If specific pointer is not NULL, unref it
    if (appctx->loop != NULL) {
      g_main_loop_unref (appctx->loop);
      appctx->loop = NULL;
    }

    if (appctx->pipeline != NULL) {
      gst_object_unref (appctx->pipeline);
      appctx->pipeline = NULL;
    }

    if (appctx->video_file != NULL)
      g_free (appctx->video_file);

    if (appctx->overlay_text != NULL)
      g_free (appctx->overlay_text);

    if (appctx != NULL)
      g_free ((gpointer)appctx);
}

static void
print_state (GstElement *pipeline) {
    GstState current, pending;
    gst_element_get_state (pipeline, &current, &pending, GST_CLOCK_TIME_NONE);
    g_print("Pipeline transitioning: %s -> %s\n",
            gst_element_state_get_name (current),
            gst_element_state_get_name (pending));
}

/**
  * Handles interrupt by CTRL+C.
  *
  * @param userdata pointer to AppContext.
  * @return FALSE if the source should be removed, else TRUE.
  */
gboolean
handle_interrupt_signal (gpointer userdata)
{
    GstAppContext *appctx = (GstAppContext *) userdata;
    GstState state, pending;

    g_print ("\n\nReceived an interrupt signal, send EOS ...\n");

    if (!gst_element_get_state (
        appctx->pipeline, &state, &pending, GST_CLOCK_TIME_NONE)) {
      gst_printerr ("ERROR: get current state!\n");
      gst_element_send_event (appctx->pipeline, gst_event_new_eos ());
      return TRUE;
    }

    if (state == GST_STATE_PLAYING) {
      gst_element_send_event (appctx->pipeline, gst_event_new_eos ());
    } else {
      g_main_loop_quit (appctx->loop);
    }
    return TRUE;
}

/**
  * Handles End-Of-Stream(eos) Event.
  *
  * @param bus Gstreamer bus for Mesaage passing in Pipeline.
  * @param message Gstreamer eos Event Message.
  * @param userdata Pointer to Main Loop for Handling eos.
  */
static
gboolean eos_cb (GstBus *bus, GstMessage *msg, gpointer user_data) {
    g_print ("End of stream reached.\n");
    g_main_loop_quit ((GMainLoop *) user_data);
    return FALSE;
}

/**
  * Handles error events.
  *
  * @param bus Gstreamer bus for Mesaage passing in Pipeline.
  * @param message Gstreamer Error Event Message.
  * @param userdata Pointer to Main Loop for Handling Error.
  */
static
gboolean error_cb (GstBus *bus, GstMessage *msg, gpointer user_data) {
    GError *err;
    gchar *debug_info;

    gst_message_parse_error (msg, &err, &debug_info);
    g_printerr ("Error from %s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
    g_printerr ("Debug info: %s\n", debug_info ? debug_info : "none");

    g_clear_error (&err);
    g_free (debug_info);

    g_main_loop_quit ((GMainLoop *) user_data);
    return FALSE;
}

/**
  * Handles warning events.
  *
  * @param bus Gstreamer bus for Mesaage passing in Pipeline.
  * @param message Gstreamer Error Event Message.
  * @param userdata Pointer to Main Loop for Handling Error.
  */
static
gboolean warning_cb (GstBus *bus, GstMessage *msg, gpointer user_data) {
    GError *err;
    gchar *debug_info;

    gst_message_parse_warning (msg, &err, &debug_info);
    g_printerr ("Warning from %s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
    g_printerr ("Debug info: %s\n", debug_info ? debug_info : "none");

    g_clear_error (&err);
    g_free (debug_info);
    return TRUE;
}

gint
main (int argc, char *argv[]) {
    GstAppContext *appctx = NULL;
    GstElement *filesrc, *decodebin, *videoconvert, *textoverlay, *waylandsink;
    GstBus *bus;
    GError *error = NULL;
    guint intrpt_watch_id = 0;

    // create the app context
    appctx = gst_app_context_new ();
    if (NULL == appctx) {
      g_printerr ("\n Failed app context Initializing: Unknown error!\n");
      return -1;
    }

    // Setting Display environment variables
    setenv ("XDG_RUNTIME_DIR", "/dev/socket/weston", 0);
    setenv ("WAYLAND_DISPLAY", "wayland-1", 0);

    // Define command-line options
    GOptionEntry entries[] = {
        {"video", 'i', 0, G_OPTION_ARG_STRING, &appctx->video_file,
         "Input video file", "FILE"},
        {"text", 't', 0, G_OPTION_ARG_STRING, &appctx->overlay_text,
         "Overlay text", "TEXT"},
        { NULL}
    };

    GOptionContext *context = g_option_context_new (GST_APP_SUMMARY);
    g_option_context_add_main_entries (context, entries, NULL);

    // Parse command-line arguments
    g_option_context_add_group (context, gst_init_get_option_group());
    if (!g_option_context_parse (context, &argc, &argv, &error)) {
        g_printerr ("Failed to parse options: %s\n", error->message);
        g_clear_error (&error);
        return -1;
    }
    g_option_context_free(context);

    //Initialize GST library.
    gst_init (&argc, &argv);

    if (!appctx->video_file || !appctx->overlay_text) {
        g_printerr("Usage: %s --video <file> --text <overlay>\n", argv[0]);
        return -1;
    }

    // Create pipeline
    appctx->pipeline = gst_pipeline_new("pipeline");

    filesrc = gst_element_factory_make("filesrc", "source");
    decodebin = gst_element_factory_make("decodebin", "decoder");
    videoconvert = gst_element_factory_make("videoconvert", "converter");
    textoverlay = gst_element_factory_make("textoverlay", "overlay");
    waylandsink = gst_element_factory_make("waylandsink", "sink");

    if (!appctx->pipeline || !filesrc || !decodebin || !videoconvert || !textoverlay || !waylandsink) {
        g_printerr("Failed to create elements.\n");
        return -1;
    }

    // Set properties
    g_object_set (filesrc, "location", appctx->video_file, NULL);
    g_object_set (textoverlay, "text", appctx->overlay_text, NULL);
    g_object_set (waylandsink, "fullscreen", TRUE, NULL);

    // Add elements to pipeline
    gst_bin_add_many (GST_BIN (appctx->pipeline), filesrc, decodebin, videoconvert, textoverlay, waylandsink, NULL);

    // Link elements
    gst_element_link (filesrc, decodebin);
    gst_element_link_many (videoconvert, textoverlay, waylandsink, NULL);

    // Connect decodebin's pad dynamically
    g_signal_connect (decodebin, "pad-added",
                      G_CALLBACK(+[](GstElement *src, GstPad *pad, gpointer data) {
        GstElement *convert = (GstElement *)data;
        GstPad *sinkpad = gst_element_get_static_pad (convert, "sink");
        gst_pad_link(pad, sinkpad);
        gst_object_unref(sinkpad);
    }), videoconvert);

    appctx->loop = g_main_loop_new (NULL, FALSE);

    bus = gst_element_get_bus (appctx->pipeline);
    gst_bus_add_signal_watch (bus);
    g_signal_connect (bus, "message::error", G_CALLBACK(error_cb), appctx->loop);
    g_signal_connect (bus, "message::warning", G_CALLBACK(warning_cb), NULL);
    g_signal_connect (bus, "message::eos", G_CALLBACK(eos_cb), appctx->loop);
    gst_object_unref (bus);

    // Register function for handling interrupt signals with the main loop.
    intrpt_watch_id = g_unix_signal_add (SIGINT, handle_interrupt_signal, appctx);

    // Set the pipeline to the PAUSED state, On successful transition
    // move application state to PLAYING state in state_changed_cb function
    g_print ("Setting pipeline to PLAYING state ...\n");
    switch (gst_element_set_state (appctx->pipeline, GST_STATE_PLAYING)) {
      case GST_STATE_CHANGE_FAILURE:
        g_printerr ("\n Failed to transition to PLAYING state!\n");
        if(intrpt_watch_id)
          g_source_remove (intrpt_watch_id);
        gst_app_context_free (appctx);
        return -1;
      case GST_STATE_CHANGE_NO_PREROLL:
        g_print ("\n Pipeline is live and does not need PREROLL.\n");
        break;
      case GST_STATE_CHANGE_ASYNC:
        g_print ("\n Pipeline is PREROLLING ...\n");
        break;
      case GST_STATE_CHANGE_SUCCESS:
        g_print ("\n Pipeline state change was successful\n");
        break;
    }
    print_state (appctx->pipeline);

    // Run main loop
    g_main_loop_run (appctx->loop);

    g_print ("\n Setting pipeline to NULL state ...\n");
    gst_element_set_state (appctx->pipeline, GST_STATE_NULL);
    print_state (appctx->pipeline);

    gst_app_context_free (appctx);
    g_print ("Application End Successfully\n");

    return 0;
}
